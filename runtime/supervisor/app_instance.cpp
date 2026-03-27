#include "runtime/supervisor/app_instance.hpp"

#include <stdexcept>
#include <utility>

namespace aegis::runtime {

AppInstance::AppInstance(const core::AppDescriptor& descriptor)
    : instance_id_(descriptor.manifest.app_id + ":instance"),
      descriptor_(descriptor),
      state_(descriptor.validated ? AppLifecycleState::Validated : AppLifecycleState::Discovered),
      history_ {state_},
      quota_ledger_(descriptor.manifest.memory_budget) {}

const std::string& AppInstance::instance_id() const {
    return instance_id_;
}

const std::string& AppInstance::app_id() const {
    return descriptor_.manifest.app_id;
}

const core::AppDescriptor& AppInstance::descriptor() const {
    return descriptor_;
}

AppLifecycleState AppInstance::state() const {
    return state_;
}

const std::vector<AppLifecycleState>& AppInstance::history() const {
    return history_;
}

void AppInstance::transition_to(AppLifecycleState state) {
    if (state == state_) {
        return;
    }
    if (!is_legal_transition(state_, state)) {
        throw std::runtime_error("illegal lifecycle transition from " +
                                 std::string(to_string(state_)) + " to " +
                                 std::string(to_string(state)));
    }
    state_ = state;
    history_.push_back(state_);
}

void AppInstance::note_fault(AppFaultRecord fault) {
    last_fault_ = std::move(fault);
    if (last_fault_ && last_fault_->fatal) {
        ++crash_count_;
    }
}

const std::optional<AppFaultRecord>& AppInstance::last_fault() const {
    return last_fault_;
}

std::size_t AppInstance::crash_count() const noexcept {
    return crash_count_;
}

bool AppInstance::quarantined() const noexcept {
    return quarantined_;
}

void AppInstance::set_quarantined(bool quarantined) noexcept {
    quarantined_ = quarantined;
}

AppQuotaLedger& AppInstance::quota_ledger() noexcept {
    return quota_ledger_;
}

const AppQuotaLedger& AppInstance::quota_ledger() const noexcept {
    return quota_ledger_;
}

}  // namespace aegis::runtime
