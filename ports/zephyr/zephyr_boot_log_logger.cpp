#include "ports/zephyr/zephyr_boot_log_logger.hpp"

#include <string>

#include "ports/zephyr/zephyr_shell_display_adapter.hpp"

namespace aegis::ports::zephyr {

ZephyrBootLogLogger::ZephyrBootLogLogger(platform::Logger& delegate) : delegate_(delegate) {
    k_mutex_init(&mutex_);
}

void ZephyrBootLogLogger::attach_display(ZephyrShellDisplayAdapter* display) {
    (void)k_mutex_lock(&mutex_, K_FOREVER);
    display_ = display;
    (void)k_mutex_unlock(&mutex_);
}

void ZephyrBootLogLogger::info(std::string_view category, std::string_view message) {
    (void)k_mutex_lock(&mutex_, K_FOREVER);
    delegate_.info(category, message);
    if (display_ != nullptr) {
        display_->record_boot_log(category, message);
    }
    (void)k_mutex_unlock(&mutex_);
}

void ZephyrBootLogLogger::error(std::string_view category, std::string_view message) {
    (void)k_mutex_lock(&mutex_, K_FOREVER);
    delegate_.error(category, message);
    if (display_ != nullptr) {
        display_->record_boot_log(category, std::string("[error] ") + std::string(message));
    }
    (void)k_mutex_unlock(&mutex_);
}

}  // namespace aegis::ports::zephyr
