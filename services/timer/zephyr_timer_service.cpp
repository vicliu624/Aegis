#include "services/timer/zephyr_timer_service.hpp"

#include <zephyr/kernel.h>

namespace aegis::services {

struct ZephyrTimerService::TimerInstance {
    k_timer timer {};
};

namespace {

void timer_expiry_handler(k_timer* timer) {
    ARG_UNUSED(timer);
}

}  // namespace

ZephyrTimerService::ZephyrTimerService() = default;

ZephyrTimerService::~ZephyrTimerService() {
    for (auto& [id, timer] : timers_) {
        ARG_UNUSED(id);
        k_timer_stop(&timer->timer);
    }
}

int ZephyrTimerService::create(uint32_t timeout_ms, bool repeat) {
    auto instance = std::make_unique<TimerInstance>();
    k_timer_init(&instance->timer, timer_expiry_handler, nullptr);

    const auto id = next_id_++;
    k_timer_start(&instance->timer,
                  K_MSEC(timeout_ms),
                  repeat ? K_MSEC(timeout_ms) : K_NO_WAIT);
    timers_.emplace(id, std::move(instance));
    return id;
}

bool ZephyrTimerService::cancel(int timer_id) {
    const auto it = timers_.find(timer_id);
    if (it == timers_.end()) {
        return false;
    }
    k_timer_stop(&it->second->timer);
    timers_.erase(it);
    return true;
}

}  // namespace aegis::services
