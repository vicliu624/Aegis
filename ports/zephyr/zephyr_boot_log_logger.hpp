#pragma once

#include <zephyr/kernel.h>

#include "platform/logging/logger.hpp"

namespace aegis::ports::zephyr {

class ZephyrShellDisplayAdapter;

class ZephyrBootLogLogger : public platform::Logger {
public:
    explicit ZephyrBootLogLogger(platform::Logger& delegate);

    void attach_display(ZephyrShellDisplayAdapter* display);
    void info(std::string_view category, std::string_view message) override;
    void error(std::string_view category, std::string_view message) override;

private:
    platform::Logger& delegate_;
    ZephyrShellDisplayAdapter* display_ {nullptr};
    mutable k_mutex mutex_ {};
};

}  // namespace aegis::ports::zephyr
