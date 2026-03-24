#include "services/timer/in_memory_timer_service.hpp"

namespace aegis::services {

int InMemoryTimerService::create(uint32_t timeout_ms, bool repeat) {
    (void)timeout_ms;
    (void)repeat;
    const int id = next_id_++;
    active_timers_.insert(id);
    return id;
}

bool InMemoryTimerService::cancel(int timer_id) {
    return active_timers_.erase(timer_id) > 0;
}

}  // namespace aegis::services
