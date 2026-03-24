#include "ports/zephyr/zephyr_tlora_pager_board_runtime.hpp"

#include <array>
#include <string>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>

#include "ports/zephyr/zephyr_gpio_pin.hpp"

namespace aegis::ports::zephyr {

namespace {

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
    : logger_(logger), config_(std::move(config)) {}

bool ZephyrTloraPagerBoardRuntime::initialize() {
    logger_.info("board", "pager runtime begin");
    const bool devices_ready = acquire_devices();
    const bool expander_ok = initialize_expander();
    configure_interrupt_and_backlight_lines();
    prepare_shared_spi_bus();
    probe_keyboard_controller();
    probe_expander_and_log();
    probe_display_gpio_state();

    initialized_ = devices_ready;
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
    logger_.info("board",
                 "pager runtime stage=" + std::string(stage) +
                     " expander=" + std::string(expander_ready_ ? "ready" : "missing") +
                     " keyboard=" + std::string(keyboard_ready_ ? "ready" : "missing") +
                     " shared-spi=" + std::string(shared_spi_prepared_ ? "ready" : "missing"));
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

void ZephyrTloraPagerBoardRuntime::probe_keyboard_controller() {
    if (i2c_device_ == nullptr || !device_is_ready(i2c_device_)) {
        keyboard_ready_ = false;
        return;
    }

    uint8_t pending = 0;
    keyboard_ready_ = i2c_reg_read_byte(i2c_device_,
                                        kTca8418Address,
                                        kTca8418RegKeyLockEventCount,
                                        &pending) == 0;
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

}  // namespace aegis::ports::zephyr

namespace aegis::ports::zephyr {

ZephyrTloraPagerBoardRuntime& tlora_pager_board_runtime(platform::Logger& logger) {
    static ZephyrTloraPagerBoardRuntime runtime(logger, make_tlora_pager_sx1262_backend_config());
    return runtime;
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
