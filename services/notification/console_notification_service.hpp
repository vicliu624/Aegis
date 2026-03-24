#pragma once

#include "platform/logging/logger.hpp"
#include "services/common/service_interfaces.hpp"

namespace aegis::services {

class ConsoleNotificationService : public INotificationService {
public:
    explicit ConsoleNotificationService(platform::Logger& logger);
    void notify(std::string title, std::string body) override;

private:
    platform::Logger& logger_;
};

}  // namespace aegis::services
