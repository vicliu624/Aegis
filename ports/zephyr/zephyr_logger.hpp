#pragma once

#include "platform/logging/logger.hpp"

namespace aegis::ports::zephyr {

class ZephyrLogger : public platform::Logger {
public:
    void info(std::string_view category, std::string_view message) override;
    void error(std::string_view category, std::string_view message) override;
};

}  // namespace aegis::ports::zephyr
