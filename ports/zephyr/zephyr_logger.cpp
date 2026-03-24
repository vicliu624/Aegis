#include "ports/zephyr/zephyr_logger.hpp"

#include <string>

#include <zephyr/sys/printk.h>

namespace aegis::ports::zephyr {

void ZephyrLogger::info(std::string_view category, std::string_view message) {
    std::string category_text(category);
    std::string message_text(message);
    printk("[aegis][%s] %s\n", category_text.c_str(), message_text.c_str());
}

void ZephyrLogger::error(std::string_view category, std::string_view message) {
    std::string category_text(category);
    std::string message_text(message);
    printk("[aegis][%s][error] %s\n", category_text.c_str(), message_text.c_str());
}

}  // namespace aegis::ports::zephyr
