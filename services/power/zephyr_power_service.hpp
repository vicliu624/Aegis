#pragma once

#include "ports/zephyr/zephyr_board_backend_config.hpp"
#include "services/common/service_interfaces.hpp"

namespace aegis::services {

class ZephyrPowerService : public IPowerService {
public:
    explicit ZephyrPowerService(ports::zephyr::ZephyrBoardBackendConfig config);

    [[nodiscard]] bool battery_present() const override;
    [[nodiscard]] bool low_power_mode_supported() const override;
    [[nodiscard]] std::string describe_status() const override;

private:
    ports::zephyr::ZephyrBoardBackendConfig config_;
};

}  // namespace aegis::services
