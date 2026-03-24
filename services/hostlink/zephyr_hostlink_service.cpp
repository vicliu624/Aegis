#include "services/hostlink/zephyr_hostlink_service.hpp"

#include <zephyr/device.h>

namespace aegis::services {

ZephyrHostlinkService::ZephyrHostlinkService(std::string transport_device_name,
                                             std::string bridge_name)
    : transport_device_name_(std::move(transport_device_name)),
      bridge_name_(std::move(bridge_name)) {}

bool ZephyrHostlinkService::available() const {
    if (transport_device_name_.empty()) {
        return false;
    }
    const auto* device = device_get_binding(transport_device_name_.c_str());
    return device != nullptr && device_is_ready(device);
}

bool ZephyrHostlinkService::connected() const {
    return available();
}

std::string ZephyrHostlinkService::transport_name() const {
    return available() ? transport_device_name_ : "hostlink-unavailable";
}

std::string ZephyrHostlinkService::bridge_name() const {
    return bridge_name_;
}

}  // namespace aegis::services
