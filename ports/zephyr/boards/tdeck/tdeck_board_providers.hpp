#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>

#include <zephyr/kernel.h>
#include <zephyr/drivers/pwm.h>

#include "platform/logging/logger.hpp"
#include "ports/zephyr/zephyr_board_backend_config.hpp"

struct device;

namespace aegis::ports::zephyr {

enum class TdeckTransferParticipant : uint8_t {
    Unknown,
    Display,
    Radio,
    Storage,
};

enum class TdeckBoardControlParticipant : uint8_t {
    Unknown,
    PowerRail,
    Keyboard,
    Touch,
    Battery,
};

enum class TdeckPowerChannel : uint8_t {
    BoardPower,
    DisplayBacklight,
    KeyboardBacklight,
};

class ZephyrTdeckTransferCoordinator {
public:
    ZephyrTdeckTransferCoordinator(platform::Logger& logger, ZephyrBoardBackendConfig config);

    void bind_gpio_device(const struct device* gpio_device);
    [[nodiscard]] bool prepare();
    [[nodiscard]] bool ready() const;
    [[nodiscard]] TdeckTransferParticipant owner() const;
    [[nodiscard]] std::string owner_name() const;
    [[nodiscard]] static std::string coordinator_name();
    [[nodiscard]] int with_participant(TdeckTransferParticipant participant,
                                       k_timeout_t timeout,
                                       std::string_view operation,
                                       const std::function<int()>& action) const;

private:
    [[nodiscard]] bool lock_for(TdeckTransferParticipant participant, k_timeout_t timeout) const;
    void unlock_for(TdeckTransferParticipant participant) const;

    platform::Logger& logger_;
    ZephyrBoardBackendConfig config_;
    const struct device* gpio_device_ {nullptr};
    mutable k_mutex mutex_ {};
    mutable TdeckTransferParticipant owner_ {TdeckTransferParticipant::Unknown};
    bool ready_ {false};
};

class ZephyrTdeckBoardControlProvider {
public:
    ZephyrTdeckBoardControlProvider(platform::Logger& logger, ZephyrBoardBackendConfig config);

