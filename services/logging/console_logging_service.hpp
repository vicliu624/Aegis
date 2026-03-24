#pragma once

#include "services/common/service_interfaces.hpp"

namespace aegis::services {

class ConsoleLoggingService : public ILoggingService {
public:
    void log(std::string tag, std::string message) override;
};

}  // namespace aegis::services
