#include "services/notification/console_notification_service.hpp"

#include <iostream>

namespace aegis::services {

void ConsoleNotificationService::notify(std::string title, std::string body) {
    std::cout << "[notify][" << title << "] " << body << '\n';
}

}  // namespace aegis::services
