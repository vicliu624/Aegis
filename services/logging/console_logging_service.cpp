#include "services/logging/console_logging_service.hpp"

#include <iostream>

namespace aegis::services {

void ConsoleLoggingService::log(std::string tag, std::string message) {
    std::cout << "[log][" << tag << "] " << message << '\n';
}

}  // namespace aegis::services