    void bind_gpio_device(const struct device* gpio_device);
    void bind_i2c_device(const struct device* i2c_device);
    void bind_adc_device(const struct device* adc_device);
    void bind_pwm_device(const struct device* pwm_device);
    [[nodiscard]] bool ready() const;
    [[nodiscard]] bool keyboard_ready() const;
    [[nodiscard]] bool touch_ready() const;
    [[nodiscard]] bool battery_ready() const;
    [[nodiscard]] TdeckBoardControlParticipant owner() const;
    [[nodiscard]] std::string owner_name() const;
    [[nodiscard]] static std::string coordinator_name();
    [[nodiscard]] int with_participant(TdeckBoardControlParticipant participant,
                                       k_timeout_t timeout,
                                       std::string_view operation,
                                       const std::function<int()>& action) const;
    [[nodiscard]] bool initialize_power_and_backlights();
    [[nodiscard]] bool probe_keyboard_controller();
    [[nodiscard]] bool probe_touch_controller();
    [[nodiscard]] bool probe_battery_controller();
    [[nodiscard]] bool start_keyboard_sampler();
    [[nodiscard]] bool keyboard_pending_character_count(uint8_t& pending) const;
    [[nodiscard]] bool keyboard_pop_character(uint8_t& raw_character) const;
    [[nodiscard]] bool keyboard_read_character(uint8_t& raw_character) const;
    [[nodiscard]] bool read_touch_point(int16_t& x, int16_t& y, bool& pressed) const;
    [[nodiscard]] std::optional<int> battery_percent() const;
    [[nodiscard]] std::optional<int> battery_voltage_mv() const;
    [[nodiscard]] bool battery_charging() const;
    [[nodiscard]] bool set_power_enabled(TdeckPowerChannel channel, bool enabled) const;
    [[nodiscard]] bool set_display_backlight_percent(uint8_t percent) const;
    [[nodiscard]] bool set_keyboard_backlight_level(uint8_t level) const;
    [[nodiscard]] bool power_enabled(TdeckPowerChannel channel) const;

private:
    [[nodiscard]] bool lock_for(TdeckBoardControlParticipant participant, k_timeout_t timeout) const;
    void unlock_for(TdeckBoardControlParticipant participant) const;
    [[nodiscard]] bool probe_i2c_read(uint16_t address,
                                      TdeckBoardControlParticipant participant,
                                      std::string_view operation) const;
    [[nodiscard]] bool probe_i2c_register(uint16_t address,
                                          uint8_t reg,
                                          TdeckBoardControlParticipant participant,
                                          std::string_view operation) const;
    [[nodiscard]] bool write_i2c_register(uint16_t address,
                                          uint8_t reg,
                                          uint8_t value,
                                          TdeckBoardControlParticipant participant,
                                          std::string_view operation) const;
    [[nodiscard]] bool probe_gt911(uint16_t address, std::string_view operation) const;
    [[nodiscard]] bool read_gt911_register(uint16_t address,
                                           uint16_t reg,
                                           uint8_t* buffer,
                                           std::size_t size,
                                           std::string_view operation) const;
    [[nodiscard]] bool read_gt911_register8(uint16_t address,
                                            uint16_t reg,
                                            uint8_t& value,
                                            std::string_view operation) const;
    [[nodiscard]] bool read_gt911_register16(uint16_t address,
                                             uint16_t reg,
                                             uint16_t& value,
                                             std::string_view operation) const;
    [[nodiscard]] bool read_i2c_register(uint16_t address,
                                         uint8_t reg,
                                         TdeckBoardControlParticipant participant,
                                         std::string_view operation,
                                         uint8_t& value) const;
    [[nodiscard]] bool read_i2c_register16(uint16_t address,
                                           uint8_t reg_high,
                                           TdeckBoardControlParticipant participant,
                                           std::string_view operation,
                                           uint16_t& value) const;
    [[nodiscard]] bool update_i2c_register_bits(uint16_t address,
                                                uint8_t reg,
                                                uint8_t mask,
                                                bool enabled,
                                                TdeckBoardControlParticipant participant,
                                                std::string_view operation) const;
    void pulse_touch_irq_wakeup() const;
    void log_touch_diagnostics(uint16_t address) const;
    [[nodiscard]] bool probe_battery_adc();
    [[nodiscard]] std::optional<int> read_battery_adc_mv() const;
    [[nodiscard]] int battery_percent_from_mv(int mv) const;
    [[nodiscard]] bool set_display_backlight_percent_unlocked(uint8_t percent) const;
    [[nodiscard]] bool set_keyboard_backlight_level_unlocked(uint8_t level) const;
    [[nodiscard]] bool read_keyboard_character_unlocked(uint8_t& raw_character) const;
    static void keyboard_sampler_thread_entry(void* p1, void* p2, void* p3);
    void sample_keyboard_once() const;

    platform::Logger& logger_;
    ZephyrBoardBackendConfig config_;
    const struct device* gpio_device_ {nullptr};
    const struct device* i2c_device_ {nullptr};
    const struct device* adc_device_ {nullptr};
    const struct device* pwm_device_ {nullptr};
    mutable k_mutex mutex_ {};
    mutable TdeckBoardControlParticipant owner_ {TdeckBoardControlParticipant::Unknown};
    bool keyboard_ready_ {false};
    bool touch_ready_ {false};
    bool battery_ready_ {false};
    bool battery_via_adc_ {false};
    uint16_t touch_address_ {0};
    mutable uint8_t last_touch_status_ {0xFF};
    mutable int last_touch_irq_level_ {-2};
    mutable uint32_t last_touch_poll_log_ms_ {0};
    mutable uint32_t last_touch_zero_count_dump_ms_ {0};
    mutable std::array<bool, 3> power_state_ {};
    mutable uint8_t display_backlight_percent_ {100};
    mutable uint8_t keyboard_backlight_level_ {127};
    mutable k_msgq keyboard_character_queue_ {};
    mutable std::array<uint8_t, 32> keyboard_character_queue_storage_ {};
    mutable k_thread keyboard_sampler_thread_ {};
    mutable k_tid_t keyboard_sampler_tid_ {nullptr};
    mutable bool keyboard_sampler_started_ {false};
};

}  // namespace aegis::ports::zephyr
