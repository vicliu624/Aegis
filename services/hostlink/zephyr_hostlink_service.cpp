#include "services/hostlink/zephyr_hostlink_service.hpp"

#include <zephyr/device.h>

#include "ports/zephyr/zephyr_tlora_pager_board_runtime.hpp"

namespace aegis::services {

ZephyrHostlinkService::ZephyrHostlinkService(ports::zephyr::ZephyrBoardBackendConfig config)
    : config_(std::move(config)) {}

bool ZephyrHostlinkService::available() const {
    if (!zephyr_device_ready()) {
        return false;
    }
    if (config_.board_family == "lilygo_tlora_pager") {
        if (const auto* runtime = ports::zephyr::try_tlora_pager_board_runtime(); runtime != nullptr) {
            return runtime->hostlink_ready();
        }
    }
    return true;
}

bool ZephyrHostlinkService::connected() const {
    return available();
}

std::string ZephyrHostlinkService::transport_name() const {
    if (config_.board_family == "lilygo_tlora_pager") {
        if (const auto* runtime = ports::zephyr::try_tlora_pager_board_runtime(); runtime != nullptr) {
            return runtime->hostlink_ready() ? config_.hostlink_device_name : "hostlink-gated";
        }
    }
    return available() ? config_.hostlink_device_name : "hostlink-unavailable";
}

std::string ZephyrHostlinkService::bridge_name() const {
    return config_.hostlink_bridge_name;
}

bool ZephyrHostlinkService::zephyr_device_ready() const {
    if (config_.hostlink_device_name.empty()) {
        return false;
    }
    const auto* device = device_get_binding(config_.hostlink_device_name.c_str());
    return device != nullptr && device_is_ready(device);
}

}  // namespace aegis::services
