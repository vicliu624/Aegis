#pragma once

#include <string_view>

namespace aegis::platform {

class Logger {
public:
    virtual ~Logger() = default;

    virtual void info(std::string_view category, std::string_view message) = 0;
    virtual void error(std::string_view category, std::string_view message) = 0;
};

}  // namespace aegis::platform
