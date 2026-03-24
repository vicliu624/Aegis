#include "core/app_registry/app_registry.hpp"

#include <set>

#include "sdk/include/aegis/abi.h"

namespace aegis::core {

namespace {

bool is_relative_asset_path(std::string_view path) {
    return !path.empty() && path.front() != '/' && path.find("..") == std::string_view::npos;
}

std::string join_app_path(std::string_view app_dir, std::string_view relative_path) {
    if (app_dir.empty()) {
        return std::string(relative_path);
    }
    return std::string(app_dir) + "/" + std::string(relative_path);
}

std::string summarize_issues(const std::vector<AppManifestValidationIssue>& issues) {
    for (const auto& issue : issues) {
        if (issue.severity == AppManifestValidationIssue::Severity::Fatal) {
            return issue.field + ": " + issue.message;
        }
    }
    return {};
}

}  // namespace

AppRegistry::AppRegistry(std::shared_ptr<AppPackageSource> app_source)
    : app_source_(std::move(app_source)) {}

void AppRegistry::scan() {
    apps_.clear();

    for (const auto& package : app_source_->discover()) {
        AppDescriptor descriptor;
        descriptor.app_dir = package.app_dir;
        descriptor.manifest_path = package.manifest_path;
        descriptor.binary_path = package.binary_path;
        descriptor.icon_path = package.icon_path;
        descriptor.binary_exists = package.binary_exists;
        descriptor.icon_exists = package.icon_exists;

        const auto parse_result = parser_.parse_text(package.manifest_text);
        descriptor.manifest = parse_result.manifest;
        descriptor.validation_issues = parse_result.issues;
        if (parse_result.ok) {
            descriptor.binary_path = join_app_path(package.app_dir, descriptor.manifest.binary);
            if (!descriptor.manifest.icon.empty()) {
                descriptor.icon_path = join_app_path(package.app_dir, descriptor.manifest.icon);
            } else {
                descriptor.icon_path.clear();
            }
            descriptor.binary_exists = app_source_->exists(descriptor.binary_path);
            descriptor.icon_exists =
                !descriptor.icon_path.empty() && app_source_->exists(descriptor.icon_path);

            if (!descriptor.binary_exists) {
                descriptor.validation_issues.push_back(
                    {.field = "binary", .message = "binary file missing"});
            }
            if (!descriptor.icon_exists) {
                descriptor.validation_issues.push_back(
                    {.field = "icon",
                     .message = "icon file missing; shell will use placeholder art",
                     .severity = AppManifestValidationIssue::Severity::Warning});
            }
            if (!is_relative_asset_path(descriptor.manifest.binary)) {
                descriptor.validation_issues.push_back(
                    {.field = "binary", .message = "binary path must remain relative"});
            }
            if (!descriptor.manifest.icon.empty() &&
                !is_relative_asset_path(descriptor.manifest.icon)) {
                descriptor.validation_issues.push_back(
                    {.field = "icon",
                     .message = "icon path must remain relative",
                     .severity = AppManifestValidationIssue::Severity::Warning});
            }
        }

        descriptor.validated =
            !descriptor.has_validation_issue_severity(AppManifestValidationIssue::Severity::Fatal);
        descriptor.validation_error = summarize_issues(descriptor.validation_issues);
        apps_.push_back(std::move(descriptor));
    }

    std::set<std::string> seen_app_ids;
    for (auto& app : apps_) {
        if (app.manifest.app_id.empty()) {
            continue;
        }
        if (!seen_app_ids.insert(app.manifest.app_id).second) {
            app.validated = false;
            app.validation_issues.push_back(
                {.field = "app_id", .message = "duplicate app_id in registry scan"});
            app.validation_error = summarize_issues(app.validation_issues);
        }
    }
}

const std::vector<AppDescriptor>& AppRegistry::apps() const {
    return apps_;
}

const AppDescriptor* AppRegistry::find_by_id(const std::string& app_id) const {
    for (const auto& app : apps_) {
        if (app.manifest.app_id == app_id) {
            return &app;
        }
    }

    return nullptr;
}

bool AppRegistry::satisfies_requirements(const AppDescriptor& descriptor,
                                         const device::CapabilitySet& capabilities) const {
    if (!descriptor.validated) {
        return false;
    }

    for (const auto& capability : descriptor.manifest.required_capabilities) {
        if (!capabilities.has(capability.id, capability.min_level)) {
            return false;
        }
    }

    return true;
}

bool AppRegistry::is_abi_compatible(const AppDescriptor& descriptor,
                                    std::uint32_t runtime_abi_version) const {
    return descriptor.validated && descriptor.manifest.abi_version == runtime_abi_version;
}

}  // namespace aegis::core
