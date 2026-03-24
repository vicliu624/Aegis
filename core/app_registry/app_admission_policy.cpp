#include "core/app_registry/app_admission_policy.hpp"

namespace aegis::core {

AppAdmissionDecision AppAdmissionPolicy::evaluate(const AppDescriptor& descriptor,
                                                  const AppAdmissionContext& context) const {
    if (!descriptor.validated) {
        return AppAdmissionDecision {.allowed = false,
                                     .reason = descriptor.validation_error.empty() ? "invalid"
                                                                                  : descriptor.validation_error};
    }
    if (!context.registry.is_abi_compatible(descriptor, context.runtime_abi_version)) {
        return AppAdmissionDecision {.allowed = false, .reason = "abi mismatch"};
    }
    if (!context.registry.satisfies_requirements(descriptor, context.profile.capabilities)) {
        return AppAdmissionDecision {.allowed = false, .reason = "capability mismatch"};
    }
    if (descriptor.manifest.memory_budget.heap_bytes > context.profile.runtime_limits.max_app_memory_bytes) {
        return AppAdmissionDecision {.allowed = false, .reason = "memory budget too large"};
    }
    if (descriptor.manifest.memory_budget.ui_bytes > context.profile.runtime_limits.max_ui_memory_bytes) {
        return AppAdmissionDecision {.allowed = false, .reason = "ui memory budget too large"};
    }
    for (const auto permission : descriptor.manifest.requested_permissions) {
        if (!is_permission_granted(context.runtime_policy, permission)) {
            return AppAdmissionDecision {.allowed = false,
                                         .reason = "permission denied: " +
                                                   std::string(to_string(permission))};
        }
    }
    if (descriptor.manifest.allow_background &&
        context.runtime_policy.background_execution_mode == BackgroundExecutionMode::Unsupported) {
        return AppAdmissionDecision {
            .allowed = false,
            .reason = "background execution unsupported by current runtime policy",
        };
    }
    if (descriptor.manifest.singleton) {
        for (const auto& active_app_id : context.active_app_ids) {
            if (active_app_id == descriptor.manifest.app_id) {
                return AppAdmissionDecision {.allowed = false, .reason = "singleton already active"};
            }
        }
    }
    return AppAdmissionDecision {.allowed = true, .reason = "admitted"};
}

}  // namespace aegis::core
