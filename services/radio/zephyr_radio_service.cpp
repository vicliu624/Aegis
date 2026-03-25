#include "services/radio/zephyr_radio_service.hpp"

#include <zephyr/device.h>

#include "ports/zephyr/zephyr_tlora_pager_board_runtime.hpp"

namespace aegis::services {

ZephyrRadioService::ZephyrRadioService(ports::zephyr::ZephyrBoardBackendConfig config)
    : config_(std::move(config)) {}

bool ZephyrRadioService::available() const {
    if (!zephyr_device_ready()) {
        return false;
    }
    if (config_.board_family == "lilygo_tlora_pager") {
        if (const auto* runtime = ports::zephyr::try_tlora_pager_board_runtime(); runtime != nullptr) {
            return runtime->radio_ready();
        }
    }
    return true;
}

std::string ZephyrRadioService::backend_name() const {
    if (config_.board_family == "lilygo_tlora_pager") {
        if (const auto* runtime = ports::zephyr::try_tlora_pager_board_runtime(); runtime != nullptr) {
            return std::string("zephyr-pager-radio:device=") + config_.radio_device_name +
                   ",power=" + (runtime->radio_ready() ? "ready" : "gated") +
                   ",shared-spi=" + (runtime->shared_spi_ready() ? "ready" : "missing") +
                   ",owner=" + (runtime->shared_spi_owner() ==
                                        ports::zephyr::ZephyrTloraPagerBoardRuntime::SharedSpiClient::Radio
                                    ? "radio"
                                    : "other") +
                   ",expander=" + (runtime->expander_ready() ? "ready" : "missing");
        }
    }
    return available() ? "zephyr-device:" + config_.radio_device_name
                       : "zephyr-device-absent:" + config_.radio_device_name;
}

bool ZephyrRadioService::zephyr_device_ready() const {
    const auto* device = device_get_binding(config_.radio_device_name.c_str());
    return device != nullptr && device_is_ready(device);
}

}  // namespace aegis::services
