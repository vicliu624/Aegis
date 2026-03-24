#include "services/notification/console_notification_service.hpp"

namespace aegis::services {

ConsoleNotificationService::ConsoleNotificationService(platform::Logger& logger) : logger_(logger) {}

void ConsoleNotificationService::notify(std::string title, std::string body) {
    logger_.info("notify", "[" + title + "] " + body);
}

}  // namespace aegis::services
