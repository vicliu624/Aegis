#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>

#include "services/common/service_interfaces.hpp"

namespace aegis::services {

class ZephyrTimerService : public ITimerService {
public:
    ZephyrTimerService();
    ~ZephyrTimerService() override;

    int create(uint32_t timeout_ms, bool repeat) override;
    bool cancel(int timer_id) override;

private:
    struct TimerInstance;

    int next_id_ {1};
    std::unordered_map<int, std::unique_ptr<TimerInstance>> timers_;
};

}  // namespace aegis::services
