#include "services/gps/zephyr_gps_service.hpp"

#include <zephyr/device.h>

#include "ports/zephyr/zephyr_tlora_pager_board_runtime.hpp"

namespace aegis::services {

ZephyrGpsService::ZephyrGpsService(ports::zephyr::ZephyrBoardBackendConfig config)
    : config_(std::move(config)) {}

bool ZephyrGpsService::available() const {
    if (!zephyr_device_ready()) {
        return false;
    }
    if (config_.board_family == "lilygo_tlora_pager") {
        if (const auto* runtime = ports::zephyr::try_tlora_pager_board_runtime(); runtime != nullptr) {
            return runtime->gps_ready();
        }
    }
    return true;
}

std::string ZephyrGpsService::backend_name() const {
    if (config_.board_family == "lilygo_tlora_pager") {
        if (const auto* runtime = ports::zephyr::try_tlora_pager_board_runtime(); runtime != nullptr) {
            return std::string("zephyr-pager-gps:device=") + config_.gps_device_name +
                   ",power=" + (runtime->gps_ready() ? "ready" : "gated") +
                   ",expander=" + (runtime->expander_ready() ? "ready" : "missing");
        }
    }
    return available() ? "zephyr-device:" + config_.gps_device_name
                       : "zephyr-device-absent:" + config_.gps_device_name;
}

bool ZephyrGpsService::zephyr_device_ready() const {
    if (config_.gps_device_name.empty()) {
        return false;
    }
    const auto* device = device_get_binding(config_.gps_device_name.c_str());
    return device != nullptr && device_is_ready(device);
}

}  // namespace aegis::services
