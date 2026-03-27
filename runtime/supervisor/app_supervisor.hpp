#pragma once

#include <vector>

#include "runtime/fault/fault.hpp"
#include "runtime/recovery/recovery_plan.hpp"
#include "runtime/supervisor/app_instance.hpp"

namespace aegis::runtime {

struct RecoveryContext {
    std::vector<std::string> released_resources;
    bool return_to_shell {true};
};

class AppSupervisor {
public:
    void note_fault(AppInstance& instance, AppFaultRecord fault) const;
    [[nodiscard]] RecoveryPlan build_recovery_plan(const AppInstance& instance,
                                                   const RecoveryContext& context = {}) const;
};

}  // namespace aegis::runtime
