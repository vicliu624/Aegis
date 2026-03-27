#include "runtime/supervisor/app_supervisor.hpp"

#include <utility>

namespace aegis::runtime {

void AppSupervisor::note_fault(AppInstance& instance, AppFaultRecord fault) const {
    instance.note_fault(std::move(fault));
    if (instance.crash_count() >= 3) {
        instance.set_quarantined(true);
    }
}

RecoveryPlan AppSupervisor::build_recovery_plan(const AppInstance& instance,
                                                const RecoveryContext& context) const {
    RecoveryPlan plan;
    plan.actions = {
        RecoveryAction::StopExecution,
        RecoveryAction::DetachForegroundUi,
        RecoveryAction::RevokeServiceHandles,
        RecoveryAction::ReleaseOwnedResources,
    };
    if (context.return_to_shell) {
        plan.actions.push_back(RecoveryAction::ReturnToShell);
    }
    if (instance.quarantined()) {
        plan.quarantine_instance = true;
        plan.actions.push_back(RecoveryAction::QuarantineInstance);
    }
    plan.released_resources = context.released_resources;
    return plan;
}

}  // namespace aegis::runtime
