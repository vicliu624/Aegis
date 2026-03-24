#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "core/app_registry/app_permissions.hpp"

namespace aegis::core {

enum class BackgroundExecutionMode {
    Unsupported,
    Suspended,
    Resident,
};

enum class PermissionGrantState {
    Granted,
    Denied,
};

struct PermissionPolicyEntry {
    AppPermissionId permission {};
    PermissionGrantState state {PermissionGrantState::Denied};
    std::string reason;
};

struct AppRuntimePolicy {
    BackgroundExecutionMode background_execution_mode {BackgroundExecutionMode::Unsupported};
    std::vector<PermissionPolicyEntry> permissions;
};

[[nodiscard]] std::string_view to_string(BackgroundExecutionMode mode);
[[nodiscard]] const PermissionPolicyEntry* find_permission_policy(
    const AppRuntimePolicy& policy,
    AppPermissionId permission);
[[nodiscard]] bool is_permission_granted(const AppRuntimePolicy& policy, AppPermissionId permission);
[[nodiscard]] std::string permission_reason_or_default(const AppRuntimePolicy& policy,
                                                       AppPermissionId permission);

}  // namespace aegis::core
