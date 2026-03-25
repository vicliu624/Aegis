#pragma once

#include <string>

#include "ports/zephyr/zephyr_board_backend_config.hpp"
#include "services/common/service_interfaces.hpp"

namespace aegis::services {

class ZephyrHostlinkService : public IHostlinkService {
public:
    explicit ZephyrHostlinkService(ports::zephyr::ZephyrBoardBackendConfig config);

    [[nodiscard]] bool available() const override;
    [[nodiscard]] bool connected() const override;
    [[nodiscard]] std::string transport_name() const override;
    [[nodiscard]] std::string bridge_name() const override;

private:
    [[nodiscard]] bool zephyr_device_ready() const;

    ports::zephyr::ZephyrBoardBackendConfig config_;
};

}  // namespace aegis::services
