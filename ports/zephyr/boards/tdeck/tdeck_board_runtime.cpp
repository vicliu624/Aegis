#include "ports/zephyr/boards/tdeck/tdeck_board_runtime.hpp"

#include <cerrno>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/pm/device.h>

#include "ports/zephyr/zephyr_gpio_pin.hpp"

namespace aegis::ports::zephyr {

namespace {

}  // namespace

ZephyrTdeckBoardRuntime::ZephyrTdeckBoardRuntime(platform::Logger& logger, ZephyrBoardBackendConfig config)
    : logger_(logger),
      config_(std::move(config)),
      transfer_coordinator_(logger_, config_),
      board_control_provider_(logger_, config_) {}

const ZephyrBoardBackendConfig& ZephyrTdeckBoardRuntime::config() const { return config_; }

bool ZephyrTdeckBoardRuntime::initialize() {
    logger_.info("board", "tdeck runtime begin");
    const bool devices_ready = acquire_devices();
    const bool transfer_ready = transfer_coordinator_.prepare();
    const bool power_ready = board_control_provider_.initialize_power_and_backlights();
    configure_trackball_pins();
    battery_ready_ = board_control_provider_.probe_battery_controller();
    touch_ready_ = board_control_provider_.probe_touch_controller();
    const bool keyboard_ok = board_control_provider_.probe_keyboard_controller();
    const bool keyboard_sampler_ok = keyboard_ok && board_control_provider_.start_keyboard_sampler();
    initialized_ = devices_ready;
    logger_.info("board",
                 "tdeck runtime complete devices=" + std::string(devices_ready ? "ready" : "degraded") +
                     " transfer-coordination=" + std::string(transfer_ready ? "ready" : "missing") +
                     " board-control=" + std::string(board_control_provider_.ready() ? "ready" : "missing") +
                     " power-init=" + std::string(power_ready ? "ready" : "degraded") +
                     " keyboard=" + std::string(keyboard_ok ? "ready" : "missing") +
                     " keyboard-sampler=" + std::string(keyboard_sampler_ok ? "ready" : "missing") +
                     " touch=" + std::string(touch_ready_ ? "ready" : "missing") +
                     " battery=" + std::string(battery_ready_ ? "ready" : "missing") +
                     " gps=" + std::string(gps_ready() ? "ready" : "gated"));
    return devices_ready;
}

bool ZephyrTdeckBoardRuntime::ready() const { return initialized_; }

void ZephyrTdeckBoardRuntime::log_state(std::string_view stage) const {
    logger_.info("board",
                 "tdeck runtime stage=" + std::string(stage) +
                     " transfer-coordination=" + std::string(transfer_coordinator_.ready() ? "ready" : "missing") +
                     " transfer-owner=" + transfer_coordinator_.owner_name() +
                     " board-control=" + std::string(board_control_provider_.ready() ? "ready" : "missing") +
                     " board-owner=" + board_control_provider_.owner_name() +
                     " keyboard=" + std::string(keyboard_ready() ? "ready" : "missing") +
                     " touch=" + std::string(touch_ready_ ? "ready" : "missing") +
                     " battery=" + std::string(battery_ready_ ? "ready" : "missing") +
                     " gps=" + std::string(gps_ready() ? "ready" : "gated"));
}

void ZephyrTdeckBoardRuntime::signal_boot_stage(int stage) const {
    logger_.info("board", "tdeck boot stage signal=" + std::to_string(stage));
}

void ZephyrTdeckBoardRuntime::heartbeat_pulse() const {}

bool ZephyrTdeckBoardRuntime::coordination_domain_ready(ZephyrBoardCoordinationDomain domain) const {
    switch (domain) {
        case ZephyrBoardCoordinationDomain::DisplayPipeline:
        case ZephyrBoardCoordinationDomain::RadioSession:
        case ZephyrBoardCoordinationDomain::StorageSession:
            return transfer_coordinator_.ready();
        case ZephyrBoardCoordinationDomain::BoardControl:
            return board_control_provider_.ready();
        case ZephyrBoardCoordinationDomain::NfcSession:
        case ZephyrBoardCoordinationDomain::Unknown:
        default:
            return false;
    }
}

std::string ZephyrTdeckBoardRuntime::coordination_domain_coordinator_name(ZephyrBoardCoordinationDomain domain) const {
    switch (domain) {
        case ZephyrBoardCoordinationDomain::DisplayPipeline:
        case ZephyrBoardCoordinationDomain::RadioSession:
        case ZephyrBoardCoordinationDomain::StorageSession:
            return ZephyrTdeckTransferCoordinator::coordinator_name();
        case ZephyrBoardCoordinationDomain::BoardControl:
            return ZephyrTdeckBoardControlProvider::coordinator_name();
        case ZephyrBoardCoordinationDomain::NfcSession:
        case ZephyrBoardCoordinationDomain::Unknown:
        default:
            return "none";
    }
}

std::string ZephyrTdeckBoardRuntime::coordination_domain_owner_name(ZephyrBoardCoordinationDomain domain) const {
    switch (domain) {
        case ZephyrBoardCoordinationDomain::DisplayPipeline:
        case ZephyrBoardCoordinationDomain::RadioSession:
        case ZephyrBoardCoordinationDomain::StorageSession:
            return transfer_coordinator_.owner_name();
        case ZephyrBoardCoordinationDomain::BoardControl:
            return board_control_provider_.owner_name();
        case ZephyrBoardCoordinationDomain::NfcSession:
        case ZephyrBoardCoordinationDomain::Unknown:
        default:
            return "none";
    }
}

bool ZephyrTdeckBoardRuntime::keyboard_ready() const { return board_control_provider_.keyboard_ready(); }
bool ZephyrTdeckBoardRuntime::keyboard_pending_event_count(uint8_t& pending) const {
    return board_control_provider_.keyboard_pending_character_count(pending);
}
bool ZephyrTdeckBoardRuntime::keyboard_read_event(uint8_t& raw_event) const {
    return board_control_provider_.keyboard_pop_character(raw_event);
}
bool ZephyrTdeckBoardRuntime::touch_ready() const { return touch_ready_; }
bool ZephyrTdeckBoardRuntime::battery_ready() const { return battery_ready_; }
int ZephyrTdeckBoardRuntime::battery_percent() const {
    return board_control_provider_.battery_percent().value_or(-1);
}
int ZephyrTdeckBoardRuntime::battery_voltage_mv() const {
    return board_control_provider_.battery_voltage_mv().value_or(-1);
}
bool ZephyrTdeckBoardRuntime::battery_charging() const { return board_control_provider_.battery_charging(); }
bool ZephyrTdeckBoardRuntime::radio_ready() const { return ready() && transfer_coordinator_.ready(); }
bool ZephyrTdeckBoardRuntime::gps_ready() const {
    return ready() && gps_enabled_ && gps_device_ != nullptr && device_is_ready(gps_device_);
}
bool ZephyrTdeckBoardRuntime::storage_ready() const { return ready() && transfer_coordinator_.ready() && config_.removable_storage; }
bool ZephyrTdeckBoardRuntime::audio_ready() const { return ready(); }
bool ZephyrTdeckBoardRuntime::hostlink_ready() const { return ready(); }
bool ZephyrTdeckBoardRuntime::set_display_backlight_enabled(bool enabled) const {
    display_backlight_enabled_ = enabled;
    const uint8_t percent = enabled ? display_brightness_percent_ : 0;
    const bool ok = board_control_provider_.set_display_backlight_percent(percent);
    logger_.info("board",
                 "tdeck display backlight enabled=" + std::string(enabled ? "1" : "0") +
                     " percent=" + std::to_string(percent) +
                     " apply=" + std::string(ok ? "ok" : "fail"));
    return ok;
}
bool ZephyrTdeckBoardRuntime::set_display_brightness_percent(uint8_t percent) const {
    display_brightness_percent_ = percent;
    if (!display_backlight_enabled_) {
        logger_.info("board",
                     "tdeck display brightness cached percent=" + std::to_string(percent) +
                         " apply=deferred");
        return true;
    }
    const bool ok = board_control_provider_.set_display_backlight_percent(percent);
    logger_.info("board",
                 "tdeck display brightness percent=" + std::to_string(percent) +
                     " apply=" + std::string(ok ? "ok" : "fail"));
    return ok;
}
bool ZephyrTdeckBoardRuntime::set_keyboard_backlight_enabled(bool enabled) const {
    return board_control_provider_.set_keyboard_backlight_level(enabled ? 127 : 0);
}
bool ZephyrTdeckBoardRuntime::set_gps_enabled(bool enabled) {
    if (gps_enabled_ == enabled) {
        return true;
    }

    const bool applied = apply_gps_device_power_state(enabled);
    if (!applied) {
        logger_.info("board",
                     "tdeck gps apply failed enabled=" + std::string(enabled ? "1" : "0"));
        return false;
    }

    gps_enabled_ = enabled;
    return true;
}
bool ZephyrTdeckBoardRuntime::gps_enabled() const { return gps_enabled_; }
bool ZephyrTdeckBoardRuntime::keyboard_read_character(uint8_t& raw_character) const {
    return board_control_provider_.keyboard_pop_character(raw_character);
}
bool ZephyrTdeckBoardRuntime::touch_read_point(int16_t& x, int16_t& y, bool& pressed) const {
    return board_control_provider_.read_touch_point(x, y, pressed);
}

int ZephyrTdeckBoardRuntime::with_coordination_domain(ZephyrBoardCoordinationDomain domain,
                                                      k_timeout_t timeout,
                                                      std::string_view operation,
                                                      const std::function<int()>& action) const {
    switch (domain) {
        case ZephyrBoardCoordinationDomain::DisplayPipeline:
            return transfer_coordinator_.with_participant(TdeckTransferParticipant::Display, timeout, operation, action);
        case ZephyrBoardCoordinationDomain::RadioSession:
            return transfer_coordinator_.with_participant(TdeckTransferParticipant::Radio, timeout, operation, action);
        case ZephyrBoardCoordinationDomain::StorageSession:
            return transfer_coordinator_.with_participant(TdeckTransferParticipant::Storage, timeout, operation, action);
        case ZephyrBoardCoordinationDomain::BoardControl:
            return board_control_provider_.with_participant(TdeckBoardControlParticipant::Unknown, timeout, operation, action);
        case ZephyrBoardCoordinationDomain::NfcSession:
        case ZephyrBoardCoordinationDomain::Unknown:
        default:
            return ZephyrBoardRuntime::with_coordination_domain(domain, timeout, operation, action);
    }
}

bool ZephyrTdeckBoardRuntime::acquire_devices() {
    gpio_device_ = DEVICE_DT_GET(DT_NODELABEL(gpio0));
#if DT_NODE_EXISTS(DT_NODELABEL(i2c0))
    i2c_device_ = DEVICE_DT_GET(DT_NODELABEL(i2c0));
#endif
#if DT_NODE_EXISTS(DT_NODELABEL(adc0))
    adc_device_ = device_get_binding(DEVICE_DT_NAME(DT_NODELABEL(adc0)));
#endif
#if DT_NODE_EXISTS(DT_NODELABEL(ledc0))
    pwm_device_ = DEVICE_DT_GET(DT_NODELABEL(ledc0));
#endif
    gps_device_ = device_get_binding(config_.gps_device_name.c_str());
    transfer_coordinator_.bind_gpio_device(gpio_device_);
    board_control_provider_.bind_gpio_device(gpio_device_);
    board_control_provider_.bind_i2c_device(i2c_device_);
    board_control_provider_.bind_adc_device(adc_device_);
    board_control_provider_.bind_pwm_device(pwm_device_);
    const bool gpio_ready = gpio_device_ != nullptr && device_is_ready(gpio_device_);
    logger_.info("board",
                 "tdeck device acquisition gpio=" + std::string(gpio_ready ? "ready" : "missing") +
                     " i2c=" + std::string(i2c_device_ != nullptr && device_is_ready(i2c_device_) ? "ready" : "missing") +
                     " adc=" + std::string(adc_device_ != nullptr && device_is_ready(adc_device_) ? "ready" : "missing") +
                     " pwm=" + std::string(pwm_device_ != nullptr && device_is_ready(pwm_device_) ? "ready" : "missing") +
                     " gps=" + std::string(gps_device_ != nullptr && device_is_ready(gps_device_) ? "ready" : "missing"));
    return gpio_ready;
}

void ZephyrTdeckBoardRuntime::configure_trackball_pins() {
    const int pins[] = {config_.trackball_up_pin,
                        config_.trackball_down_pin,
                        config_.trackball_left_pin,
                        config_.trackball_right_pin,
                        config_.trackball_click_pin};
    for (const int pin : pins) {
        if (pin < 0) {
            continue;
        }
        const auto pin_ref = resolve_gpio_pin(pin);
        if (pin_ref.device != nullptr && device_is_ready(pin_ref.device)) {
            (void)gpio_pin_configure(pin_ref.device, pin_ref.pin, GPIO_INPUT | GPIO_PULL_UP);
        }
    }

    if (config_.touch_irq_pin >= 0) {
        const auto pin_ref = resolve_gpio_pin(config_.touch_irq_pin);
        if (pin_ref.device != nullptr && device_is_ready(pin_ref.device)) {
            (void)gpio_pin_configure(pin_ref.device, pin_ref.pin, GPIO_INPUT | GPIO_PULL_UP);
            logger_.info("board",
                         "tdeck touch irq configured pin=" + std::to_string(config_.touch_irq_pin));
        }
    }
}

bool ZephyrTdeckBoardRuntime::apply_gps_device_power_state(bool enabled) {
    if (gps_device_ == nullptr || !device_is_ready(gps_device_)) {
        logger_.info("board",
                     "tdeck gps device missing name=" + config_.gps_device_name +
                         " enabled=" + std::string(enabled ? "1" : "0") +
                         " mode=logical-only");
        return true;
    }

#if defined(CONFIG_PM_DEVICE)
    const auto action = enabled ? PM_DEVICE_ACTION_RESUME : PM_DEVICE_ACTION_SUSPEND;
    const int rc = pm_device_action_run(gps_device_, action);
    if (rc == 0) {
        logger_.info("board",
                     "tdeck gps device action=" +
                         std::string(enabled ? "resume" : "suspend") +
                         " name=" + gps_device_->name + " rc=0");
        return true;
    }

    if (rc == -ENOTSUP || rc == -ENOSYS) {
        logger_.info("board",
                     "tdeck gps device action unsupported name=" + std::string(gps_device_->name) +
                         " rc=" + std::to_string(rc) + " mode=logical-only");
        return true;
    }

    logger_.info("board",
                 "tdeck gps device action failed name=" + std::string(gps_device_->name) +
                     " action=" + std::string(enabled ? "resume" : "suspend") +
                     " rc=" + std::to_string(rc));
    return false;
#else
    logger_.info("board",
                 "tdeck gps device name=" + std::string(gps_device_->name) +
                     " enabled=" + std::string(enabled ? "1" : "0") +
                     " mode=logical-only reason=pm-device-disabled");
    return true;
#endif
}

ZephyrTdeckBoardRuntime& tdeck_board_runtime(platform::Logger& logger) {
    static ZephyrTdeckBoardRuntime runtime(logger, make_tdeck_sx1262_backend_config());
    return runtime;
}

}  // namespace aegis::ports::zephyr
