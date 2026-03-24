#include "services/radio/zephyr_radio_service.hpp"

#include <zephyr/device.h>

namespace aegis::services {

ZephyrRadioService::ZephyrRadioService(std::string device_name)
    : device_name_(std::move(device_name)) {}

bool ZephyrRadioService::available() const {
    const auto* device = device_get_binding(device_name_.c_str());
    return device != nullptr && device_is_ready(device);
}

std::string ZephyrRadioService::backend_name() const {
    return available() ? "zephyr-device:" + device_name_ : "zephyr-device-absent:" + device_name_;
}

}  // namespace aegis::services
