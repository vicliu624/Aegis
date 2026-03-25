#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>

#include <zephyr/kernel.h>

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
    [[nodiscard]] bool keyboard_read_character(uint8_t& raw_character) const;
    [[nodiscard]] std::optional<int> battery_percent() const;
    [[nodiscard]] std::optional<int> battery_voltage_mv() const;
    [[nodiscard]] bool battery_charging() const;
    [[nodiscard]] bool set_power_enabled(TdeckPowerChannel channel, bool enabled) const;
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
    [[nodiscard]] bool probe_battery_adc();
    [[nodiscard]] std::optional<int> read_battery_adc_mv() const;
    [[nodiscard]] int battery_percent_from_mv(int mv) const;

    platform::Logger& logger_;
    ZephyrBoardBackendConfig config_;
    const struct device* gpio_device_ {nullptr};
    const struct device* i2c_device_ {nullptr};
    const struct device* adc_device_ {nullptr};
    mutable k_mutex mutex_ {};
    mutable TdeckBoardControlParticipant owner_ {TdeckBoardControlParticipant::Unknown};
    bool keyboard_ready_ {false};
    bool touch_ready_ {false};
    bool battery_ready_ {false};
    bool battery_via_adc_ {false};
    uint16_t touch_address_ {0};
    mutable std::array<bool, 3> power_state_ {};
};

}  // namespace aegis::ports::zephyr
