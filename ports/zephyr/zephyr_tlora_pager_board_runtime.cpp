#include "ports/zephyr/zephyr_tlora_pager_board_runtime.hpp"

#include <array>
#include <cerrno>
#include <optional>
#include <string>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>

#include "ports/zephyr/zephyr_gpio_pin.hpp"

namespace aegis::ports::zephyr {

namespace {
ZephyrTloraPagerBoardRuntime* g_tlora_pager_runtime = nullptr;
}  // namespace

namespace {

constexpr std::size_t kPowerChannelCount = 8;
constexpr int64_t kStorageRefreshIntervalMs = 250;
constexpr uint8_t kStoragePresenceCommitSamples = 3;

void pulse_gpio_pin(int pin, int pulses, int on_ms, int off_ms) {
    if (pin < 0) {
        return;
    }

    const auto pin_ref = resolve_gpio_pin(pin);
    if (pin_ref.device == nullptr || !device_is_ready(pin_ref.device)) {
        return;
    }

    (void)gpio_pin_configure(pin_ref.device, pin_ref.pin, GPIO_OUTPUT_ACTIVE);
    (void)gpio_pin_set(pin_ref.device, pin_ref.pin, 1);
    for (int index = 0; index < pulses; ++index) {
        (void)gpio_pin_set(pin_ref.device, pin_ref.pin, 0);
        k_sleep(K_MSEC(on_ms));
        (void)gpio_pin_set(pin_ref.device, pin_ref.pin, 1);
        k_sleep(K_MSEC(off_ms));
    }
}

constexpr uint8_t kXl9555Address = 0x20;
constexpr uint8_t kXl9555RegInput0 = 0x00;
constexpr uint8_t kXl9555RegInput1 = 0x01;
constexpr uint8_t kXl9555RegOutput0 = 0x02;
constexpr uint8_t kXl9555RegOutput1 = 0x03;
constexpr uint8_t kXl9555RegPolarity0 = 0x04;
constexpr uint8_t kXl9555RegPolarity1 = 0x05;
constexpr uint8_t kXl9555RegConfig0 = 0x06;
constexpr uint8_t kXl9555RegConfig1 = 0x07;

constexpr uint8_t kTca8418Address = 0x34;
constexpr uint8_t kTca8418RegKeyLockEventCount = 0x03;
constexpr uint8_t kTca8418RegKeyEventA = 0x04;

constexpr uint8_t kExpandDrvEn = 0;
constexpr uint8_t kExpandAmpEn = 1;
constexpr uint8_t kExpandKbRst = 2;
constexpr uint8_t kExpandLoraEn = 3;
constexpr uint8_t kExpandGpsEn = 4;
constexpr uint8_t kExpandNfcEn = 5;
constexpr uint8_t kExpandGpsRst = 7;
constexpr uint8_t kExpandKbEn = 8;
constexpr uint8_t kExpandGpioEn = 9;
constexpr uint8_t kExpandSdDet = 10;
constexpr uint8_t kExpandSdPullen = 11;
constexpr uint8_t kExpandSdEn = 12;

std::string hex_byte(uint8_t value) {
    static constexpr char kHex[] = "0123456789ABCDEF";
    std::string result = "0x";
    result.push_back(kHex[(value >> 4) & 0x0F]);
    result.push_back(kHex[value & 0x0F]);
    return result;
}

std::optional<uint8_t> expander_pin_for_channel(ZephyrTloraPagerBoardRuntime::PowerChannel channel) {
    switch (channel) {
        case ZephyrTloraPagerBoardRuntime::PowerChannel::Radio:
            return kExpandLoraEn;
        case ZephyrTloraPagerBoardRuntime::PowerChannel::HapticDriver:
            return kExpandDrvEn;
        case ZephyrTloraPagerBoardRuntime::PowerChannel::Gps:
            return kExpandGpsEn;
        case ZephyrTloraPagerBoardRuntime::PowerChannel::Nfc:
            return kExpandNfcEn;
        case ZephyrTloraPagerBoardRuntime::PowerChannel::SdCard:
            return kExpandSdEn;
        case ZephyrTloraPagerBoardRuntime::PowerChannel::SpeakerAmp:
            return kExpandAmpEn;
        case ZephyrTloraPagerBoardRuntime::PowerChannel::Keyboard:
            return kExpandKbEn;
        case ZephyrTloraPagerBoardRuntime::PowerChannel::GpioDomain:
            return kExpandGpioEn;
    }
    return std::nullopt;
}

constexpr std::size_t power_channel_index(ZephyrTloraPagerBoardRuntime::PowerChannel channel) {
    return static_cast<std::size_t>(channel);
}

std::string_view shared_spi_client_name(ZephyrTloraPagerBoardRuntime::SharedSpiClient client) {
    switch (client) {
        case ZephyrTloraPagerBoardRuntime::SharedSpiClient::Unknown:
            return "unknown";
        case ZephyrTloraPagerBoardRuntime::SharedSpiClient::Display:
            return "display";
        case ZephyrTloraPagerBoardRuntime::SharedSpiClient::Radio:
            return "radio";
        case ZephyrTloraPagerBoardRuntime::SharedSpiClient::Storage:
            return "storage";
        case ZephyrTloraPagerBoardRuntime::SharedSpiClient::Nfc:
            return "nfc";
    }
    return "unknown";
}

}  // namespace

class ZephyrTloraPagerBoardRuntime::TloraPagerXl9555 {
public:
    TloraPagerXl9555(const struct device* bus, uint16_t address)
        : bus_(bus), address_(address) {}

