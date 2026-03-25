#pragma once

#include <string>

#include "ports/zephyr/zephyr_board_backend_config.hpp"
#include "services/common/service_interfaces.hpp"

namespace aegis::services {

class ZephyrAudioService : public IAudioService {
public:
    explicit ZephyrAudioService(ports::zephyr::ZephyrBoardBackendConfig config);

    [[nodiscard]] bool output_available() const override;
    [[nodiscard]] bool input_available() const override;
    [[nodiscard]] std::string backend_name() const override;

private:
    [[nodiscard]] bool ready(const std::string& device_name) const;

    ports::zephyr::ZephyrBoardBackendConfig config_;
};

}  // namespace aegis::services
