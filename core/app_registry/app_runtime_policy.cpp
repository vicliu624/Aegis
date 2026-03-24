#include "core/app_registry/app_runtime_policy.hpp"

namespace aegis::core {

std::string_view to_string(BackgroundExecutionMode mode) {
    switch (mode) {
        case BackgroundExecutionMode::Unsupported:
            return "unsupported";
        case BackgroundExecutionMode::Suspended:
            return "suspended";
        case BackgroundExecutionMode::Resident:
            return "resident";
    }

    return "unknown";
}

const PermissionPolicyEntry* find_permission_policy(const AppRuntimePolicy& policy,
                                                    AppPermissionId permission) {
    for (const auto& entry : policy.permissions) {
        if (entry.permission == permission) {
            return &entry;
        }
    }
    return nullptr;
}

bool is_permission_granted(const AppRuntimePolicy& policy, AppPermissionId permission) {
    const auto* entry = find_permission_policy(policy, permission);
    return entry != nullptr && entry->state == PermissionGrantState::Granted;
}

std::string permission_reason_or_default(const AppRuntimePolicy& policy, AppPermissionId permission) {
    if (const auto* entry = find_permission_policy(policy, permission)) {
        if (!entry->reason.empty()) {
            return entry->reason;
        }
        return entry->state == PermissionGrantState::Granted ? "granted" : "denied";
    }
    return "no runtime permission policy entry";
}

}  // namespace aegis::core
