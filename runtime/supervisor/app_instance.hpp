#pragma once

#include <optional>
#include <string>
#include <vector>

#include "core/app_registry/app_manifest.hpp"
#include "runtime/fault/fault.hpp"
#include "runtime/lifecycle/lifecycle.hpp"
#include "runtime/quota/quota_ledger.hpp"

namespace aegis::runtime {

class AppInstance {
public:
    explicit AppInstance(const core::AppDescriptor& descriptor);

    [[nodiscard]] const std::string& instance_id() const;
    [[nodiscard]] const std::string& app_id() const;
    [[nodiscard]] const core::AppDescriptor& descriptor() const;
    [[nodiscard]] AppLifecycleState state() const;
    [[nodiscard]] const std::vector<AppLifecycleState>& history() const;
    void transition_to(AppLifecycleState state);

    void note_fault(AppFaultRecord fault);
    [[nodiscard]] const std::optional<AppFaultRecord>& last_fault() const;
    [[nodiscard]] std::size_t crash_count() const noexcept;
    [[nodiscard]] bool quarantined() const noexcept;
    void set_quarantined(bool quarantined) noexcept;

    [[nodiscard]] AppQuotaLedger& quota_ledger() noexcept;
    [[nodiscard]] const AppQuotaLedger& quota_ledger() const noexcept;

private:
    std::string instance_id_;
    const core::AppDescriptor& descriptor_;
    AppLifecycleState state_ {AppLifecycleState::LoadRequested};
    std::vector<AppLifecycleState> history_;
    AppQuotaLedger quota_ledger_;
    std::optional<AppFaultRecord> last_fault_;
    std::size_t crash_count_ {0};
    bool quarantined_ {false};
};

}  // namespace aegis::runtime
