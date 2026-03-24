#include "services/power/zephyr_power_service.hpp"

#include <zephyr/kernel.h>

namespace aegis::services {

bool ZephyrPowerService::battery_present() const {
    return true;
}

bool ZephyrPowerService::low_power_mode_supported() const {
#if defined(CONFIG_PM)
    return true;
#else
    return false;
#endif
}

std::string ZephyrPowerService::describe_status() const {
    return "zephyr-power uptime_ms=" + std::to_string(k_uptime_get());
}

}  // namespace aegis::services
