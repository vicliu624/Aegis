#include "services/audio/zephyr_audio_service.hpp"

#include <zephyr/device.h>

namespace aegis::services {

ZephyrAudioService::ZephyrAudioService(std::string output_device_name, std::string input_device_name)
    : output_device_name_(std::move(output_device_name)),
      input_device_name_(std::move(input_device_name)) {}

bool ZephyrAudioService::output_available() const {
    return ready(output_device_name_);
}

bool ZephyrAudioService::input_available() const {
    return ready(input_device_name_);
}

std::string ZephyrAudioService::backend_name() const {
    return "zephyr-audio:out=" + output_device_name_ + ",in=" + input_device_name_;
}

bool ZephyrAudioService::ready(const std::string& device_name) const {
    if (device_name.empty()) {
        return false;
    }
    const auto* device = device_get_binding(device_name.c_str());
    return device != nullptr && device_is_ready(device);
}

}  // namespace aegis::services
