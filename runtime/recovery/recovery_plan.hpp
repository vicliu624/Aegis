#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace aegis::runtime {

enum class RecoveryAction {
    StopExecution,
    DetachForegroundUi,
    RevokeServiceHandles,
    ReleaseOwnedResources,
    ReturnToShell,
    QuarantineInstance,
};

struct RecoveryPlan {
    bool terminate_instance {true};
    bool quarantine_instance {false};
    std::vector<RecoveryAction> actions;
    std::vector<std::string> released_resources;
};

[[nodiscard]] constexpr std::string_view to_string(RecoveryAction action) {
    switch (action) {
        case RecoveryAction::StopExecution:
            return "stop execution";
        case RecoveryAction::DetachForegroundUi:
            return "detach foreground ui";
        case RecoveryAction::RevokeServiceHandles:
            return "revoke service handles";
        case RecoveryAction::ReleaseOwnedResources:
            return "release owned resources";
        case RecoveryAction::ReturnToShell:
            return "return to shell";
        case RecoveryAction::QuarantineInstance:
            return "quarantine instance";
    }

    return "unknown";
}

}  // namespace aegis::runtime
