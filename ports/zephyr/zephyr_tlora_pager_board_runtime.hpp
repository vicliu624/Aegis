#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <string_view>

#include <zephyr/kernel.h>

#include "platform/logging/logger.hpp"
#include "ports/zephyr/zephyr_board_backend_config.hpp"
#include "ports/zephyr/zephyr_board_runtime.hpp"
#include "ports/zephyr/zephyr_tlora_pager_board_providers.hpp"

struct device;

namespace aegis::ports::zephyr {

class ZephyrTloraPagerBoardRuntime : public ZephyrBoardRuntime {
public:
    using CoordinationParticipant = TloraPagerTransferParticipant;
    using BoardControlParticipant = TloraPagerBoardControlParticipant;
    using PowerChannel = TloraPagerPowerChannel;

    ZephyrTloraPagerBoardRuntime(platform::Logger& logger, ZephyrBoardBackendConfig config);

    [[nodiscard]] const ZephyrBoardBackendConfig& config() const override;
    [[nodiscard]] bool initialize() override;
    [[nodiscard]] bool ready() const override;
    void log_state(std::string_view stage) const override;
    [[nodiscard]] int with_coordination_participant(CoordinationParticipant participant,
                                                    k_timeout_t timeout,
                                                    std::string_view operation,
                                                    const std::function<int()>& action) const;
    [[nodiscard]] int with_radio_participant(k_timeout_t timeout,
                                             std::string_view operation,
                                             const std::function<int()>& action) const;
    [[nodiscard]] int with_storage_participant(k_timeout_t timeout,
                                               std::string_view operation,
                                               const std::function<int()>& action) const;
    [[nodiscard]] int with_nfc_participant(k_timeout_t timeout,
                                           std::string_view operation,
                                           const std::function<int()>& action) const;
    [[nodiscard]] int with_board_control_participant(BoardControlParticipant participant,
                                                     k_timeout_t timeout,
                                                     std::string_view operation,
                                                     const std::function<int()>& action) const;
    [[nodiscard]] int read_i2c_reg(uint16_t address, uint8_t reg, uint8_t* value) const;
    [[nodiscard]] int write_i2c_reg(uint16_t address, uint8_t reg, uint8_t value) const;
    [[nodiscard]] bool keyboard_irq_asserted() const;
    [[nodiscard]] bool keyboard_pending_event_count(uint8_t& pending) const;
    [[nodiscard]] bool keyboard_read_event(uint8_t& raw_event) const;
    [[nodiscard]] bool set_power_enabled(PowerChannel channel, bool enabled) const;
    [[nodiscard]] bool set_display_backlight_enabled(bool enabled) const;
    [[nodiscard]] bool set_keyboard_backlight_enabled(bool enabled) const;
    void signal_boot_stage(int stage) const override;
    void heartbeat_pulse() const override;
    [[nodiscard]] bool peripheral_power_enabled(PowerChannel channel) const;
    [[nodiscard]] bool sd_card_present() const override;
    [[nodiscard]] bool expander_ready() const override;
    [[nodiscard]] bool coordination_domain_ready(ZephyrBoardCoordinationDomain domain) const override;
    [[nodiscard]] std::string coordination_domain_coordinator_name(
        ZephyrBoardCoordinationDomain domain) const override;
    [[nodiscard]] std::string coordination_domain_owner_name(
        ZephyrBoardCoordinationDomain domain) const override;
    [[nodiscard]] CoordinationParticipant coordination_participant_owner() const;
    [[nodiscard]] BoardControlParticipant board_control_participant_owner() const;
    [[nodiscard]] bool keyboard_ready() const override;
    [[nodiscard]] bool radio_ready() const override;
    [[nodiscard]] bool gps_ready() const override;
    [[nodiscard]] bool nfc_ready() const override;
    [[nodiscard]] bool storage_ready() const override;
    [[nodiscard]] bool audio_ready() const override;
    [[nodiscard]] bool hostlink_ready() const override;
    [[nodiscard]] ZephyrShellDisplayBackendProfile shell_display_backend_profile() const override;
    [[nodiscard]] ZephyrShellInputBackendProfile shell_input_backend_profile() const override;
    [[nodiscard]] int with_coordination_domain(ZephyrBoardCoordinationDomain domain,
                                               k_timeout_t timeout,
                                               std::string_view operation,
                                               const std::function<int()>& action) const override;

private:
    [[nodiscard]] bool acquire_devices();
    void configure_interrupt_and_backlight_lines();
    void probe_display_gpio_state() const;
    void refresh_storage_state(bool force_log, bool force_sample) const;

    platform::Logger& logger_;
    ZephyrBoardBackendConfig config_;
    const struct device* gpio_device_ {nullptr};
    const struct device* i2c_device_ {nullptr};
    mutable k_mutex storage_state_mutex_ {};
    bool initialized_ {false};
    mutable bool sd_card_present_cached_ {false};
    mutable bool storage_ready_cached_ {false};
    mutable bool storage_state_valid_ {false};
    mutable uint8_t storage_presence_disagree_streak_ {0};
    mutable int64_t last_storage_refresh_ms_ {0};
    ZephyrTloraPagerTransferCoordinator transfer_coordinator_;
    ZephyrTloraPagerBoardControlProvider board_control_provider_;
};

ZephyrTloraPagerBoardRuntime& tlora_pager_board_runtime(platform::Logger& logger);
ZephyrTloraPagerBoardRuntime* try_tlora_pager_board_runtime();
bool bootstrap_tlora_pager_board_runtime(platform::Logger& logger);

}  // namespace aegis::ports::zephyr
