#pragma once

#include <string>

#include "ports/zephyr/zephyr_board_backend_config.hpp"
#include "services/common/service_interfaces.hpp"

namespace aegis::services {

class ZephyrGpsService : public IGpsService {
public:
    explicit ZephyrGpsService(ports::zephyr::ZephyrBoardBackendConfig config);

    [[nodiscard]] bool available() const override;
    [[nodiscard]] std::string backend_name() const override;

private:
    [[nodiscard]] bool zephyr_device_ready() const;

    ports::zephyr::ZephyrBoardBackendConfig config_;
};

}  // namespace aegis::services
