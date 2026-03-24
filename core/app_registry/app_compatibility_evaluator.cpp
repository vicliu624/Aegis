#include "core/app_registry/app_compatibility_evaluator.hpp"

#include <string>

#include "device/common/capability/capability_set.hpp"

namespace aegis::core {

namespace {

std::string capability_note_prefix(const char* prefix, device::CapabilityId id) {
    return std::string(prefix) + std::string(device::to_string(id));
}

}  // namespace

AppCompatibilityReport AppCompatibilityEvaluator::evaluate(const AppDescriptor& descriptor,
                                                           const device::DeviceProfile& profile,
                                                           const AppRuntimePolicy& runtime_policy) const {
    AppCompatibilityReport report;
    if (!descriptor.validated) {
        return report;
    }

    for (const auto& requirement : descriptor.manifest.optional_capabilities) {
        if (!profile.capabilities.has(requirement.id, requirement.min_level)) {
            report.notes.push_back(capability_note_prefix("optional capability unavailable: ",
                                                         requirement.id));
        }
    }

    for (const auto& requirement : descriptor.manifest.preferred_capabilities) {
        if (!profile.capabilities.has(requirement.id, requirement.min_level)) {
            report.notes.push_back(capability_note_prefix("preferred capability unavailable: ",
                                                         requirement.id));
        }
    }

    if (descriptor.manifest.allow_background &&
        runtime_policy.background_execution_mode == BackgroundExecutionMode::Unsupported) {
        report.notes.push_back("background execution unsupported by current runtime policy");
    }

    for (const auto permission : descriptor.manifest.requested_permissions) {
        if (!is_permission_granted(runtime_policy, permission)) {
            report.notes.push_back("permission denied by runtime policy: " +
                                   std::string(to_string(permission)) + " (" +
                                   permission_reason_or_default(runtime_policy, permission) + ")");
        }
    }

    return report;
}

}  // namespace aegis::core
