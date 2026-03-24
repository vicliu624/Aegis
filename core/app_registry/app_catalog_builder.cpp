#include "core/app_registry/app_catalog_builder.hpp"

#include "runtime/loader/runtime_loader.hpp"

namespace aegis::core {

namespace {

AppAdmissionContext make_context(const AppRegistry& registry,
                                 const device::DeviceProfile& profile,
                                 const AppRuntimePolicy& runtime_policy,
                                 std::uint32_t runtime_abi_version,
                                 const std::vector<std::string>& active_app_ids) {
    return AppAdmissionContext {
        .registry = registry,
        .profile = profile,
        .runtime_abi_version = runtime_abi_version,
        .runtime_policy = runtime_policy,
        .active_app_ids = active_app_ids,
    };
}

std::vector<std::string> validation_issue_notes(
    const AppDescriptor& descriptor,
    AppManifestValidationIssue::Severity severity) {
    std::vector<std::string> notes;
    for (const auto& issue : descriptor.validation_issues) {
        if (issue.severity == severity) {
            notes.push_back(issue.field + ": " + issue.message);
        }
    }
    return notes;
}

}  // namespace

AppCatalog AppCatalogBuilder::build(const AppRegistry& registry,
                                    const device::DeviceProfile& profile,
                                    const AppAdmissionPolicy& admission_policy,
                                    const AppCompatibilityEvaluator& compatibility_evaluator,
                                    const runtime::RuntimeLoader& runtime_loader,
                                    const AppRuntimePolicy& runtime_policy,
                                    std::uint32_t runtime_abi_version,
                                    const std::vector<std::string>& active_app_ids) const {
    AppCatalog catalog;
    std::vector<AppCatalogEntry> entries;

    const auto context =
        make_context(registry, profile, runtime_policy, runtime_abi_version, active_app_ids);
    for (const auto& app : registry.apps()) {
        AppCatalogEntry entry {.descriptor = app};
        const auto admission = admission_policy.evaluate(app, context);
        entry.compatibility_notes =
            compatibility_evaluator.evaluate(app, profile, runtime_policy).notes;
        if (!app.validated) {
            entry.state = AppCatalogEntryState::Invalid;
            entry.reason = admission.reason;
            entry.compatibility_notes =
                validation_issue_notes(app, AppManifestValidationIssue::Severity::Fatal);
        } else if (app.manifest.hidden) {
            entry.state = AppCatalogEntryState::Hidden;
            entry.reason = "hidden";
        } else if (!admission.allowed) {
            entry.state = AppCatalogEntryState::Incompatible;
            entry.reason = admission.reason;
        } else if (!runtime_loader.can_resolve_entry_symbol(app.manifest.entry_symbol)) {
            entry.state = AppCatalogEntryState::Incompatible;
            entry.reason = "entry symbol unavailable";
        }
        const auto warning_notes =
            validation_issue_notes(app, AppManifestValidationIssue::Severity::Warning);
        entry.compatibility_notes.insert(entry.compatibility_notes.end(),
                                         warning_notes.begin(),
                                         warning_notes.end());
        entries.push_back(std::move(entry));
    }

    catalog.set_entries(std::move(entries));
    return catalog;
}

}  // namespace aegis::core
