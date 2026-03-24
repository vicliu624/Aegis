#include "services/logging/console_logging_service.hpp"

namespace aegis::services {

ConsoleLoggingService::ConsoleLoggingService(platform::Logger& logger) : logger_(logger) {}

void ConsoleLoggingService::log(std::string tag, std::string message) {
    logger_.info("log", "[" + tag + "] " + message);
}

}  // namespace aegis::services
