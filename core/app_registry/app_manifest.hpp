#pragma once

#include <string>
#include <vector>

#include "core/app_registry/app_permissions.hpp"
#include "device/common/capability/capability_set.hpp"

namespace aegis::core {

struct AppCapabilityRequirement {
    device::CapabilityId id {};
    device::CapabilityLevel min_level {device::CapabilityLevel::Degraded};
};

struct AppMemoryBudget {
    std::size_t heap_bytes {0};
    std::size_t ui_bytes {0};
};

struct AppManifestValidationIssue {
    enum class Severity {
        Fatal,
        Warning,
    };

    std::string field;
    std::string message;
    Severity severity {Severity::Fatal};
};

struct AppManifest {
    std::string app_id;
    std::string display_name;
    std::string version;
    std::uint32_t abi_version {0};
    std::string binary;
    std::string entry_symbol;
    std::string icon;
    std::string description;
    std::vector<AppCapabilityRequirement> required_capabilities;
    std::vector<AppCapabilityRequirement> optional_capabilities;
    std::vector<AppCapabilityRequirement> preferred_capabilities;
    std::vector<AppPermissionId> requested_permissions;
    AppMemoryBudget memory_budget;
    bool singleton {true};
    bool allow_background {false};
    std::string category;
    int order_hint {0};
    bool hidden {false};
};

struct ManifestParseResult {
    bool ok {false};
    AppManifest manifest;
    std::vector<AppManifestValidationIssue> issues;
};

struct AppDescriptor {
    AppManifest manifest;
    std::string app_dir;
    std::string manifest_path;
    std::string binary_path;
    std::string icon_path;
    bool binary_exists {false};
    bool icon_exists {false};
    bool validated {false};
    std::string validation_error;
    std::vector<AppManifestValidationIssue> validation_issues;

    [[nodiscard]] bool has_validation_issue_severity(AppManifestValidationIssue::Severity severity) const;
    [[nodiscard]] std::vector<AppManifestValidationIssue> validation_issues_with_severity(
        AppManifestValidationIssue::Severity severity) const;
};

struct AppAdmissionDecision {
    bool allowed {false};
    std::string reason;
};

class ManifestParser {
public:
    [[nodiscard]] ManifestParseResult parse_text(const std::string& manifest_text) const;
};

}  // namespace aegis::core
