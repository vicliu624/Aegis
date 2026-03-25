#include "services/time/zephyr_time_service.hpp"

#include <ctime>

#include <zephyr/kernel.h>
#include <zephyr/posix/posix_time.h>
#include <zephyr/sys/clock.h>

namespace aegis::services {

bool ZephyrTimeService::available() const {
    return true;
}

std::optional<int64_t> ZephyrTimeService::current_unix_ms() const {
    timespec ts {};
    if (sys_clock_gettime(SYS_CLOCK_REALTIME, &ts) != 0) {
        return std::nullopt;
    }
    const int64_t seconds = static_cast<int64_t>(ts.tv_sec);
    const int64_t millis = static_cast<int64_t>(ts.tv_nsec / 1000000LL);
    if (seconds <= 0) {
        return std::nullopt;
    }
    return (seconds * 1000LL) + millis;
}

std::string ZephyrTimeService::describe_source() const {
    if (const auto unix_ms = current_unix_ms(); unix_ms.has_value()) {
        return "zephyr-realtime-clock:" + std::to_string(*unix_ms);
    }
    return "zephyr-kernel-uptime:" + std::to_string(k_uptime_get());
}

}  // namespace aegis::services
