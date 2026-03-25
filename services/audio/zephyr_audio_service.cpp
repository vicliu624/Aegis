#include "services/audio/zephyr_audio_service.hpp"

#include <zephyr/device.h>

#include "ports/zephyr/zephyr_tlora_pager_board_runtime.hpp"

namespace aegis::services {

ZephyrAudioService::ZephyrAudioService(ports::zephyr::ZephyrBoardBackendConfig config)
    : config_(std::move(config)) {}

bool ZephyrAudioService::output_available() const {
    if (!ready(config_.audio_output_device_name)) {
        return false;
    }
    if (config_.board_family == "lilygo_tlora_pager") {
        if (const auto* runtime = ports::zephyr::try_tlora_pager_board_runtime(); runtime != nullptr) {
            return runtime->audio_ready();
        }
    }
    return true;
}

bool ZephyrAudioService::input_available() const {
    if (!ready(config_.audio_input_device_name)) {
        return false;
    }
    if (config_.board_family == "lilygo_tlora_pager") {
        if (const auto* runtime = ports::zephyr::try_tlora_pager_board_runtime(); runtime != nullptr) {
            return runtime->audio_ready();
        }
    }
    return true;
}

std::string ZephyrAudioService::backend_name() const {
    if (config_.board_family == "lilygo_tlora_pager") {
        if (const auto* runtime = ports::zephyr::try_tlora_pager_board_runtime(); runtime != nullptr) {
            return std::string("zephyr-pager-audio:out=") + config_.audio_output_device_name +
                   ",in=" + config_.audio_input_device_name +
                   ",amp=" + (runtime->audio_ready() ? "ready" : "gated") +
                   ",expander=" + (runtime->expander_ready() ? "ready" : "missing");
        }
    }
    return "zephyr-audio:out=" + config_.audio_output_device_name +
           ",in=" + config_.audio_input_device_name;
}

bool ZephyrAudioService::ready(const std::string& device_name) const {
    if (device_name.empty()) {
        return false;
    }
    const auto* device = device_get_binding(device_name.c_str());
    return device != nullptr && device_is_ready(device);
}

}  // namespace aegis::services
