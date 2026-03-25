#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <string_view>

#include <zephyr/kernel.h>

#include "platform/logging/logger.hpp"
#include "ports/zephyr/zephyr_board_backend_config.hpp"
#include "ports/zephyr/zephyr_board_runtime.hpp"

struct device;

namespace aegis::ports::zephyr {

class ZephyrTloraPagerBoardRuntime : public ZephyrBoardRuntime {
public:
    enum class SharedSpiClient : uint8_t {
        Unknown,
        Display,
        Radio,
        Storage,
        Nfc,
    };

    enum class PowerChannel : uint8_t {
        Radio,
        HapticDriver,
        Gps,
        Nfc,
        SdCard,
        SpeakerAmp,
        Keyboard,
        GpioDomain,
    };

    ZephyrTloraPagerBoardRuntime(platform::Logger& logger, ZephyrBoardBackendConfig config);

    [[nodiscard]] const ZephyrBoardBackendConfig& config() const override;
    [[nodiscard]] bool initialize() override;
    [[nodiscard]] bool ready() const override;
    void log_state(std::string_view stage) const override;
    [[nodiscard]] int with_shared_spi_client(SharedSpiClient client,
                                             k_timeout_t timeout,
                                             std::string_view operation,
                                             const std::function<int()>& action) const;
    [[nodiscard]] int with_radio_spi_client(k_timeout_t timeout,
                                            std::string_view operation,
                                            const std::function<int()>& action) const;
    [[nodiscard]] int with_storage_spi_client(k_timeout_t timeout,
                                              std::string_view operation,
                                              const std::function<int()>& action) const;
    [[nodiscard]] int with_nfc_spi_client(k_timeout_t timeout,
                                          std::string_view operation,
                                          const std::function<int()>& action) const;
    [[nodiscard]] bool lock_shared_spi_bus(k_timeout_t timeout) const;
    [[nodiscard]] bool lock_shared_spi_bus_for(SharedSpiClient client, k_timeout_t timeout) const;
    void unlock_shared_spi_bus() const;
    void unlock_shared_spi_bus_for(SharedSpiClient client) const;
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
    [[nodiscard]] bool shared_spi_ready() const override;
    [[nodiscard]] SharedSpiClient shared_spi_owner() const;
    [[nodiscard]] std::string shared_spi_owner_name() const override;
    [[nodiscard]] bool keyboard_ready() const override;
    [[nodiscard]] bool radio_ready() const override;
    [[nodiscard]] bool gps_ready() const override;
    [[nodiscard]] bool nfc_ready() const override;
    [[nodiscard]] bool storage_ready() const override;
    [[nodiscard]] bool audio_ready() const override;
    [[nodiscard]] bool hostlink_ready() const override;
    [[nodiscard]] bool board_direct_input_mode() const override;
    [[nodiscard]] int with_display_spi_client(k_timeout_t timeout,
                                              std::string_view operation,
                                              const std::function<int()>& action) const override;

private:
    class TloraPagerXl9555;

    [[nodiscard]] bool acquire_devices();
    [[nodiscard]] bool initialize_expander();
    void configure_expander_outputs();
    void configure_interrupt_and_backlight_lines();
    void prepare_shared_spi_bus();
    void probe_shared_spi_clients();
    void probe_keyboard_controller();
    void probe_expander_and_log() const;
    void probe_display_gpio_state() const;
    [[nodiscard]] bool read_sd_card_detect_raw(bool& present) const;
    void refresh_storage_state(bool force_log, bool force_sample) const;

    platform::Logger& logger_;
    ZephyrBoardBackendConfig config_;
    const struct device* gpio_device_ {nullptr};
    const struct device* i2c_device_ {nullptr};
    mutable k_mutex i2c_mutex_ {};
    mutable k_mutex shared_spi_mutex_ {};
    mutable SharedSpiClient shared_spi_owner_ {SharedSpiClient::Unknown};
    mutable k_mutex storage_state_mutex_ {};
    bool initialized_ {false};
    bool expander_ready_ {false};
    bool keyboard_ready_ {false};
    bool shared_spi_prepared_ {false};
    mutable std::array<bool, 8> power_channel_state_ {};
    mutable bool sd_card_present_cached_ {false};
    mutable bool storage_ready_cached_ {false};
    mutable bool storage_state_valid_ {false};
    mutable uint8_t storage_presence_disagree_streak_ {0};
    mutable int64_t last_storage_refresh_ms_ {0};
};

ZephyrTloraPagerBoardRuntime& tlora_pager_board_runtime(platform::Logger& logger);
ZephyrTloraPagerBoardRuntime* try_tlora_pager_board_runtime();
bool bootstrap_tlora_pager_board_runtime(platform::Logger& logger);

}  // namespace aegis::ports::zephyr
