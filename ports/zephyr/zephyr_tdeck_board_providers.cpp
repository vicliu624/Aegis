#include "ports/zephyr/zephyr_tdeck_board_providers.hpp"

#include <cerrno>

#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>

#include "ports/zephyr/zephyr_gpio_pin.hpp"

namespace aegis::ports::zephyr {

namespace {

constexpr uint16_t kTdeckKeyboardAddress = 0x55;
constexpr uint16_t kTdeckPmuAddress = 0x34;
constexpr uint16_t kTdeckTouchAddressHigh = 0x14;
constexpr uint16_t kTdeckTouchAddressLow = 0x5D;
constexpr uint16_t kTdeckTouchProductIdRegister = 0x8140;
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
constexpr uint8_t kTdeckKeyboardBrightnessCmd = 0x01;
constexpr uint8_t kTdeckKeyboardDefaultBrightnessCmd = 0x02;
constexpr uint8_t kTdeckKeyboardModeKeyCmd = 0x04;
constexpr uint32_t kTdeckKeyboardBootDelayMs = 500;

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
    if (operation != "keyboard.read_char") {
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
                               configure_output(config_.display_backlight_pin, true, TdeckPowerChannel::DisplayBacklight);
                               configure_output(config_.keyboard_backlight_pin, true, TdeckPowerChannel::KeyboardBacklight);
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
    keyboard_ready_ = true;
    return true;
}

bool ZephyrTdeckBoardControlProvider::probe_touch_controller() {
    touch_address_ = 0;
    if (probe_gt911(kTdeckTouchAddressHigh, "touch.probe.high")) {
        touch_address_ = kTdeckTouchAddressHigh;
    } else if (probe_gt911(kTdeckTouchAddressLow, "touch.probe.low")) {
        touch_address_ = kTdeckTouchAddressLow;
    }
    touch_ready_ = touch_address_ != 0;
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

bool ZephyrTdeckBoardControlProvider::lock_for(TdeckBoardControlParticipant participant, k_timeout_t timeout) const {
    if (!ready() || k_mutex_lock(&mutex_, timeout) != 0) {
        return false;
    }
    owner_ = participant;
    if (participant != TdeckBoardControlParticipant::Keyboard) {
        logger_.info("board",
                     "coordination acquire coordinator=" + coordinator_name() +
                         " owner=" + std::string(board_control_participant_name(participant)));
    }
    return true;
}

void ZephyrTdeckBoardControlProvider::unlock_for(TdeckBoardControlParticipant participant) const {
    if (participant != TdeckBoardControlParticipant::Keyboard) {
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
    return rc == 0 && probe[0] >= '0' && probe[0] <= '9';
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

    int32_t mv = sample;
    if (adc_raw_to_millivolts(adc_ref_internal(adc_device_),
                              ADC_GAIN_1_4,
                              kTdeckBatteryAdcResolution,
                              &mv) != 0 ||
        mv <= 0) {
        return std::nullopt;
    }

    return static_cast<int>(mv * 2);
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
