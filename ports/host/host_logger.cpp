#include "ports/host/host_logger.hpp"

#include <iostream>

namespace aegis::ports::host {

void HostLogger::info(std::string_view category, std::string_view message) {
    std::cout << "[" << category << "] " << message << '\n';
}

void HostLogger::error(std::string_view category, std::string_view message) {
    std::cerr << "[" << category << "] " << message << '\n';
}

}  // namespace aegis::ports::host
