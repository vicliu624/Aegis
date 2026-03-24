#include "services/notification/zephyr_notification_service.hpp"

#include <zephyr/kernel.h>

namespace aegis::services {

ZephyrNotificationService::ZephyrNotificationService(platform::Logger& logger) : logger_(logger) {}

void ZephyrNotificationService::notify(std::string title, std::string body) {
    logger_.info("notify",
                 "[zephyr@" + std::to_string(k_uptime_get()) + "] [" + title + "] " + body);
}

}  // namespace aegis::services
