#pragma once

#include <string>

#include "ports/zephyr/zephyr_board_backend_config.hpp"
#include "services/common/service_interfaces.hpp"

namespace aegis::services {

class ZephyrDisplayService : public IDisplayService {
public:
    explicit ZephyrDisplayService(ports::zephyr::ZephyrBoardBackendConfig config);

    [[nodiscard]] DisplayInfo display_info() const override;
    [[nodiscard]] std::string describe_surface() const override;

private:
    [[nodiscard]] DisplayInfo fallback_info() const;

    ports::zephyr::ZephyrBoardBackendConfig config_;
};

class ZephyrInputService : public IInputService {
public:
    explicit ZephyrInputService(ports::zephyr::ZephyrBoardBackendConfig config);

    [[nodiscard]] InputInfo input_info() const override;
    [[nodiscard]] std::string describe_input_mode() const override;

private:
    [[nodiscard]] bool rotary_ready() const;
    [[nodiscard]] bool keyboard_ready() const;
    [[nodiscard]] bool pointer_ready() const;

    ports::zephyr::ZephyrBoardBackendConfig config_;
};

}  // namespace aegis::services
