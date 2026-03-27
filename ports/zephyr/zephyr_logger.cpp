#include "ports/zephyr/zephyr_logger.hpp"

#include <zephyr/sys/printk.h>

namespace aegis::ports::zephyr {

void ZephyrLogger::info(std::string_view category, std::string_view message) {
    printk("[aegis][%.*s] %.*s\n",
           static_cast<int>(category.size()),
           category.data(),
           static_cast<int>(message.size()),
           message.data());
}

void ZephyrLogger::error(std::string_view category, std::string_view message) {
    printk("[aegis][%.*s][error] %.*s\n",
           static_cast<int>(category.size()),
           category.data(),
           static_cast<int>(message.size()),
           message.data());
}

}  // namespace aegis::ports::zephyr