    [[nodiscard]] bool begin() const {
        uint8_t value = 0;
        return read_reg(kXl9555RegInput0, value);
    }

    [[nodiscard]] bool pin_mode(uint8_t pin, bool output) const {
        const uint8_t reg = pin >= 8 ? kXl9555RegConfig1 : kXl9555RegConfig0;
        const uint8_t bit = pin >= 8 ? static_cast<uint8_t>(pin - 8) : pin;
        return update_bit(reg, bit, output ? 0U : 1U);
    }

    [[nodiscard]] bool digital_write(uint8_t pin, bool high) const {
        const uint8_t reg = pin >= 8 ? kXl9555RegOutput1 : kXl9555RegOutput0;
        const uint8_t bit = pin >= 8 ? static_cast<uint8_t>(pin - 8) : pin;
        return update_bit(reg, bit, high ? 1U : 0U);
    }

    [[nodiscard]] bool digital_read(uint8_t pin, uint8_t& value) const {
        const uint8_t reg = pin >= 8 ? kXl9555RegInput1 : kXl9555RegInput0;
        const uint8_t bit = pin >= 8 ? static_cast<uint8_t>(pin - 8) : pin;
        uint8_t raw = 0;
        if (!read_reg(reg, raw)) {
            return false;
        }
        value = static_cast<uint8_t>((raw >> bit) & 0x01U);
        return true;
    }

    [[nodiscard]] bool read_reg(uint8_t reg, uint8_t& value) const {
        if (bus_ == nullptr || !device_is_ready(bus_)) {
            return false;
        }
        return i2c_reg_read_byte(bus_, address_, reg, &value) == 0;
    }

private:
    [[nodiscard]] bool write_reg(uint8_t reg, uint8_t value) const {
        if (bus_ == nullptr || !device_is_ready(bus_)) {
            return false;
        }
        return i2c_reg_write_byte(bus_, address_, reg, value) == 0;
    }

    [[nodiscard]] bool update_bit(uint8_t reg, uint8_t bit, uint8_t value) const {
        uint8_t current = 0;
        if (!read_reg(reg, current)) {
            return false;
        }
        const uint8_t mask = static_cast<uint8_t>(1U << bit);
        const uint8_t next = value != 0 ? static_cast<uint8_t>(current | mask)
                                        : static_cast<uint8_t>(current & ~mask);
        return write_reg(reg, next);
    }

