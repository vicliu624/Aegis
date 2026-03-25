#include "ports/zephyr/zephyr_tlora_pager_board_runtime.hpp"

#include <cerrno>
#include <optional>
#include <string>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>

#include "ports/zephyr/zephyr_gpio_pin.hpp"

namespace aegis::ports::zephyr {

namespace {
ZephyrTloraPagerBoardRuntime* g_tlora_pager_runtime = nullptr;

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

}  // namespace

ZephyrTloraPagerBoardRuntime::ZephyrTloraPagerBoardRuntime(platform::Logger& logger,
                                                           ZephyrBoardBackendConfig config)
    : logger_(logger),
      config_(std::move(config)),
      transfer_coordinator_(logger_, config_),
      board_control_provider_(logger_, config_) {
    k_mutex_init(&storage_state_mutex_);
    g_tlora_pager_runtime = this;
}

const ZephyrBoardBackendConfig& ZephyrTloraPagerBoardRuntime::config() const {
    return config_;
}

bool ZephyrTloraPagerBoardRuntime::initialize() {
    logger_.info("board", "pager runtime begin");
    const bool devices_ready = acquire_devices();
    const bool expander_ok = board_control_provider_.initialize_expander();
    configure_interrupt_and_backlight_lines();
    (void)transfer_coordinator_.prepare();

    if (peripheral_power_enabled(PowerChannel::Radio)) {
        (void)with_radio_participant(K_MSEC(20), "radio.probe.quiesce", []() { return 0; });
    }
    if (peripheral_power_enabled(PowerChannel::SdCard)) {
        (void)with_storage_participant(K_MSEC(20), "storage.probe.quiesce", []() { return 0; });
    }
    if (peripheral_power_enabled(PowerChannel::Nfc)) {
        (void)with_nfc_participant(K_MSEC(20), "nfc.probe.quiesce", []() { return 0; });
    }

    const bool keyboard_ok = board_control_provider_.probe_keyboard_controller();
    board_control_provider_.log_expander_state();
    probe_display_gpio_state();

    initialized_ = devices_ready;
    refresh_storage_state(true, true);
    logger_.info("board",
                 "pager runtime complete devices=" + std::string(devices_ready ? "ready" : "degraded") +
                     " expander=" + std::string(expander_ok ? "ready" : "missing") +
                     " keyboard-probe=" + std::string(keyboard_ok ? "ready" : "missing") +
                     " transfer-coordination=" +
                     std::string(transfer_coordinator_.ready() ? "ready" : "missing") +
                     " board-control=" +
                     std::string(board_control_provider_.ready() ? "ready" : "missing"));
    return devices_ready;
}

bool ZephyrTloraPagerBoardRuntime::ready() const {
    return initialized_;
}

void ZephyrTloraPagerBoardRuntime::log_state(std::string_view stage) const {
    refresh_storage_state(false, false);
    logger_.info("board",
                 "pager runtime stage=" + std::string(stage) +
                     " expander=" + std::string(expander_ready() ? "ready" : "missing") +
                     " keyboard=" + std::string(keyboard_ready() ? "ready" : "missing") +
                     " transfer-coordination=" +
                     std::string(transfer_coordinator_.ready() ? "ready" : "missing") +
                     " transfer-owner=" + transfer_coordinator_.owner_name() +
                     " board-control=" +
                     std::string(board_control_provider_.ready() ? "ready" : "missing") +
                     " board-owner=" + board_control_provider_.owner_name() +
                     " radio=" + std::string(radio_ready() ? "ready" : "gated") +
                     " gps=" + std::string(gps_ready() ? "ready" : "gated") +
                     " storage=" + std::string(storage_ready() ? "ready" : "gated"));
}

int ZephyrTloraPagerBoardRuntime::with_coordination_participant(
    CoordinationParticipant participant,
    k_timeout_t timeout,
    std::string_view operation,
    const std::function<int()>& action) const {
    return transfer_coordinator_.with_participant(participant, timeout, operation, action);
}

int ZephyrTloraPagerBoardRuntime::with_radio_participant(
    k_timeout_t timeout,
    std::string_view operation,
    const std::function<int()>& action) const {
    return with_coordination_participant(CoordinationParticipant::Radio, timeout, operation, action);
}

int ZephyrTloraPagerBoardRuntime::with_storage_participant(
    k_timeout_t timeout,
    std::string_view operation,
    const std::function<int()>& action) const {
    return with_coordination_participant(CoordinationParticipant::Storage, timeout, operation, action);
}

int ZephyrTloraPagerBoardRuntime::with_nfc_participant(
    k_timeout_t timeout,
    std::string_view operation,
    const std::function<int()>& action) const {
    return with_coordination_participant(CoordinationParticipant::Nfc, timeout, operation, action);
}

int ZephyrTloraPagerBoardRuntime::with_board_control_participant(
    BoardControlParticipant participant,
    k_timeout_t timeout,
    std::string_view operation,
    const std::function<int()>& action) const {
    return board_control_provider_.with_participant(participant, timeout, operation, action);
}

int ZephyrTloraPagerBoardRuntime::read_i2c_reg(uint16_t address, uint8_t reg, uint8_t* value) const {
    return board_control_provider_.read_i2c_reg(address, reg, value);
}

int ZephyrTloraPagerBoardRuntime::write_i2c_reg(uint16_t address, uint8_t reg, uint8_t value) const {
    return board_control_provider_.write_i2c_reg(address, reg, value);
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
    return board_control_provider_.keyboard_pending_event_count(pending);
}

bool ZephyrTloraPagerBoardRuntime::keyboard_read_event(uint8_t& raw_event) const {
    return board_control_provider_.keyboard_read_event(raw_event);
}

bool ZephyrTloraPagerBoardRuntime::set_power_enabled(PowerChannel channel, bool enabled) const {
    const bool ok = board_control_provider_.set_power_enabled(channel, enabled);
    if (ok && channel == PowerChannel::SdCard) {
        refresh_storage_state(true, true);
    }
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
    return board_control_provider_.peripheral_power_enabled(channel);
}

bool ZephyrTloraPagerBoardRuntime::sd_card_present() const {
    refresh_storage_state(false, false);
    return sd_card_present_cached_;
}

bool ZephyrTloraPagerBoardRuntime::expander_ready() const {
    return board_control_provider_.expander_ready();
}

ZephyrTloraPagerBoardRuntime::CoordinationParticipant
ZephyrTloraPagerBoardRuntime::coordination_participant_owner() const {
    return transfer_coordinator_.owner();
}

ZephyrTloraPagerBoardRuntime::BoardControlParticipant
ZephyrTloraPagerBoardRuntime::board_control_participant_owner() const {
    return board_control_provider_.owner();
}

bool ZephyrTloraPagerBoardRuntime::coordination_domain_ready(ZephyrBoardCoordinationDomain domain) const {
    switch (domain) {
        case ZephyrBoardCoordinationDomain::DisplayPipeline:
        case ZephyrBoardCoordinationDomain::RadioSession:
        case ZephyrBoardCoordinationDomain::StorageSession:
        case ZephyrBoardCoordinationDomain::NfcSession:
            return transfer_coordinator_.ready();
        case ZephyrBoardCoordinationDomain::BoardControl:
            return board_control_provider_.ready();
        case ZephyrBoardCoordinationDomain::Unknown:
        default:
            return false;
    }
}

std::string ZephyrTloraPagerBoardRuntime::coordination_domain_coordinator_name(
    ZephyrBoardCoordinationDomain domain) const {
    switch (domain) {
        case ZephyrBoardCoordinationDomain::DisplayPipeline:
        case ZephyrBoardCoordinationDomain::RadioSession:
        case ZephyrBoardCoordinationDomain::StorageSession:
        case ZephyrBoardCoordinationDomain::NfcSession:
            return ZephyrTloraPagerTransferCoordinator::coordinator_name();
        case ZephyrBoardCoordinationDomain::BoardControl:
            return ZephyrTloraPagerBoardControlProvider::coordinator_name();
        case ZephyrBoardCoordinationDomain::Unknown:
        default:
            return "none";
    }
}

std::string ZephyrTloraPagerBoardRuntime::coordination_domain_owner_name(
    ZephyrBoardCoordinationDomain domain) const {
    switch (domain) {
        case ZephyrBoardCoordinationDomain::DisplayPipeline:
        case ZephyrBoardCoordinationDomain::RadioSession:
        case ZephyrBoardCoordinationDomain::StorageSession:
        case ZephyrBoardCoordinationDomain::NfcSession:
            return transfer_coordinator_.owner_name();
        case ZephyrBoardCoordinationDomain::BoardControl:
            return board_control_provider_.owner_name();
        case ZephyrBoardCoordinationDomain::Unknown:
        default:
            return "none";
    }
}

bool ZephyrTloraPagerBoardRuntime::keyboard_ready() const {
    return board_control_provider_.keyboard_ready();
}

bool ZephyrTloraPagerBoardRuntime::radio_ready() const {
    return ready() && expander_ready() && transfer_coordinator_.ready() &&
           peripheral_power_enabled(PowerChannel::Radio);
}

bool ZephyrTloraPagerBoardRuntime::gps_ready() const {
    return ready() && expander_ready() && peripheral_power_enabled(PowerChannel::Gps);
}

bool ZephyrTloraPagerBoardRuntime::nfc_ready() const {
    return ready() && expander_ready() && transfer_coordinator_.ready() &&
           peripheral_power_enabled(PowerChannel::Nfc);
}

bool ZephyrTloraPagerBoardRuntime::storage_ready() const {
    refresh_storage_state(false, false);
    return storage_ready_cached_;
}

bool ZephyrTloraPagerBoardRuntime::audio_ready() const {
    return ready() && expander_ready() &&
           peripheral_power_enabled(PowerChannel::SpeakerAmp);
}

bool ZephyrTloraPagerBoardRuntime::hostlink_ready() const {
    return ready();
}

ZephyrShellDisplayBackendProfile ZephyrTloraPagerBoardRuntime::shell_display_backend_profile() const {
    return ZephyrShellDisplayBackendProfile::Pager;
}

ZephyrShellInputBackendProfile ZephyrTloraPagerBoardRuntime::shell_input_backend_profile() const {
    return ZephyrShellInputBackendProfile::PagerDirect;
}

int ZephyrTloraPagerBoardRuntime::with_coordination_domain(
    ZephyrBoardCoordinationDomain domain,
    k_timeout_t timeout,
    std::string_view operation,
    const std::function<int()>& action) const {
    switch (domain) {
        case ZephyrBoardCoordinationDomain::DisplayPipeline:
            return with_coordination_participant(CoordinationParticipant::Display, timeout, operation, action);
        case ZephyrBoardCoordinationDomain::RadioSession:
            return with_coordination_participant(CoordinationParticipant::Radio, timeout, operation, action);
        case ZephyrBoardCoordinationDomain::StorageSession:
            return with_coordination_participant(CoordinationParticipant::Storage, timeout, operation, action);
        case ZephyrBoardCoordinationDomain::NfcSession:
            return with_coordination_participant(CoordinationParticipant::Nfc, timeout, operation, action);
        case ZephyrBoardCoordinationDomain::BoardControl:
            return with_board_control_participant(BoardControlParticipant::Unknown, timeout, operation, action);
        case ZephyrBoardCoordinationDomain::Unknown:
        default:
            return ZephyrBoardRuntime::with_coordination_domain(domain, timeout, operation, action);
    }
}

bool ZephyrTloraPagerBoardRuntime::acquire_devices() {
    gpio_device_ = DEVICE_DT_GET(DT_NODELABEL(gpio0));
#if DT_NODE_EXISTS(DT_NODELABEL(i2c0))
    i2c_device_ = DEVICE_DT_GET(DT_NODELABEL(i2c0));
#endif

    transfer_coordinator_.bind_gpio_device(gpio_device_);
    board_control_provider_.bind_i2c_device(i2c_device_);

    const bool gpio_ready = gpio_device_ != nullptr && device_is_ready(gpio_device_);
    const bool i2c_ready = i2c_device_ != nullptr && device_is_ready(i2c_device_);
    logger_.info("board",
                 "pager device acquisition gpio=" + std::string(gpio_ready ? "ready" : "missing") +
                     " i2c=" + std::string(i2c_ready ? "ready" : "missing"));
    return gpio_ready;
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

void ZephyrTloraPagerBoardRuntime::refresh_storage_state(bool force_log, bool force_sample) const {
    if (k_mutex_lock(&storage_state_mutex_, K_MSEC(50)) != 0) {
        return;
    }

    const bool power_enabled = peripheral_power_enabled(PowerChannel::SdCard);
    const bool transport_ready = ready() && expander_ready() && transfer_coordinator_.ready() && power_enabled;
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
        if (board_control_provider_.read_sd_card_detect_raw(sample_present)) {
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
