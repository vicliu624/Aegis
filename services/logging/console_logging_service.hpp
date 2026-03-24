#pragma once

#include "platform/logging/logger.hpp"
#include "services/common/service_interfaces.hpp"

namespace aegis::services {

class ConsoleLoggingService : public ILoggingService {
public:
    explicit ConsoleLoggingService(platform::Logger& logger);
    void log(std::string tag, std::string message) override;

private:
    platform::Logger& logger_;
};

}  // namespace aegis::services
