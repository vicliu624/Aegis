#pragma once

#include <functional>
#include <string_view>

#include <zephyr/kernel.h>

#include "platform/logging/logger.hpp"
#include "ports/zephyr/zephyr_board_backend_config.hpp"
#include "ports/zephyr/zephyr_board_runtime.hpp"
#include "ports/zephyr/zephyr_tdeck_board_providers.hpp"

struct device;

namespace aegis::ports::zephyr {

class ZephyrTdeckBoardRuntime : public ZephyrBoardRuntime {
public:
    using CoordinationParticipant = TdeckTransferParticipant;
    using BoardControlParticipant = TdeckBoardControlParticipant;
    using PowerChannel = TdeckPowerChannel;

    ZephyrTdeckBoardRuntime(platform::Logger& logger, ZephyrBoardBackendConfig config);

    [[nodiscard]] const ZephyrBoardBackendConfig& config() const override;
    [[nodiscard]] bool initialize() override;
    [[nodiscard]] bool ready() const override;
    void log_state(std::string_view stage) const override;
    void signal_boot_stage(int stage) const override;
    void heartbeat_pulse() const override;
    [[nodiscard]] bool coordination_domain_ready(ZephyrBoardCoordinationDomain domain) const override;
    [[nodiscard]] std::string coordination_domain_coordinator_name(ZephyrBoardCoordinationDomain domain) const override;
    [[nodiscard]] std::string coordination_domain_owner_name(ZephyrBoardCoordinationDomain domain) const override;
    [[nodiscard]] bool keyboard_ready() const override;
    [[nodiscard]] bool touch_ready() const override;
    [[nodiscard]] bool battery_ready() const override;
    [[nodiscard]] int battery_percent() const override;
    [[nodiscard]] int battery_voltage_mv() const override;
    [[nodiscard]] bool battery_charging() const override;
    [[nodiscard]] bool radio_ready() const override;
    [[nodiscard]] bool gps_ready() const override;
    [[nodiscard]] bool storage_ready() const override;
    [[nodiscard]] bool audio_ready() const override;
    [[nodiscard]] bool hostlink_ready() const override;
    [[nodiscard]] bool keyboard_read_character(uint8_t& raw_character) const override;
    [[nodiscard]] int with_coordination_domain(ZephyrBoardCoordinationDomain domain,
                                               k_timeout_t timeout,
                                               std::string_view operation,
                                               const std::function<int()>& action) const override;

private:
    [[nodiscard]] bool acquire_devices();
    void configure_trackball_pins();

    platform::Logger& logger_;
    ZephyrBoardBackendConfig config_;
    const struct device* gpio_device_ {nullptr};
    const struct device* i2c_device_ {nullptr};
    const struct device* adc_device_ {nullptr};
    bool initialized_ {false};
    bool touch_ready_ {false};
    bool battery_ready_ {false};
    ZephyrTdeckTransferCoordinator transfer_coordinator_;
    ZephyrTdeckBoardControlProvider board_control_provider_;
};

ZephyrTdeckBoardRuntime& tdeck_board_runtime(platform::Logger& logger);

}  // namespace aegis::ports::zephyr
