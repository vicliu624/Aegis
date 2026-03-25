#include "ports/zephyr/boards/tdeck/tdeck_board_providers.hpp"

#include <cerrno>
#include <iomanip>
#include <sstream>

#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/pwm.h>

#include "ports/zephyr/zephyr_gpio_pin.hpp"

namespace aegis::ports::zephyr {

namespace {

constexpr uint16_t kTdeckKeyboardAddress = 0x55;
constexpr uint16_t kTdeckPmuAddress = 0x34;
constexpr uint16_t kTdeckTouchAddressHigh = 0x14;
constexpr uint16_t kTdeckTouchAddressLow = 0x5D;
constexpr uint16_t kTdeckTouchProductIdRegister = 0x8140;
constexpr uint16_t kTdeckTouchFirmwareVersionRegister = 0x8144;
constexpr uint16_t kTdeckTouchXResolutionRegister = 0x8146;
constexpr uint16_t kTdeckTouchYResolutionRegister = 0x8148;
constexpr uint16_t kTdeckTouchVendorIdRegister = 0x814A;
constexpr uint16_t kTdeckTouchRefreshRateRegister = 0x8056;
constexpr uint16_t kTdeckTouchModuleSwitch1Register = 0x804D;
constexpr uint16_t kTdeckTouchStatusRegister = 0x814E;
constexpr uint16_t kTdeckTouchPoint1Register = 0x814F;
constexpr uint8_t kTdeckTouchDataReadyMask = 0x80;
constexpr uint8_t kTdeckTouchCountMask = 0x0F;
constexpr uint8_t kAxp2101ChipIdRegister = 0x03;
constexpr uint8_t kAxp2101ExpectedChipId = 0x4A;
constexpr uint8_t kAxp2101Status1Register = 0x00;
constexpr uint8_t kAxp2101Status2Register = 0x01;
constexpr uint8_t kAxp2101AdcChannelControlRegister = 0x30;
constexpr uint8_t kAxp2101BattDetectControlRegister = 0x68;
constexpr uint8_t kAxp2101BatteryPercentRegister = 0xA4;
constexpr uint8_t kAxp2101BattVoltageHighRegister = 0x34;
constexpr uint8_t kTdeckBatteryAdcChannel = 3;
constexpr uint8_t kTdeckBatteryAdcResolution = 12;
constexpr int kAdcInternalReferenceMv = 1100;
constexpr uint8_t kTdeckKeyboardBrightnessCmd = 0x01;
constexpr uint8_t kTdeckKeyboardDefaultBrightnessCmd = 0x02;
constexpr uint8_t kTdeckKeyboardModeKeyCmd = 0x04;
constexpr uint32_t kTdeckKeyboardBootDelayMs = 500;
constexpr uint32_t kTdeckDisplayBacklightPeriodNs = 1000000U;

std::string hex_u32(uint32_t value) {
    std::ostringstream stream;
    stream << std::hex << std::uppercase << value;
    return stream.str();
}

std::string format_ascii_bytes(const uint8_t* data, std::size_t size) {
    std::string text;
    text.reserve(size);
    for (std::size_t index = 0; index < size; ++index) {
        const char ch = static_cast<char>(data[index]);
        if (ch >= 32 && ch <= 126) {
            text.push_back(ch);
        }
    }
    return text.empty() ? "n/a" : text;
}

std::string format_hex_bytes(const uint8_t* data, std::size_t size) {
    std::ostringstream stream;
    for (std::size_t index = 0; index < size; ++index) {
        if (index != 0) {
            stream << ' ';
        }
        stream << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
               << static_cast<unsigned int>(data[index]);
    }
    return stream.str();
}

std::string_view transfer_participant_name(TdeckTransferParticipant participant) {
    switch (participant) {
        case TdeckTransferParticipant::Display: return "display";
        case TdeckTransferParticipant::Radio: return "radio";
        case TdeckTransferParticipant::Storage: return "storage";
        case TdeckTransferParticipant::Unknown:
        default: return "unknown";
    }
}

std::string_view board_control_participant_name(TdeckBoardControlParticipant participant) {
    switch (participant) {
        case TdeckBoardControlParticipant::PowerRail: return "power-rail";
        case TdeckBoardControlParticipant::Keyboard: return "keyboard";
        case TdeckBoardControlParticipant::Touch: return "touch";
        case TdeckBoardControlParticipant::Battery: return "battery";
        case TdeckBoardControlParticipant::Unknown:
        default: return "unknown";
    }
}

constexpr std::size_t power_channel_index(TdeckPowerChannel channel) {
    return static_cast<std::size_t>(channel);
}

bool write_gt911_register(const struct device* i2c_device, uint16_t address, uint16_t reg, uint8_t value) {
    uint8_t payload[] = {
        static_cast<uint8_t>((reg >> 8) & 0xFF),
        static_cast<uint8_t>(reg & 0xFF),
        value,
    };
    return i2c_write(i2c_device, payload, sizeof(payload), address) == 0;
}

void map_tdeck_touch_point(int display_width,
                           int display_height,
                           int16_t raw_x,
                           int16_t raw_y,
                           int16_t& mapped_x,
                           int16_t& mapped_y) {
    mapped_x = raw_y;
    mapped_y = static_cast<int16_t>(display_height - 1 - raw_x);

    if (mapped_x < 0) {
        mapped_x = 0;
    } else if (mapped_x >= display_width) {
        mapped_x = static_cast<int16_t>(display_width - 1);
    }

    if (mapped_y < 0) {
        mapped_y = 0;
    } else if (mapped_y >= display_height) {
        mapped_y = static_cast<int16_t>(display_height - 1);
    }
}

}  // namespace

ZephyrTdeckTransferCoordinator::ZephyrTdeckTransferCoordinator(platform::Logger& logger,
                                                               ZephyrBoardBackendConfig config)
    : logger_(logger), config_(std::move(config)) {
    k_mutex_init(&mutex_);
}

void ZephyrTdeckTransferCoordinator::bind_gpio_device(const struct device* gpio_device) {
    gpio_device_ = gpio_device;
}

bool ZephyrTdeckTransferCoordinator::prepare() {
    if (gpio_device_ == nullptr || !device_is_ready(gpio_device_)) {
        ready_ = false;
        return false;
    }
    for (const auto pin : config_.coordination_quiesce_pins) {
        if (pin < 0) {
            continue;
        }
        const auto pin_ref = resolve_gpio_pin(pin);
        if (pin_ref.device != nullptr && device_is_ready(pin_ref.device)) {
            const int cfg_rc = gpio_pin_configure(pin_ref.device, pin_ref.pin, GPIO_OUTPUT_ACTIVE);
            const int set_rc = gpio_pin_set(pin_ref.device, pin_ref.pin, 1);
            logger_.info("board",
                         "coordination quiesce pin=" + std::to_string(pin) +
                             " cfg_rc=" + std::to_string(cfg_rc) +
                             " set_rc=" + std::to_string(set_rc));
        }
    }
    ready_ = true;
    return true;
}

bool ZephyrTdeckTransferCoordinator::ready() const { return ready_; }
TdeckTransferParticipant ZephyrTdeckTransferCoordinator::owner() const { return owner_; }
std::string ZephyrTdeckTransferCoordinator::owner_name() const { return std::string(transfer_participant_name(owner_)); }
std::string ZephyrTdeckTransferCoordinator::coordinator_name() { return "tdeck-transfer-coordinator"; }

int ZephyrTdeckTransferCoordinator::with_participant(TdeckTransferParticipant participant,
                                                     k_timeout_t timeout,
                                                     std::string_view operation,
                                                     const std::function<int()>& action) const {
    if (action == nullptr) {
        return -EINVAL;
    }
    if (!lock_for(participant, timeout)) {
        return -EAGAIN;
    }
    const int rc = action();
    if (operation != "display.tdeck_write_pixels") {
        logger_.info("board",
                     "coordination op coordinator=" + coordinator_name() +
                         " owner=" + std::string(transfer_participant_name(participant)) +
                         " op=" + std::string(operation) + " rc=" + std::to_string(rc));
    }
    unlock_for(participant);
    return rc;
}

bool ZephyrTdeckTransferCoordinator::lock_for(TdeckTransferParticipant participant, k_timeout_t timeout) const {
    if (!ready_ || k_mutex_lock(&mutex_, timeout) != 0) {
        return false;
    }
    owner_ = participant;
    if (participant != TdeckTransferParticipant::Display) {
        logger_.info("board",
                     "coordination acquire coordinator=" + coordinator_name() +
                         " owner=" + std::string(transfer_participant_name(participant)));
    }
    return true;
}

void ZephyrTdeckTransferCoordinator::unlock_for(TdeckTransferParticipant participant) const {
    if (participant != TdeckTransferParticipant::Display) {
        logger_.info("board",
                     "coordination release coordinator=" + coordinator_name() +
                         " owner=" + std::string(transfer_participant_name(owner_)) +
                         " requested-by=" + std::string(transfer_participant_name(participant)));
    }
    owner_ = TdeckTransferParticipant::Unknown;
    (void)k_mutex_unlock(&mutex_);
}

ZephyrTdeckBoardControlProvider::ZephyrTdeckBoardControlProvider(platform::Logger& logger,
                                                                 ZephyrBoardBackendConfig config)
    : logger_(logger), config_(std::move(config)) {
    k_mutex_init(&mutex_);
    power_state_.fill(false);
}

void ZephyrTdeckBoardControlProvider::bind_gpio_device(const struct device* gpio_device) {
    gpio_device_ = gpio_device;
}

void ZephyrTdeckBoardControlProvider::bind_i2c_device(const struct device* i2c_device) {
    i2c_device_ = i2c_device;
}

void ZephyrTdeckBoardControlProvider::bind_adc_device(const struct device* adc_device) {
    adc_device_ = adc_device;
}

void ZephyrTdeckBoardControlProvider::bind_pwm_device(const struct device* pwm_device) {
    pwm_device_ = pwm_device;
}

bool ZephyrTdeckBoardControlProvider::ready() const {
    return i2c_device_ != nullptr && device_is_ready(i2c_device_);
}

bool ZephyrTdeckBoardControlProvider::keyboard_ready() const { return keyboard_ready_; }
bool ZephyrTdeckBoardControlProvider::touch_ready() const { return touch_ready_; }
bool ZephyrTdeckBoardControlProvider::battery_ready() const { return battery_ready_; }
TdeckBoardControlParticipant ZephyrTdeckBoardControlProvider::owner() const { return owner_; }
std::string ZephyrTdeckBoardControlProvider::owner_name() const { return std::string(board_control_participant_name(owner_)); }
std::string ZephyrTdeckBoardControlProvider::coordinator_name() { return "tdeck-board-control"; }

int ZephyrTdeckBoardControlProvider::with_participant(TdeckBoardControlParticipant participant,
                                                      k_timeout_t timeout,
                                                      std::string_view operation,
                                                      const std::function<int()>& action) const {
    if (action == nullptr) {
        return -EINVAL;
    }
    if (!lock_for(participant, timeout)) {
        return -EAGAIN;
    }
    const int rc = action();
    if (operation != "keyboard.read_char" && operation != "touch.read.status") {
        logger_.info("board",
                     "coordination op coordinator=" + coordinator_name() +
                         " owner=" + std::string(board_control_participant_name(participant)) +
                         " op=" + std::string(operation) + " rc=" + std::to_string(rc));
    }
    unlock_for(participant);
    return rc;
}

bool ZephyrTdeckBoardControlProvider::initialize_power_and_backlights() {
    bool ok = true;
    const auto configure_output = [&](int pin, bool enabled, TdeckPowerChannel channel) {
        if (pin < 0) {
            return;
        }
        const auto pin_ref = resolve_gpio_pin(pin);
        if (pin_ref.device == nullptr || !device_is_ready(pin_ref.device)) {
            ok = false;
            return;
        }
        const int cfg_rc = gpio_pin_configure(pin_ref.device, pin_ref.pin, GPIO_OUTPUT_ACTIVE);
        const int set_rc = gpio_pin_set(pin_ref.device, pin_ref.pin, enabled ? 1 : 0);
        power_state_[power_channel_index(channel)] = (cfg_rc == 0 && set_rc == 0 && enabled);
    };

    (void)with_participant(TdeckBoardControlParticipant::PowerRail,
                           K_MSEC(50),
                           "power.init",
                           [&]() {
                               configure_output(10, true, TdeckPowerChannel::BoardPower);
                               if (pwm_device_ != nullptr && device_is_ready(pwm_device_)) {
                                   power_state_[power_channel_index(TdeckPowerChannel::DisplayBacklight)] = true;
                                   ok = set_display_backlight_percent_unlocked(display_backlight_percent_) && ok;
                               } else {
                                   configure_output(
                                       config_.display_backlight_pin, true, TdeckPowerChannel::DisplayBacklight);
                               }
                               if (config_.keyboard_backlight_pin >= 0) {
                                   configure_output(config_.keyboard_backlight_pin,
                                                    true,
                                                    TdeckPowerChannel::KeyboardBacklight);
                               } else {
                                   power_state_[power_channel_index(TdeckPowerChannel::KeyboardBacklight)] = true;
                               }
                               k_sleep(K_MSEC(50));
                               return ok ? 0 : -EIO;
                           });
    return ok;
}

bool ZephyrTdeckBoardControlProvider::probe_keyboard_controller() {
    keyboard_ready_ = false;
    if (!ready()) {
        return false;
    }
    const uint32_t uptime_ms = k_uptime_get_32();
    if (uptime_ms < kTdeckKeyboardBootDelayMs) {
        k_sleep(K_MSEC(kTdeckKeyboardBootDelayMs - uptime_ms));
    }
    const bool probe_ok = probe_i2c_read(kTdeckKeyboardAddress,
                                         TdeckBoardControlParticipant::Keyboard,
                                         "keyboard.probe");
    if (!probe_ok) {
        return false;
    }

    uint8_t mode_cmd[] = {kTdeckKeyboardModeKeyCmd};
    uint8_t default_brightness[] = {kTdeckKeyboardDefaultBrightnessCmd, 127};
    uint8_t brightness[] = {kTdeckKeyboardBrightnessCmd, 127};
    (void)with_participant(TdeckBoardControlParticipant::Keyboard,
                           K_MSEC(50),
                           "keyboard.init",
                           [&]() {
                               (void)i2c_write(i2c_device_, mode_cmd, sizeof(mode_cmd), kTdeckKeyboardAddress);
                               (void)i2c_write(i2c_device_, default_brightness, sizeof(default_brightness), kTdeckKeyboardAddress);
                               (void)i2c_write(i2c_device_, brightness, sizeof(brightness), kTdeckKeyboardAddress);
                               return 0;
                           });
    keyboard_backlight_level_ = 127;
    power_state_[power_channel_index(TdeckPowerChannel::KeyboardBacklight)] = true;
    keyboard_ready_ = true;
    return true;
}

bool ZephyrTdeckBoardControlProvider::probe_touch_controller() {
    touch_address_ = 0;
    pulse_touch_irq_wakeup();
    if (probe_gt911(kTdeckTouchAddressHigh, "touch.probe.high")) {
        touch_address_ = kTdeckTouchAddressHigh;
    } else if (probe_gt911(kTdeckTouchAddressLow, "touch.probe.low")) {
        touch_address_ = kTdeckTouchAddressLow;
    }
    touch_ready_ = touch_address_ != 0;
    if (touch_ready_) {
        log_touch_diagnostics(touch_address_);
    } else {
        logger_.info("board", "touch probe failed on 0x14 and 0x5D");
    }
    return touch_ready_;
}

bool ZephyrTdeckBoardControlProvider::probe_battery_controller() {
    battery_via_adc_ = false;
    uint8_t chip_id = 0;
    if (!read_i2c_register(kTdeckPmuAddress,
                           kAxp2101ChipIdRegister,
                           TdeckBoardControlParticipant::Battery,
                           "battery.probe.chip_id",
                           chip_id)) {
        battery_ready_ = probe_battery_adc();
        battery_via_adc_ = battery_ready_;
        return battery_ready_;
    }

    if (chip_id != kAxp2101ExpectedChipId) {
        logger_.info("board",
                     "battery probe unexpected chip-id=" + std::to_string(chip_id) +
                         " expected=" + std::to_string(kAxp2101ExpectedChipId));
        battery_ready_ = probe_battery_adc();
        battery_via_adc_ = battery_ready_;
        return battery_ready_;
    }

    // Mirror the trail-mate T-Deck bring-up path so fuel-gauge and ADC-backed
    // battery telemetry are enabled on boards that expose them through AXP2101.
    const bool batt_detect_ok = update_i2c_register_bits(kTdeckPmuAddress,
                                                         kAxp2101BattDetectControlRegister,
                                                         0x01,
                                                         true,
                                                         TdeckBoardControlParticipant::Battery,
                                                         "battery.enable_detection");
    const bool adc_ok = update_i2c_register_bits(kTdeckPmuAddress,
                                                 kAxp2101AdcChannelControlRegister,
                                                 0x1D,
                                                 true,
                                                 TdeckBoardControlParticipant::Battery,
                                                 "battery.enable_adc");
    battery_ready_ = batt_detect_ok && adc_ok;
    if (!battery_ready_) {
        battery_ready_ = probe_battery_adc();
        battery_via_adc_ = battery_ready_;
    }
    return battery_ready_;
}

bool ZephyrTdeckBoardControlProvider::keyboard_read_character(uint8_t& raw_character) const {
    raw_character = 0;
    if (!keyboard_ready_ || !ready()) {
        return false;
    }
    int rc = -EAGAIN;
    (void)with_participant(TdeckBoardControlParticipant::Keyboard,
                           K_MSEC(50),
                           "keyboard.read_char",
                           [&]() {
                               rc = i2c_read(i2c_device_, &raw_character, 1, kTdeckKeyboardAddress);
                               return rc;
                           });
    return rc == 0 && raw_character != 0;
}

bool ZephyrTdeckBoardControlProvider::read_touch_point(int16_t& x, int16_t& y, bool& pressed) const {
    x = 0;
    y = 0;
    pressed = false;
    if (!touch_ready_ || !ready() || touch_address_ == 0) {
        return false;
    }

    const uint32_t now_ms = k_uptime_get_32();

    uint8_t status = 0;
    const uint8_t status_reg[2] = {
        static_cast<uint8_t>((kTdeckTouchStatusRegister >> 8) & 0xFF),
        static_cast<uint8_t>(kTdeckTouchStatusRegister & 0xFF),
    };
    int rc = -EAGAIN;
    (void)with_participant(TdeckBoardControlParticipant::Touch,
                           K_MSEC(20),
                           "touch.read.status",
                           [&]() {
                               rc = i2c_write_read(
                                   i2c_device_, touch_address_, status_reg, sizeof(status_reg), &status, sizeof(status));
                               return rc;
                           });
    if (rc != 0) {
        logger_.info("board", "touch status read failed rc=" + std::to_string(rc));
        return false;
    }

    int irq_level = -1;
    if (config_.touch_irq_pin >= 0) {
        const auto irq_ref = resolve_gpio_pin(config_.touch_irq_pin);
        if (irq_ref.device != nullptr && device_is_ready(irq_ref.device)) {
            irq_level = gpio_pin_get(irq_ref.device, irq_ref.pin);
        }
    }
    last_touch_poll_log_ms_ = now_ms;
    last_touch_status_ = status;
    last_touch_irq_level_ = irq_level;

    const uint8_t touch_count = static_cast<uint8_t>(status & kTdeckTouchCountMask);
    const bool ready_bit = (status & kTdeckTouchDataReadyMask) != 0;
    const bool irq_asserted = irq_level == 0;
    if (touch_count == 0 && !ready_bit && !irq_asserted) {
        return true;
    }

    uint8_t point[39] {};
    const uint8_t point_reg[2] = {
        static_cast<uint8_t>((kTdeckTouchPoint1Register >> 8) & 0xFF),
        static_cast<uint8_t>(kTdeckTouchPoint1Register & 0xFF),
    };
    rc = -EAGAIN;
    (void)with_participant(TdeckBoardControlParticipant::Touch,
                           K_MSEC(20),
                           "touch.read.point",
                           [&]() {
                               rc = i2c_write_read(
                                     i2c_device_, touch_address_, point_reg, sizeof(point_reg), point, sizeof(point));
                               if (rc == 0) {
                                   (void)write_gt911_register(
                                       i2c_device_, touch_address_, kTdeckTouchStatusRegister, 0x00);
                               }
                                 return rc;
                             });
    if (rc != 0) {
        logger_.info("board", "touch point read failed rc=" + std::to_string(rc));
        return false;
    }

    const uint8_t track_id = point[0];
    const int16_t raw_x = static_cast<int16_t>((static_cast<uint16_t>(point[2]) << 8) | point[1]);
    const int16_t raw_y = static_cast<int16_t>((static_cast<uint16_t>(point[4]) << 8) | point[3]);
    const uint16_t touch_size = static_cast<uint16_t>((static_cast<uint16_t>(point[6]) << 8) | point[5]);

    const bool coordinates_look_valid =
        raw_x >= 0 && raw_y >= 0 && raw_x < 512 && raw_y < 512 && !(raw_x == 0 && raw_y == 0 && track_id == 0);
    if (!coordinates_look_valid && !irq_asserted && touch_count == 0) {
        return true;
    }

    int16_t mapped_x = 0;
    int16_t mapped_y = 0;
    map_tdeck_touch_point(config_.display_width, config_.display_height, raw_x, raw_y, mapped_x, mapped_y);

    x = mapped_x;
    y = mapped_y;
    pressed = true;
    logger_.info("board",
                 "touch point track=" + std::to_string(track_id) +
                     " raw=" + std::to_string(raw_x) + "," + std::to_string(raw_y) +
                     " mapped=" + std::to_string(mapped_x) + "," + std::to_string(mapped_y) +
                     " size=" + std::to_string(touch_size) +
                     " status=0x" + hex_u32(status));
    return true;
}

std::optional<int> ZephyrTdeckBoardControlProvider::battery_percent() const {
    if (!battery_ready_) {
        return std::nullopt;
    }
    if (battery_via_adc_) {
        const auto mv = read_battery_adc_mv();
        return mv.has_value() ? std::optional<int>(battery_percent_from_mv(*mv)) : std::nullopt;
    }

    uint8_t status1 = 0;
    if (!read_i2c_register(kTdeckPmuAddress,
                           kAxp2101Status1Register,
                           TdeckBoardControlParticipant::Battery,
                           "battery.read_status1",
                           status1) ||
        (status1 & 0x08u) == 0u) {
        return std::nullopt;
    }

    uint8_t percent = 0;
    if (!read_i2c_register(kTdeckPmuAddress,
                           kAxp2101BatteryPercentRegister,
                           TdeckBoardControlParticipant::Battery,
                           "battery.read_percent",
                           percent)) {
        return std::nullopt;
    }
    if (percent > 100u) {
        return std::nullopt;
    }
    return static_cast<int>(percent);
}

std::optional<int> ZephyrTdeckBoardControlProvider::battery_voltage_mv() const {
    if (!battery_ready_) {
        return std::nullopt;
    }
    if (battery_via_adc_) {
        return read_battery_adc_mv();
    }

    uint8_t status1 = 0;
    if (!read_i2c_register(kTdeckPmuAddress,
                           kAxp2101Status1Register,
                           TdeckBoardControlParticipant::Battery,
                           "battery.read_status1",
                           status1) ||
        (status1 & 0x08u) == 0u) {
        return std::nullopt;
    }

    uint16_t raw = 0;
    if (!read_i2c_register16(kTdeckPmuAddress,
                             kAxp2101BattVoltageHighRegister,
                             TdeckBoardControlParticipant::Battery,
                             "battery.read_voltage",
                             raw)) {
        return std::nullopt;
    }
    return static_cast<int>(raw);
}

bool ZephyrTdeckBoardControlProvider::battery_charging() const {
    if (!battery_ready_) {
        return false;
    }
    if (battery_via_adc_) {
        return false;
    }

    uint8_t status2 = 0;
    if (!read_i2c_register(kTdeckPmuAddress,
                           kAxp2101Status2Register,
                           TdeckBoardControlParticipant::Battery,
                           "battery.read_status2",
                           status2)) {
        return false;
    }
    return ((status2 >> 5) & 0x03u) == 0x01u;
}

bool ZephyrTdeckBoardControlProvider::set_power_enabled(TdeckPowerChannel channel, bool enabled) const {
    if (channel == TdeckPowerChannel::DisplayBacklight) {
        return set_display_backlight_percent(enabled ? display_backlight_percent_ : 0);
    }
    if (channel == TdeckPowerChannel::KeyboardBacklight && config_.keyboard_backlight_pin < 0) {
        return set_keyboard_backlight_level(enabled ? keyboard_backlight_level_ : 0);
    }

    int pin = -1;
    if (channel == TdeckPowerChannel::DisplayBacklight) {
        pin = config_.display_backlight_pin;
    } else if (channel == TdeckPowerChannel::KeyboardBacklight) {
        pin = config_.keyboard_backlight_pin;
    } else if (channel == TdeckPowerChannel::BoardPower) {
        pin = 10;
    }
    if (pin < 0) {
        return false;
    }
    bool ok = false;
    (void)with_participant(TdeckBoardControlParticipant::PowerRail,
                           K_MSEC(50),
                           "power.set_channel",
                           [&]() {
                               const auto pin_ref = resolve_gpio_pin(pin);
                               if (pin_ref.device == nullptr || !device_is_ready(pin_ref.device)) {
                                   return -ENODEV;
                               }
                               ok = gpio_pin_set(pin_ref.device, pin_ref.pin, enabled ? 1 : 0) == 0;
                               return ok ? 0 : -EIO;
                           });
    if (ok) {
        power_state_[power_channel_index(channel)] = enabled;
    }
    return ok;
}

bool ZephyrTdeckBoardControlProvider::power_enabled(TdeckPowerChannel channel) const {
    return power_state_[power_channel_index(channel)];
}

bool ZephyrTdeckBoardControlProvider::set_display_backlight_percent(uint8_t percent) const {
    bool ok = false;
    (void)with_participant(TdeckBoardControlParticipant::PowerRail,
                           K_MSEC(50),
                           "power.set_display_backlight_percent",
                           [&]() {
                               ok = set_display_backlight_percent_unlocked(percent);
                               return ok ? 0 : -EIO;
                           });
    return ok;
}

bool ZephyrTdeckBoardControlProvider::set_keyboard_backlight_level(uint8_t level) const {
    bool ok = false;
    (void)with_participant(TdeckBoardControlParticipant::Keyboard,
                           K_MSEC(50),
                           "keyboard.set_backlight_level",
                           [&]() {
                               ok = set_keyboard_backlight_level_unlocked(level);
                               return ok ? 0 : -EIO;
                           });
    return ok;
}

bool ZephyrTdeckBoardControlProvider::set_display_backlight_percent_unlocked(uint8_t percent) const {
    display_backlight_percent_ = percent;
    if (pwm_device_ != nullptr && device_is_ready(pwm_device_)) {
        const uint32_t pulse_ns =
            (static_cast<uint32_t>(percent) * kTdeckDisplayBacklightPeriodNs) / 100U;
        const int rc = pwm_set(pwm_device_, 0, kTdeckDisplayBacklightPeriodNs, pulse_ns, 0);
        if (rc == 0) {
            power_state_[power_channel_index(TdeckPowerChannel::DisplayBacklight)] = percent > 0;
            return true;
        }
        logger_.info("board",
                     "display backlight pwm set failed rc=" + std::to_string(rc) +
                         " percent=" + std::to_string(percent));
    }

    if (config_.display_backlight_pin < 0) {
        return false;
    }
    const auto pin_ref = resolve_gpio_pin(config_.display_backlight_pin);
    if (pin_ref.device == nullptr || !device_is_ready(pin_ref.device)) {
        return false;
    }
    const bool enabled = percent > 0;
    const int rc = gpio_pin_set(pin_ref.device, pin_ref.pin, enabled ? 1 : 0);
    if (rc == 0) {
        power_state_[power_channel_index(TdeckPowerChannel::DisplayBacklight)] = enabled;
        return true;
    }
    return false;
}

bool ZephyrTdeckBoardControlProvider::set_keyboard_backlight_level_unlocked(uint8_t level) const {
    keyboard_backlight_level_ = level;
    if (keyboard_ready_ && ready()) {
        uint8_t payload[] = {kTdeckKeyboardBrightnessCmd, level};
        const int rc = i2c_write(i2c_device_, payload, sizeof(payload), kTdeckKeyboardAddress);
        if (rc != 0) {
            logger_.info("board",
                         "keyboard backlight write failed rc=" + std::to_string(rc) +
                             " level=" + std::to_string(level));
            return false;
        }
    } else if (config_.keyboard_backlight_pin >= 0) {
        const auto pin_ref = resolve_gpio_pin(config_.keyboard_backlight_pin);
        if (pin_ref.device == nullptr || !device_is_ready(pin_ref.device)) {
            return false;
        }
        const int rc = gpio_pin_set(pin_ref.device, pin_ref.pin, level > 0 ? 1 : 0);
        if (rc != 0) {
            return false;
        }
    }
    power_state_[power_channel_index(TdeckPowerChannel::KeyboardBacklight)] = level > 0;
    return true;
}

bool ZephyrTdeckBoardControlProvider::lock_for(TdeckBoardControlParticipant participant, k_timeout_t timeout) const {
    if (!ready() || k_mutex_lock(&mutex_, timeout) != 0) {
        return false;
    }
    owner_ = participant;
    if (participant != TdeckBoardControlParticipant::Keyboard &&
        participant != TdeckBoardControlParticipant::Touch) {
        logger_.info("board",
                     "coordination acquire coordinator=" + coordinator_name() +
                         " owner=" + std::string(board_control_participant_name(participant)));
    }
    return true;
}

void ZephyrTdeckBoardControlProvider::unlock_for(TdeckBoardControlParticipant participant) const {
    if (participant != TdeckBoardControlParticipant::Keyboard &&
        participant != TdeckBoardControlParticipant::Touch) {
        logger_.info("board",
                     "coordination release coordinator=" + coordinator_name() +
                         " owner=" + std::string(board_control_participant_name(owner_)) +
                         " requested-by=" + std::string(board_control_participant_name(participant)));
    }
    owner_ = TdeckBoardControlParticipant::Unknown;
    (void)k_mutex_unlock(&mutex_);
}

bool ZephyrTdeckBoardControlProvider::probe_i2c_read(uint16_t address,
                                                     TdeckBoardControlParticipant participant,
                                                     std::string_view operation) const {
    if (!ready()) {
        return false;
    }
    int rc = -EAGAIN;
    uint8_t probe = 0;
    (void)with_participant(participant,
                           K_MSEC(50),
                           operation,
                           [&]() {
                               rc = i2c_read(i2c_device_, &probe, sizeof(probe), address);
                               return rc;
                           });
    return rc == 0;
}

bool ZephyrTdeckBoardControlProvider::probe_i2c_register(uint16_t address,
                                                         uint8_t reg,
                                                         TdeckBoardControlParticipant participant,
                                                         std::string_view operation) const {
    if (!ready()) {
        return false;
    }
    int rc = -EAGAIN;
    uint8_t probe = 0;
    (void)with_participant(participant,
                           K_MSEC(50),
                           operation,
                           [&]() {
                               rc = i2c_write_read(i2c_device_, address, &reg, sizeof(reg), &probe, sizeof(probe));
                               return rc;
                           });
    return rc == 0;
}

bool ZephyrTdeckBoardControlProvider::write_i2c_register(uint16_t address,
                                                         uint8_t reg,
                                                         uint8_t value,
                                                         TdeckBoardControlParticipant participant,
                                                         std::string_view operation) const {
    if (!ready()) {
        return false;
    }
    int rc = -EAGAIN;
    uint8_t payload[] = {reg, value};
    (void)with_participant(participant,
                           K_MSEC(50),
                           operation,
                           [&]() {
                               rc = i2c_write(i2c_device_, payload, sizeof(payload), address);
                               return rc;
                           });
    return rc == 0;
}

bool ZephyrTdeckBoardControlProvider::probe_gt911(uint16_t address, std::string_view operation) const {
    if (!ready()) {
        return false;
    }
    int rc = -EAGAIN;
    uint8_t probe[4] {};
    const uint8_t reg[2] {
        static_cast<uint8_t>((kTdeckTouchProductIdRegister >> 8) & 0xFF),
        static_cast<uint8_t>(kTdeckTouchProductIdRegister & 0xFF),
    };
    (void)with_participant(TdeckBoardControlParticipant::Touch,
                           K_MSEC(50),
                           operation,
                           [&]() {
                               rc = i2c_write_read(i2c_device_, address, reg, sizeof(reg), probe, sizeof(probe));
                               return rc;
                           });
    if (rc == 0) {
        logger_.info("board",
                     "touch probe addr=0x" + hex_u32(address) +
                         " product-id-bytes=" + std::to_string(probe[0]) + "," + std::to_string(probe[1]) + "," +
                         std::to_string(probe[2]) + "," + std::to_string(probe[3]));
    }
    return rc == 0 && probe[0] >= '0' && probe[0] <= '9';
}

bool ZephyrTdeckBoardControlProvider::read_gt911_register(uint16_t address,
                                                          uint16_t reg,
                                                          uint8_t* buffer,
                                                          std::size_t size,
                                                          std::string_view operation) const {
    if (!ready() || buffer == nullptr || size == 0) {
        return false;
    }
    int rc = -EAGAIN;
    const uint8_t reg_bytes[2] = {
        static_cast<uint8_t>((reg >> 8) & 0xFF),
        static_cast<uint8_t>(reg & 0xFF),
    };
    (void)with_participant(TdeckBoardControlParticipant::Touch,
                           K_MSEC(50),
                           operation,
                           [&]() {
                               rc = i2c_write_read(i2c_device_, address, reg_bytes, sizeof(reg_bytes), buffer, size);
                               return rc;
                           });
    return rc == 0;
}

bool ZephyrTdeckBoardControlProvider::read_gt911_register8(uint16_t address,
                                                           uint16_t reg,
                                                           uint8_t& value,
                                                           std::string_view operation) const {
    return read_gt911_register(address, reg, &value, sizeof(value), operation);
}

bool ZephyrTdeckBoardControlProvider::read_gt911_register16(uint16_t address,
                                                            uint16_t reg,
                                                            uint16_t& value,
                                                            std::string_view operation) const {
    uint8_t bytes[2] {};
    if (!read_gt911_register(address, reg, bytes, sizeof(bytes), operation)) {
        value = 0;
        return false;
    }
    value = static_cast<uint16_t>(bytes[0] | (static_cast<uint16_t>(bytes[1]) << 8));
    return true;
}

bool ZephyrTdeckBoardControlProvider::read_i2c_register(uint16_t address,
                                                        uint8_t reg,
                                                        TdeckBoardControlParticipant participant,
                                                        std::string_view operation,
                                                        uint8_t& value) const {
    if (!ready()) {
        return false;
    }
    int rc = -EAGAIN;
    value = 0;
    (void)with_participant(participant,
                           K_MSEC(50),
                           operation,
                           [&]() {
                               rc = i2c_write_read(i2c_device_, address, &reg, sizeof(reg), &value, sizeof(value));
                               if (rc == 0) {
                                   return rc;
                               }
                               rc = i2c_write(i2c_device_, &reg, sizeof(reg), address);
                               if (rc != 0) {
                                   return rc;
                               }
                               rc = i2c_read(i2c_device_, &value, sizeof(value), address);
                               return rc;
                           });
    return rc == 0;
}

bool ZephyrTdeckBoardControlProvider::read_i2c_register16(uint16_t address,
                                                          uint8_t reg_high,
                                                          TdeckBoardControlParticipant participant,
                                                          std::string_view operation,
                                                          uint16_t& value) const {
    if (!ready()) {
        return false;
    }
    uint8_t bytes[2] {};
    int rc = -EAGAIN;
    (void)with_participant(participant,
                           K_MSEC(50),
                           operation,
                           [&]() {
                               rc = i2c_write_read(i2c_device_, address, &reg_high, sizeof(reg_high), bytes, sizeof(bytes));
                               if (rc == 0) {
                                   return rc;
                               }
                               rc = i2c_write(i2c_device_, &reg_high, sizeof(reg_high), address);
                               if (rc != 0) {
                                   return rc;
                               }
                               rc = i2c_read(i2c_device_, bytes, sizeof(bytes), address);
                               return rc;
                           });
    if (rc != 0) {
        value = 0;
        return false;
    }
    value = static_cast<uint16_t>(((bytes[0] & 0x3Fu) << 8) | bytes[1]);
    return true;
}

bool ZephyrTdeckBoardControlProvider::update_i2c_register_bits(uint16_t address,
                                                               uint8_t reg,
                                                               uint8_t mask,
                                                               bool enabled,
                                                               TdeckBoardControlParticipant participant,
                                                               std::string_view operation) const {
    uint8_t current = 0;
    if (!read_i2c_register(address, reg, participant, std::string(operation) + ".read", current)) {
        return false;
    }
    const uint8_t next = enabled ? static_cast<uint8_t>(current | mask)
                                 : static_cast<uint8_t>(current & ~mask);
    if (next == current) {
        return true;
    }
    return write_i2c_register(address, reg, next, participant, std::string(operation) + ".write");
}

void ZephyrTdeckBoardControlProvider::pulse_touch_irq_wakeup() const {
    if (config_.touch_irq_pin < 0) {
        return;
    }
    const auto irq_ref = resolve_gpio_pin(config_.touch_irq_pin);
    if (irq_ref.device == nullptr || !device_is_ready(irq_ref.device)) {
        return;
    }

    const int out_rc = gpio_pin_configure(irq_ref.device, irq_ref.pin, GPIO_OUTPUT_HIGH);
    const int set_rc = gpio_pin_set(irq_ref.device, irq_ref.pin, 1);
    k_msleep(8);
    const int in_rc = gpio_pin_configure(irq_ref.device, irq_ref.pin, GPIO_INPUT | GPIO_PULL_UP);
    logger_.info("board",
                 "touch irq wakeup pin=" + std::to_string(config_.touch_irq_pin) +
                     " cfg_out_rc=" + std::to_string(out_rc) +
                     " set_rc=" + std::to_string(set_rc) +
                     " cfg_in_rc=" + std::to_string(in_rc));
    k_msleep(20);
}

void ZephyrTdeckBoardControlProvider::log_touch_diagnostics(uint16_t address) const {
    uint8_t product_id[4] {};
    uint16_t fw_version = 0;
    uint16_t x_resolution = 0;
    uint16_t y_resolution = 0;
    uint8_t vendor_id = 0;
    uint8_t refresh_rate = 0;
    uint8_t module_switch_1 = 0;

    const bool product_ok =
        read_gt911_register(address, kTdeckTouchProductIdRegister, product_id, sizeof(product_id), "touch.diag.product");
    const bool fw_ok =
        read_gt911_register16(address, kTdeckTouchFirmwareVersionRegister, fw_version, "touch.diag.fw_version");
    const bool x_ok =
        read_gt911_register16(address, kTdeckTouchXResolutionRegister, x_resolution, "touch.diag.x_resolution");
    const bool y_ok =
        read_gt911_register16(address, kTdeckTouchYResolutionRegister, y_resolution, "touch.diag.y_resolution");
    const bool vendor_ok =
        read_gt911_register8(address, kTdeckTouchVendorIdRegister, vendor_id, "touch.diag.vendor");
    const bool refresh_ok =
        read_gt911_register8(address, kTdeckTouchRefreshRateRegister, refresh_rate, "touch.diag.refresh");
    const bool switch_ok =
        read_gt911_register8(address, kTdeckTouchModuleSwitch1Register, module_switch_1, "touch.diag.module_switch");

    std::string irq_mode = "unknown";
    if (switch_ok) {
        switch (module_switch_1 & 0x03) {
            case 0x00: irq_mode = "rising"; break;
            case 0x01: irq_mode = "falling"; break;
            case 0x02: irq_mode = "low-level-query"; break;
            case 0x03: irq_mode = "high-level-query"; break;
            default: break;
        }
    }

    logger_.info("board",
                 "touch diag addr=0x" + hex_u32(address) +
                     " product=" +
                     std::string(product_ok ? format_ascii_bytes(product_id, sizeof(product_id)) : "read-failed") +
                     " product-bytes=" +
                     std::string(product_ok ? format_hex_bytes(product_id, sizeof(product_id)) : "read-failed") +
                     " fw=" + std::string(fw_ok ? ("0x" + hex_u32(fw_version)) : "read-failed") +
                     " res=" +
                     (x_ok && y_ok ? (std::to_string(x_resolution) + "x" + std::to_string(y_resolution))
                                   : std::string("read-failed")) +
                     " vendor=" + std::string(vendor_ok ? std::to_string(vendor_id) : "read-failed") +
                     " refresh=" + std::string(refresh_ok ? std::to_string(refresh_rate + 5U) : "read-failed") +
                     " irq-mode=" + irq_mode +
                     " module-switch1=" + std::string(switch_ok ? ("0x" + hex_u32(module_switch_1)) : "read-failed"));
}

bool ZephyrTdeckBoardControlProvider::probe_battery_adc() {
    if (config_.battery_adc_pin < 0 || adc_device_ == nullptr || !device_is_ready(adc_device_)) {
        return false;
    }

    const adc_channel_cfg channel_cfg {
        .gain = ADC_GAIN_1_4,
        .reference = ADC_REF_INTERNAL,
        .acquisition_time = ADC_ACQ_TIME_DEFAULT,
        .channel_id = kTdeckBatteryAdcChannel,
#if defined(CONFIG_ADC_CONFIGURABLE_INPUTS)
        .input_positive = 0,
#endif
    };
    return adc_channel_setup(adc_device_, &channel_cfg) == 0;
}

std::optional<int> ZephyrTdeckBoardControlProvider::read_battery_adc_mv() const {
    if (!battery_via_adc_ || adc_device_ == nullptr || !device_is_ready(adc_device_)) {
        return std::nullopt;
    }

    int16_t sample = 0;
    adc_sequence sequence {
        .options = nullptr,
        .channels = BIT(kTdeckBatteryAdcChannel),
        .buffer = &sample,
        .buffer_size = sizeof(sample),
        .resolution = kTdeckBatteryAdcResolution,
        .oversampling = 0,
        .calibrate = false,
    };
    if (adc_read(adc_device_, &sequence) != 0) {
        return std::nullopt;
    }

    if (sample <= 0) {
        return std::nullopt;
    }

    constexpr int kAdcMaxSample = (1 << kTdeckBatteryAdcResolution) - 1;
    const int64_t sensed_mv =
        (static_cast<int64_t>(sample) * kAdcInternalReferenceMv * 4) / kAdcMaxSample;
    if (sensed_mv <= 0) {
        return std::nullopt;
    }

    return static_cast<int>(sensed_mv * 2);
}

int ZephyrTdeckBoardControlProvider::battery_percent_from_mv(int mv) const {
    if (mv <= 0) {
        return -1;
    }
    int percent = static_cast<int>(((mv - 3300) * 100) / 900);
    if (percent < 0) {
        percent = 0;
    }
    if (percent > 100) {
        percent = 100;
    }
    return percent;
}

}  // namespace aegis::ports::zephyr
