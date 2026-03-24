#pragma once

#include <string>

#include "services/common/service_interfaces.hpp"

namespace aegis::services {

class ZephyrAudioService : public IAudioService {
public:
    ZephyrAudioService(std::string output_device_name, std::string input_device_name);

    [[nodiscard]] bool output_available() const override;
    [[nodiscard]] bool input_available() const override;
    [[nodiscard]] std::string backend_name() const override;

private:
    [[nodiscard]] bool ready(const std::string& device_name) const;

    std::string output_device_name_;
    std::string input_device_name_;
};

}  // namespace aegis::services
