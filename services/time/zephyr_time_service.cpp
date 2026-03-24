#include "services/time/zephyr_time_service.hpp"

#include <zephyr/kernel.h>

namespace aegis::services {

bool ZephyrTimeService::available() const {
    return true;
}

std::string ZephyrTimeService::describe_source() const {
    return "zephyr-kernel-uptime:" + std::to_string(k_uptime_get());
}

}  // namespace aegis::services
