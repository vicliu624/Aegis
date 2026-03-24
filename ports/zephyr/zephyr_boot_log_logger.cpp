#include "ports/zephyr/zephyr_boot_log_logger.hpp"

#include <string>

#include "ports/zephyr/zephyr_shell_display_adapter.hpp"

namespace aegis::ports::zephyr {

ZephyrBootLogLogger::ZephyrBootLogLogger(platform::Logger& delegate) : delegate_(delegate) {}

void ZephyrBootLogLogger::attach_display(ZephyrShellDisplayAdapter* display) {
    display_ = display;
}

void ZephyrBootLogLogger::info(std::string_view category, std::string_view message) {
    delegate_.info(category, message);
    if (display_ != nullptr) {
        display_->record_boot_log(category, message);
    }
}

void ZephyrBootLogLogger::error(std::string_view category, std::string_view message) {
    delegate_.error(category, message);
    if (display_ != nullptr) {
        display_->record_boot_log(category, std::string("[error] ") + std::string(message));
    }
}

}  // namespace aegis::ports::zephyr
