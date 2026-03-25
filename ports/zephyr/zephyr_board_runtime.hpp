#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

#include <zephyr/kernel.h>

#include "ports/zephyr/zephyr_board_backend_config.hpp"

namespace aegis::ports::zephyr {

enum class ZephyrBoardCoordinationDomain : uint8_t {
    Unknown,
    DisplayPipeline,
    RadioSession,
    StorageSession,
    NfcSession,
    BoardControl,
};

enum class ZephyrShellDisplayBackendProfile : uint8_t {
    Generic,
    Pager,
    TDeck,
};

enum class ZephyrShellInputBackendProfile : uint8_t {
    Generic,
    PagerDirect,
    TDeckDirect,
};

class ZephyrBoardRuntime {
public:
    virtual ~ZephyrBoardRuntime() = default;

    [[nodiscard]] virtual const ZephyrBoardBackendConfig& config() const = 0;
    [[nodiscard]] virtual bool initialize() = 0;
    [[nodiscard]] virtual bool ready() const = 0;
    virtual void log_state(std::string_view stage) const = 0;
    virtual void signal_boot_stage(int stage) const = 0;
    virtual void heartbeat_pulse() const = 0;

    [[nodiscard]] virtual bool expander_ready() const;
    [[nodiscard]] virtual bool coordination_domain_ready(ZephyrBoardCoordinationDomain domain) const;
    [[nodiscard]] virtual std::string coordination_domain_coordinator_name(
        ZephyrBoardCoordinationDomain domain) const;
    [[nodiscard]] virtual std::string coordination_domain_owner_name(
        ZephyrBoardCoordinationDomain domain) const;
    [[nodiscard]] virtual bool keyboard_ready() const;
    [[nodiscard]] virtual bool touch_ready() const;
    [[nodiscard]] virtual bool battery_ready() const;
    [[nodiscard]] virtual int battery_percent() const;
    [[nodiscard]] virtual int battery_voltage_mv() const;
    [[nodiscard]] virtual bool battery_charging() const;
    [[nodiscard]] virtual bool radio_ready() const;
    [[nodiscard]] virtual bool gps_ready() const;
    [[nodiscard]] virtual bool nfc_ready() const;
    [[nodiscard]] virtual bool storage_ready() const;
    [[nodiscard]] virtual bool audio_ready() const;
    [[nodiscard]] virtual bool hostlink_ready() const;
    [[nodiscard]] virtual bool sd_card_present() const;
    [[nodiscard]] virtual bool set_display_backlight_enabled(bool enabled) const;
    [[nodiscard]] virtual bool set_display_brightness_percent(uint8_t percent) const;
    [[nodiscard]] virtual bool set_keyboard_backlight_enabled(bool enabled) const;
    [[nodiscard]] virtual bool set_bluetooth_enabled(bool enabled);
    [[nodiscard]] virtual bool bluetooth_enabled() const;
    [[nodiscard]] virtual bool set_gps_enabled(bool enabled);
    [[nodiscard]] virtual bool gps_enabled() const;

    [[nodiscard]] virtual ZephyrShellDisplayBackendProfile shell_display_backend_profile() const;
    [[nodiscard]] virtual ZephyrShellInputBackendProfile shell_input_backend_profile() const;
    [[nodiscard]] virtual bool keyboard_irq_asserted() const;
    [[nodiscard]] virtual bool keyboard_pending_event_count(uint8_t& pending) const;
    [[nodiscard]] virtual bool keyboard_read_event(uint8_t& raw_event) const;
    [[nodiscard]] virtual bool keyboard_read_character(uint8_t& raw_character) const;
    [[nodiscard]] virtual bool touch_read_point(int16_t& x, int16_t& y, bool& pressed) const;
    [[nodiscard]] virtual int with_coordination_domain(
        ZephyrBoardCoordinationDomain domain,
        k_timeout_t timeout,
        std::string_view operation,
        const std::function<int()>& action) const;
};

class ZephyrGenericBoardRuntime final : public ZephyrBoardRuntime {
public:
    explicit ZephyrGenericBoardRuntime(ZephyrBoardBackendConfig config);

    [[nodiscard]] const ZephyrBoardBackendConfig& config() const override;
    [[nodiscard]] bool initialize() override;
    [[nodiscard]] bool ready() const override;
    void log_state(std::string_view stage) const override;
    void signal_boot_stage(int stage) const override;
    void heartbeat_pulse() const override;

private:
    ZephyrBoardBackendConfig config_;
    bool initialized_ {false};
};

ZephyrBoardRuntime& generic_zephyr_board_runtime(std::string_view package_id);
void set_active_zephyr_board_runtime(ZephyrBoardRuntime* runtime);
[[nodiscard]] ZephyrBoardRuntime* try_active_zephyr_board_runtime();
[[nodiscard]] ZephyrBoardRuntime& require_active_zephyr_board_runtime();

}  // namespace aegis::ports::zephyr
