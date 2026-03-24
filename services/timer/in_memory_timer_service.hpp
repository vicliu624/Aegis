#pragma once

#include <unordered_set>

#include "services/common/service_interfaces.hpp"

namespace aegis::services {

class InMemoryTimerService : public ITimerService {
public:
    int create(uint32_t timeout_ms, bool repeat) override;
    bool cancel(int timer_id) override;

private:
    int next_id_ {1};
    std::unordered_set<int> active_timers_;
};

}  // namespace aegis::services