    const struct device* bus_ {nullptr};
    uint16_t address_ {0};
};

ZephyrTloraPagerBoardRuntime::ZephyrTloraPagerBoardRuntime(platform::Logger& logger,
                                                           ZephyrBoardBackendConfig config)
    : logger_(logger), config_(std::move(config)) {
    k_mutex_init(&i2c_mutex_);
    k_mutex_init(&shared_spi_mutex_);
    k_mutex_init(&storage_state_mutex_);
    g_tlora_pager_runtime = this;
}

const ZephyrBoardBackendConfig& ZephyrTloraPagerBoardRuntime::config() const {
    return config_;
}

bool ZephyrTloraPagerBoardRuntime::initialize() {
    logger_.info("board", "pager runtime begin");
    const bool devices_ready = acquire_devices();
    const bool expander_ok = initialize_expander();
    configure_interrupt_and_backlight_lines();
    prepare_shared_spi_bus();
    probe_shared_spi_clients();
    probe_keyboard_controller();
    probe_expander_and_log();
    probe_display_gpio_state();

    initialized_ = devices_ready;
    refresh_storage_state(true, true);
    logger_.info("board",
                 "pager runtime complete devices=" + std::string(devices_ready ? "ready" : "degraded") +
                     " expander=" + std::string(expander_ok ? "ready" : "missing") +
                     " keyboard-probe=" + std::string(keyboard_ready_ ? "ready" : "missing") +
                     " shared-spi=" + std::string(shared_spi_prepared_ ? "ready" : "missing"));
    return devices_ready;
}

bool ZephyrTloraPagerBoardRuntime::ready() const {
    return initialized_;
}

void ZephyrTloraPagerBoardRuntime::log_state(std::string_view stage) const {
    refresh_storage_state(false, false);
    logger_.info("board",
                 "pager runtime stage=" + std::string(stage) +
                     " expander=" + std::string(expander_ready_ ? "ready" : "missing") +
                     " keyboard=" + std::string(keyboard_ready_ ? "ready" : "missing") +
                     " shared-spi=" + std::string(shared_spi_prepared_ ? "ready" : "missing") +
                     " owner=" + std::string(shared_spi_client_name(shared_spi_owner_)) +
                     " radio=" + std::string(radio_ready() ? "ready" : "gated") +
                     " gps=" + std::string(gps_ready() ? "ready" : "gated") +
                     " storage=" + std::string(storage_ready() ? "ready" : "gated"));
}

int ZephyrTloraPagerBoardRuntime::with_shared_spi_client(SharedSpiClient client,
                                                         k_timeout_t timeout,
                                                         std::string_view operation,
                                                         const std::function<int()>& action) const {
    if (action == nullptr) {
        return -EINVAL;
    }
    if (!lock_shared_spi_bus_for(client, timeout)) {
        logger_.info("board",
                     "shared spi transaction denied owner=" + std::string(shared_spi_client_name(client)) +
                         " op=" + std::string(operation));
        return -EAGAIN;
    }

    const int rc = action();
    logger_.info("board",
                 "shared spi transaction owner=" + std::string(shared_spi_client_name(client)) +
                     " op=" + std::string(operation) + " rc=" + std::to_string(rc));
    unlock_shared_spi_bus_for(client);
    return rc;
}

int ZephyrTloraPagerBoardRuntime::with_radio_spi_client(k_timeout_t timeout,
                                                        std::string_view operation,
                                                        const std::function<int()>& action) const {
    return with_shared_spi_client(SharedSpiClient::Radio, timeout, operation, action);
}

int ZephyrTloraPagerBoardRuntime::with_storage_spi_client(k_timeout_t timeout,
                                                          std::string_view operation,
                                                          const std::function<int()>& action) const {
    return with_shared_spi_client(SharedSpiClient::Storage, timeout, operation, action);
}

int ZephyrTloraPagerBoardRuntime::with_nfc_spi_client(k_timeout_t timeout,
                                                      std::string_view operation,
                                                      const std::function<int()>& action) const {
    return with_shared_spi_client(SharedSpiClient::Nfc, timeout, operation, action);
}

bool ZephyrTloraPagerBoardRuntime::lock_shared_spi_bus(k_timeout_t timeout) const {
    return lock_shared_spi_bus_for(SharedSpiClient::Unknown, timeout);
}

bool ZephyrTloraPagerBoardRuntime::lock_shared_spi_bus_for(SharedSpiClient client,
                                                           k_timeout_t timeout) const {
    if (!shared_spi_prepared_) {
        return false;
    }
    if (k_mutex_lock(&shared_spi_mutex_, timeout) != 0) {
        return false;
    }
    shared_spi_owner_ = client;
    logger_.info("board",
                 "shared spi acquire owner=" + std::string(shared_spi_client_name(client)));
    return true;
}

void ZephyrTloraPagerBoardRuntime::unlock_shared_spi_bus() const {
    unlock_shared_spi_bus_for(SharedSpiClient::Unknown);
}

void ZephyrTloraPagerBoardRuntime::unlock_shared_spi_bus_for(SharedSpiClient client) const {
    logger_.info("board",
                 "shared spi release owner=" +
                     std::string(shared_spi_client_name(shared_spi_owner_)) +
                     " requested-by=" + std::string(shared_spi_client_name(client)));
    shared_spi_owner_ = SharedSpiClient::Unknown;
    (void)k_mutex_unlock(&shared_spi_mutex_);
}

int ZephyrTloraPagerBoardRuntime::read_i2c_reg(uint16_t address, uint8_t reg, uint8_t* value) const {
    if (value == nullptr || i2c_device_ == nullptr || !device_is_ready(i2c_device_)) {
        return -ENODEV;
    }
    if (k_mutex_lock(&i2c_mutex_, K_MSEC(50)) != 0) {
        return -EAGAIN;
    }
    const int rc = i2c_reg_read_byte(i2c_device_, address, reg, value);
    (void)k_mutex_unlock(&i2c_mutex_);
    return rc;
}

int ZephyrTloraPagerBoardRuntime::write_i2c_reg(uint16_t address, uint8_t reg, uint8_t value) const {
    if (i2c_device_ == nullptr || !device_is_ready(i2c_device_)) {
        return -ENODEV;
    }
    if (k_mutex_lock(&i2c_mutex_, K_MSEC(50)) != 0) {
        return -EAGAIN;
    }
    const int rc = i2c_reg_write_byte(i2c_device_, address, reg, value);
    (void)k_mutex_unlock(&i2c_mutex_);
    return rc;
}

bool ZephyrTloraPagerBoardRuntime::keyboard_irq_asserted() const {
    if (config_.keyboard_irq_pin < 0) {
        return false;
    }
    const auto pin_ref = resolve_gpio_pin(config_.keyboard_irq_pin);
    if (pin_ref.device == nullptr || !device_is_ready(pin_ref.device)) {
        return false;
    }
    return gpio_pin_get(pin_ref.device, pin_ref.pin) == 0;
}

bool ZephyrTloraPagerBoardRuntime::keyboard_pending_event_count(uint8_t& pending) const {
    pending = 0;
    if (!keyboard_ready_) {
        return false;
    }
    return read_i2c_reg(kTca8418Address, kTca8418RegKeyLockEventCount, &pending) == 0;
}

bool ZephyrTloraPagerBoardRuntime::keyboard_read_event(uint8_t& raw_event) const {
    raw_event = 0;
    if (!keyboard_ready_) {
        return false;
    }
    return read_i2c_reg(kTca8418Address, kTca8418RegKeyEventA, &raw_event) == 0;
}

bool ZephyrTloraPagerBoardRuntime::set_power_enabled(PowerChannel channel, bool enabled) const {
    if (!expander_ready_ || i2c_device_ == nullptr || !device_is_ready(i2c_device_)) {
        return false;
    }
    const auto pin = expander_pin_for_channel(channel);
    if (!pin.has_value()) {
        return false;
    }
    if (k_mutex_lock(&i2c_mutex_, K_MSEC(50)) != 0) {
        return false;
    }
    const TloraPagerXl9555 expander(i2c_device_, kXl9555Address);
    const bool ok = expander.digital_write(*pin, enabled);
    (void)k_mutex_unlock(&i2c_mutex_);
    if (ok && power_channel_index(channel) < power_channel_state_.size()) {
        power_channel_state_[power_channel_index(channel)] = enabled;
    }
    if (ok && channel == PowerChannel::SdCard) {
        refresh_storage_state(true, true);
    }
    logger_.info("board",
                 "power channel=" + std::to_string(static_cast<int>(channel)) +
                     " enabled=" + std::string(enabled ? "1" : "0") +
                     " rc=" + std::string(ok ? "ok" : "failed"));
    return ok;
}

bool ZephyrTloraPagerBoardRuntime::set_display_backlight_enabled(bool enabled) const {
    if (config_.display_backlight_pin < 0) {
        return false;
    }
    const auto pin_ref = resolve_gpio_pin(config_.display_backlight_pin);
    if (pin_ref.device == nullptr || !device_is_ready(pin_ref.device)) {
        return false;
    }
    const int rc = gpio_pin_set(pin_ref.device, pin_ref.pin, enabled ? 1 : 0);
    logger_.info("board",
                 "display backlight enabled=" + std::string(enabled ? "1" : "0") +
                     " rc=" + std::to_string(rc));
    return rc == 0;
}

bool ZephyrTloraPagerBoardRuntime::set_keyboard_backlight_enabled(bool enabled) const {
    if (config_.keyboard_backlight_pin < 0) {
        return false;
    }
    const auto pin_ref = resolve_gpio_pin(config_.keyboard_backlight_pin);
    if (pin_ref.device == nullptr || !device_is_ready(pin_ref.device)) {
        return false;
    }
    const int rc = gpio_pin_set(pin_ref.device, pin_ref.pin, enabled ? 1 : 0);
    logger_.info("board",
                 "keyboard backlight enabled=" + std::string(enabled ? "1" : "0") +
                     " rc=" + std::to_string(rc));
    return rc == 0;
}

void ZephyrTloraPagerBoardRuntime::signal_boot_stage(int stage) const {
    if (stage <= 0) {
        return;
    }
    pulse_gpio_pin(config_.display_backlight_pin, stage, 70, 90);
}

void ZephyrTloraPagerBoardRuntime::heartbeat_pulse() const {
    pulse_gpio_pin(config_.keyboard_backlight_pin, 1, 35, 35);
}

bool ZephyrTloraPagerBoardRuntime::peripheral_power_enabled(PowerChannel channel) const {
    const auto index = power_channel_index(channel);
    return index < power_channel_state_.size() ? power_channel_state_[index] : false;
}

bool ZephyrTloraPagerBoardRuntime::sd_card_present() const {
    refresh_storage_state(false, false);
    return sd_card_present_cached_;
}

bool ZephyrTloraPagerBoardRuntime::expander_ready() const {
    return expander_ready_;
}

bool ZephyrTloraPagerBoardRuntime::shared_spi_ready() const {
    return shared_spi_prepared_;
}

ZephyrTloraPagerBoardRuntime::SharedSpiClient ZephyrTloraPagerBoardRuntime::shared_spi_owner() const {
    return shared_spi_owner_;
}

std::string ZephyrTloraPagerBoardRuntime::shared_spi_owner_name() const {
    return std::string(shared_spi_client_name(shared_spi_owner_));
}

bool ZephyrTloraPagerBoardRuntime::keyboard_ready() const {
    return keyboard_ready_;
}

bool ZephyrTloraPagerBoardRuntime::radio_ready() const {
    return ready() && expander_ready_ && shared_spi_prepared_ &&
           peripheral_power_enabled(PowerChannel::Radio);
}

bool ZephyrTloraPagerBoardRuntime::gps_ready() const {
    return ready() && expander_ready_ && peripheral_power_enabled(PowerChannel::Gps);
}

bool ZephyrTloraPagerBoardRuntime::nfc_ready() const {
    return ready() && expander_ready_ && shared_spi_prepared_ &&
           peripheral_power_enabled(PowerChannel::Nfc);
}

bool ZephyrTloraPagerBoardRuntime::storage_ready() const {
    refresh_storage_state(false, false);
    return storage_ready_cached_;
}

bool ZephyrTloraPagerBoardRuntime::audio_ready() const {
    return ready() && expander_ready_ &&
           peripheral_power_enabled(PowerChannel::SpeakerAmp);
}

bool ZephyrTloraPagerBoardRuntime::hostlink_ready() const {
    return ready();
}

bool ZephyrTloraPagerBoardRuntime::board_direct_input_mode() const {
    return true;
}

int ZephyrTloraPagerBoardRuntime::with_display_spi_client(
    k_timeout_t timeout,
    std::string_view operation,
    const std::function<int()>& action) const {
    return with_shared_spi_client(SharedSpiClient::Display, timeout, operation, action);
}

bool ZephyrTloraPagerBoardRuntime::acquire_devices() {
    gpio_device_ = DEVICE_DT_GET(DT_NODELABEL(gpio0));
#if DT_NODE_EXISTS(DT_NODELABEL(i2c0))
    i2c_device_ = DEVICE_DT_GET(DT_NODELABEL(i2c0));
#endif

    const bool gpio_ready = gpio_device_ != nullptr && device_is_ready(gpio_device_);
    const bool i2c_ready = i2c_device_ != nullptr && device_is_ready(i2c_device_);
    logger_.info("board",
                 "pager device acquisition gpio=" + std::string(gpio_ready ? "ready" : "missing") +
                     " i2c=" + std::string(i2c_ready ? "ready" : "missing"));
    return gpio_ready;
}

bool ZephyrTloraPagerBoardRuntime::initialize_expander() {
    if (i2c_device_ == nullptr || !device_is_ready(i2c_device_)) {
        logger_.info("board", "xl9555 skipped: i2c unavailable");
        expander_ready_ = false;
        return false;
    }

    const TloraPagerXl9555 expander(i2c_device_, kXl9555Address);
    if (!expander.begin()) {
        logger_.info("board", "xl9555 probe failed addr=" + hex_byte(kXl9555Address));
        expander_ready_ = false;
        return false;
    }

    uint8_t polarity = 0;
    (void)expander.read_reg(kXl9555RegPolarity0, polarity);
    (void)expander.read_reg(kXl9555RegPolarity1, polarity);
    (void)i2c_reg_write_byte(i2c_device_, kXl9555Address, kXl9555RegPolarity0, 0x00);
    (void)i2c_reg_write_byte(i2c_device_, kXl9555Address, kXl9555RegPolarity1, 0x00);

    expander_ready_ = true;
    configure_expander_outputs();
    k_sleep(K_MSEC(10));
    logger_.info("board", "xl9555 ready addr=" + hex_byte(kXl9555Address));
    return true;
}

void ZephyrTloraPagerBoardRuntime::configure_expander_outputs() {
    if (!expander_ready_) {
        return;
    }

    const TloraPagerXl9555 expander(i2c_device_, kXl9555Address);
    const std::array<uint8_t, 10> enable_lines {
        kExpandKbRst,
        kExpandLoraEn,
        kExpandGpsEn,
        kExpandDrvEn,
        kExpandAmpEn,
        kExpandNfcEn,
        kExpandGpsRst,
        kExpandKbEn,
        kExpandGpioEn,
        kExpandSdEn,
    };

    for (const auto pin : enable_lines) {
        const bool mode_ok = expander.pin_mode(pin, true);
        const bool write_ok = expander.digital_write(pin, true);
        logger_.info("board",
                     "xl9555 enable pin=" + std::to_string(pin) +
                         " mode=" + (mode_ok ? std::string("ok") : std::string("failed")) +
                         " write=" + (write_ok ? std::string("ok") : std::string("failed")));
        k_sleep(K_MSEC(1));
    }

    power_channel_state_.fill(false);
    power_channel_state_[power_channel_index(PowerChannel::HapticDriver)] = true;
    power_channel_state_[power_channel_index(PowerChannel::SpeakerAmp)] = true;
    power_channel_state_[power_channel_index(PowerChannel::Radio)] = true;
    power_channel_state_[power_channel_index(PowerChannel::Gps)] = true;
    power_channel_state_[power_channel_index(PowerChannel::Nfc)] = true;
    power_channel_state_[power_channel_index(PowerChannel::Keyboard)] = true;
    power_channel_state_[power_channel_index(PowerChannel::GpioDomain)] = true;
    power_channel_state_[power_channel_index(PowerChannel::SdCard)] = true;

    (void)expander.pin_mode(kExpandSdPullen, false);
    (void)expander.pin_mode(kExpandSdDet, false);
}

void ZephyrTloraPagerBoardRuntime::configure_interrupt_and_backlight_lines() {
    if (gpio_device_ == nullptr || !device_is_ready(gpio_device_)) {
        return;
    }

    if (config_.display_backlight_pin >= 0) {
        const auto pin_ref = resolve_gpio_pin(config_.display_backlight_pin);
        if (pin_ref.device != nullptr && device_is_ready(pin_ref.device)) {
            const int cfg_rc = gpio_pin_configure(pin_ref.device, pin_ref.pin, GPIO_OUTPUT_ACTIVE);
            const int set_rc = gpio_pin_set(pin_ref.device, pin_ref.pin, 1);
            logger_.info("board",
                         "display backlight pin=" + std::to_string(config_.display_backlight_pin) +
                             " cfg_rc=" + std::to_string(cfg_rc) +
                             " set_rc=" + std::to_string(set_rc));
        }
    }
    if (config_.keyboard_backlight_pin >= 0) {
        const auto pin_ref = resolve_gpio_pin(config_.keyboard_backlight_pin);
        if (pin_ref.device != nullptr && device_is_ready(pin_ref.device)) {
            const int cfg_rc = gpio_pin_configure(pin_ref.device, pin_ref.pin, GPIO_OUTPUT_ACTIVE);
            const int set_rc = gpio_pin_set(pin_ref.device, pin_ref.pin, 1);
            logger_.info("board",
                         "keyboard backlight pin=" + std::to_string(config_.keyboard_backlight_pin) +
                             " cfg_rc=" + std::to_string(cfg_rc) +
                             " set_rc=" + std::to_string(set_rc));
        }
    }
    if (config_.keyboard_irq_pin >= 0) {
        const auto pin_ref = resolve_gpio_pin(config_.keyboard_irq_pin);
        if (pin_ref.device != nullptr && device_is_ready(pin_ref.device)) {
            (void)gpio_pin_configure(pin_ref.device, pin_ref.pin, GPIO_INPUT | GPIO_PULL_UP);
        }
    }
    if (config_.rotary_center_pin >= 0) {
        const auto pin_ref = resolve_gpio_pin(config_.rotary_center_pin);
        if (pin_ref.device != nullptr && device_is_ready(pin_ref.device)) {
            (void)gpio_pin_configure(pin_ref.device, pin_ref.pin, GPIO_INPUT | GPIO_PULL_UP);
        }
    }
}

void ZephyrTloraPagerBoardRuntime::prepare_shared_spi_bus() {
    if (gpio_device_ == nullptr || !device_is_ready(gpio_device_)) {
        shared_spi_prepared_ = false;
        return;
    }

    for (const auto pin : config_.shared_spi_quiesce_pins) {
        if (pin < 0) {
            continue;
        }
        const auto pin_ref = resolve_gpio_pin(pin);
        if (pin_ref.device != nullptr && device_is_ready(pin_ref.device)) {
            const int cfg_rc = gpio_pin_configure(pin_ref.device, pin_ref.pin, GPIO_OUTPUT_ACTIVE);
            const int set_rc = gpio_pin_set(pin_ref.device, pin_ref.pin, 1);
            logger_.info("board",
                         "shared spi pin=" + std::to_string(pin) +
                             " cfg_rc=" + std::to_string(cfg_rc) +
                             " set_rc=" + std::to_string(set_rc));
        }
    }

    if (config_.display_cs_pin >= 0) {
        const auto pin_ref = resolve_gpio_pin(config_.display_cs_pin);
        if (pin_ref.device != nullptr && device_is_ready(pin_ref.device)) {
            const int cfg_rc = gpio_pin_configure(pin_ref.device, pin_ref.pin, GPIO_OUTPUT_ACTIVE);
            const int set_rc = gpio_pin_set(pin_ref.device, pin_ref.pin, 1);
            logger_.info("board",
                         "display cs pin=" + std::to_string(config_.display_cs_pin) +
                             " cfg_rc=" + std::to_string(cfg_rc) +
                             " set_rc=" + std::to_string(set_rc));
        }
    }
    if (config_.display_dc_pin >= 0) {
        const auto pin_ref = resolve_gpio_pin(config_.display_dc_pin);
        if (pin_ref.device != nullptr && device_is_ready(pin_ref.device)) {
            const int cfg_rc = gpio_pin_configure(pin_ref.device, pin_ref.pin, GPIO_OUTPUT_ACTIVE);
            const int set_rc = gpio_pin_set(pin_ref.device, pin_ref.pin, 1);
            logger_.info("board",
                         "display dc pin=" + std::to_string(config_.display_dc_pin) +
                             " cfg_rc=" + std::to_string(cfg_rc) +
                             " set_rc=" + std::to_string(set_rc));
        }
    }

    k_sleep(K_MSEC(20));
    shared_spi_prepared_ = true;
}

void ZephyrTloraPagerBoardRuntime::probe_shared_spi_clients() {
    if (!shared_spi_prepared_) {
        return;
    }

    if (peripheral_power_enabled(PowerChannel::Radio)) {
        (void)with_radio_spi_client(K_MSEC(20), "radio.probe.quiesce", []() { return 0; });
    }
    if (peripheral_power_enabled(PowerChannel::SdCard)) {
        (void)with_storage_spi_client(K_MSEC(20), "storage.probe.quiesce", []() { return 0; });
    }
    if (peripheral_power_enabled(PowerChannel::Nfc)) {
        (void)with_nfc_spi_client(K_MSEC(20), "nfc.probe.quiesce", []() { return 0; });
    }
}

void ZephyrTloraPagerBoardRuntime::probe_keyboard_controller() {
    if (i2c_device_ == nullptr || !device_is_ready(i2c_device_)) {
        keyboard_ready_ = false;
        return;
    }

    uint8_t pending = 0;
    keyboard_ready_ = read_i2c_reg(kTca8418Address, kTca8418RegKeyLockEventCount, &pending) == 0;
    logger_.info("board",
                 "tca8418 probe " + std::string(keyboard_ready_ ? "ok" : "failed") +
                     " addr=" + hex_byte(kTca8418Address) +
                     (keyboard_ready_ ? " events=" + hex_byte(pending) : ""));
}

void ZephyrTloraPagerBoardRuntime::probe_expander_and_log() const {
    if (!expander_ready_ || i2c_device_ == nullptr || !device_is_ready(i2c_device_)) {
        return;
    }

    const TloraPagerXl9555 expander(i2c_device_, kXl9555Address);
    uint8_t out0 = 0;
    uint8_t out1 = 0;
    uint8_t cfg0 = 0;
    uint8_t cfg1 = 0;
    uint8_t in0 = 0;
    uint8_t in1 = 0;
    (void)expander.read_reg(kXl9555RegOutput0, out0);
    (void)expander.read_reg(kXl9555RegOutput1, out1);
    (void)expander.read_reg(kXl9555RegConfig0, cfg0);
    (void)expander.read_reg(kXl9555RegConfig1, cfg1);
    (void)expander.read_reg(kXl9555RegInput0, in0);
    (void)expander.read_reg(kXl9555RegInput1, in1);

    logger_.info("board",
                 "xl9555 state out0=" + hex_byte(out0) + " out1=" + hex_byte(out1) +
                     " cfg0=" + hex_byte(cfg0) + " cfg1=" + hex_byte(cfg1) +
                     " in0=" + hex_byte(in0) + " in1=" + hex_byte(in1));
}

void ZephyrTloraPagerBoardRuntime::probe_display_gpio_state() const {
    if (gpio_device_ == nullptr || !device_is_ready(gpio_device_)) {
        return;
    }

    const auto bl_ref = resolve_gpio_pin(config_.display_backlight_pin);
    const auto cs_ref = resolve_gpio_pin(config_.display_cs_pin);
    const auto dc_ref = resolve_gpio_pin(config_.display_dc_pin);
    const int backlight =
        (bl_ref.device != nullptr && device_is_ready(bl_ref.device)) ? gpio_pin_get(bl_ref.device, bl_ref.pin) : -1;
    const int display_cs =
        (cs_ref.device != nullptr && device_is_ready(cs_ref.device)) ? gpio_pin_get(cs_ref.device, cs_ref.pin) : -1;
    const int display_dc =
        (dc_ref.device != nullptr && device_is_ready(dc_ref.device)) ? gpio_pin_get(dc_ref.device, dc_ref.pin) : -1;
    logger_.info("board",
                 "display gpio state bl=" + std::to_string(backlight) +
                     " cs=" + std::to_string(display_cs) +
                     " dc=" + std::to_string(display_dc));
}

bool ZephyrTloraPagerBoardRuntime::read_sd_card_detect_raw(bool& present) const {
    present = false;
    if (!expander_ready_ || i2c_device_ == nullptr || !device_is_ready(i2c_device_)) {
        return false;
    }

    uint8_t value = 1;
    if (k_mutex_lock(&i2c_mutex_, K_MSEC(50)) != 0) {
        return false;
    }
    const TloraPagerXl9555 expander(i2c_device_, kXl9555Address);
    const bool ok = expander.digital_read(kExpandSdDet, value);
    (void)k_mutex_unlock(&i2c_mutex_);
    if (!ok) {
        return false;
    }

    present = value == 0;
    return true;
}

void ZephyrTloraPagerBoardRuntime::refresh_storage_state(bool force_log, bool force_sample) const {
    if (k_mutex_lock(&storage_state_mutex_, K_MSEC(50)) != 0) {
        return;
    }

    const bool power_enabled = peripheral_power_enabled(PowerChannel::SdCard);
    const bool transport_ready = ready() && expander_ready_ && shared_spi_prepared_ && power_enabled;
    const int64_t now = k_uptime_get();
    if (!force_sample && storage_state_valid_ &&
        (now - last_storage_refresh_ms_) < kStorageRefreshIntervalMs) {
        (void)k_mutex_unlock(&storage_state_mutex_);
        return;
    }

    const bool old_present = sd_card_present_cached_;
    const bool old_ready = storage_ready_cached_;
    bool changed = false;

    if (!transport_ready) {
        storage_ready_cached_ = false;
        storage_presence_disagree_streak_ = 0;
        if (!storage_state_valid_) {
            sd_card_present_cached_ = false;
            storage_state_valid_ = true;
        }
        changed = storage_ready_cached_ != old_ready || sd_card_present_cached_ != old_present;
        last_storage_refresh_ms_ = now;
        if (changed || force_log) {
            logger_.info("board",
                         "storage state present=" + std::string(sd_card_present_cached_ ? "1" : "0") +
                             " ready=" + std::string(storage_ready_cached_ ? "1" : "0") +
                             " power=" + std::string(power_enabled ? "1" : "0") +
                             " reason=transport-gated");
        }
        (void)k_mutex_unlock(&storage_state_mutex_);
        return;
    }

    bool sampled_present = false;
    uint8_t present_votes = 0;
    uint8_t sample_count = 0;
    for (int attempt = 0; attempt < 3; ++attempt) {
        bool sample_present = false;
        if (read_sd_card_detect_raw(sample_present)) {
            ++sample_count;
            if (sample_present) {
                ++present_votes;
            }
        }
        k_sleep(K_MSEC(2));
    }

    if (sample_count > 0) {
        sampled_present = present_votes * 2 >= sample_count;
        if (!storage_state_valid_) {
            sd_card_present_cached_ = sampled_present;
            storage_state_valid_ = true;
            storage_presence_disagree_streak_ = 0;
        } else if (sampled_present == sd_card_present_cached_) {
            storage_presence_disagree_streak_ = 0;
        } else {
            ++storage_presence_disagree_streak_;
            if (storage_presence_disagree_streak_ >= kStoragePresenceCommitSamples) {
                sd_card_present_cached_ = sampled_present;
                storage_presence_disagree_streak_ = 0;
            }
        }
    }

    storage_ready_cached_ = storage_state_valid_ && transport_ready && sd_card_present_cached_;
    changed = storage_ready_cached_ != old_ready || sd_card_present_cached_ != old_present;
    last_storage_refresh_ms_ = now;
    if (changed || force_log) {
        logger_.info("board",
                     "storage state present=" + std::string(sd_card_present_cached_ ? "1" : "0") +
                         " ready=" + std::string(storage_ready_cached_ ? "1" : "0") +
                         " power=" + std::string(power_enabled ? "1" : "0") +
                         " samples=" + std::to_string(sample_count) +
                         " votes=" + std::to_string(present_votes) +
                         " disagree-streak=" + std::to_string(storage_presence_disagree_streak_));
    }
    (void)k_mutex_unlock(&storage_state_mutex_);
}

}  // namespace aegis::ports::zephyr

namespace aegis::ports::zephyr {

ZephyrTloraPagerBoardRuntime& tlora_pager_board_runtime(platform::Logger& logger) {
    static ZephyrTloraPagerBoardRuntime runtime(logger, make_tlora_pager_sx1262_backend_config());
    return runtime;
}

ZephyrTloraPagerBoardRuntime* try_tlora_pager_board_runtime() {
    return g_tlora_pager_runtime;
}

bool bootstrap_tlora_pager_board_runtime(platform::Logger& logger) {
    auto& runtime = tlora_pager_board_runtime(logger);
    if (!runtime.ready()) {
        return runtime.initialize();
    }
    runtime.log_state("reuse");
    return true;
}

}  // namespace aegis::ports::zephyr
