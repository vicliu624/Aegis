#pragma once

#include <cstdint>
#include <string_view>

#include "platform/logging/logger.hpp"
#include "ports/zephyr/zephyr_board_backend_config.hpp"

struct device;

namespace aegis::ports::zephyr {

class ZephyrTloraPagerBoardRuntime {
public:
    ZephyrTloraPagerBoardRuntime(platform::Logger& logger, ZephyrBoardBackendConfig config);

    [[nodiscard]] bool initialize();
    [[nodiscard]] bool ready() const;
    void log_state(std::string_view stage) const;

private:
    class TloraPagerXl9555;

    [[nodiscard]] bool acquire_devices();
    [[nodiscard]] bool initialize_expander();
    void configure_expander_outputs();
    void configure_interrupt_and_backlight_lines();
    void prepare_shared_spi_bus();
    void probe_keyboard_controller();
    void probe_expander_and_log() const;
    void probe_display_gpio_state() const;

    platform::Logger& logger_;
    ZephyrBoardBackendConfig config_;
    const struct device* gpio_device_ {nullptr};
    const struct device* i2c_device_ {nullptr};
    bool initialized_ {false};
    bool expander_ready_ {false};
    bool keyboard_ready_ {false};
    bool shared_spi_prepared_ {false};
};

ZephyrTloraPagerBoardRuntime& tlora_pager_board_runtime(platform::Logger& logger);
bool bootstrap_tlora_pager_board_runtime(platform::Logger& logger);

}  // namespace aegis::ports::zephyr
