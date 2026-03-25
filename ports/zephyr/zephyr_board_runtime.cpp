#include "ports/zephyr/zephyr_board_runtime.hpp"

#include <cerrno>
#include <stdexcept>

#if defined(CONFIG_BT)
#include <zephyr/bluetooth/bluetooth.h>
#endif

#include "ports/zephyr/zephyr_board_descriptors.hpp"

namespace aegis::ports::zephyr {

namespace {

ZephyrBoardRuntime* g_active_runtime = nullptr;

#if defined(CONFIG_BT)
k_mutex g_bluetooth_state_mutex;
bool g_bluetooth_state_mutex_ready = false;
bool g_bluetooth_stack_enabled = false;

void ensure_bluetooth_mutex_ready() {
    if (!g_bluetooth_state_mutex_ready) {
        k_mutex_init(&g_bluetooth_state_mutex);
        g_bluetooth_state_mutex_ready = true;
    }
}
#endif

ZephyrBoardBackendConfig config_for_package(std::string_view package_id) {
    try {
        return descriptor_for_package(package_id).config;
    } catch (const std::runtime_error&) {
    }
    return ZephyrBoardBackendConfig {
        .backend_id = std::string(package_id),
        .target_board = "unknown",
        .profile_device_id = std::string(package_id),
        .board_family = "generic-zephyr",
        .runtime_family = ZephyrBoardRuntimeFamily::Generic,
        .display_backend_family = ZephyrBoardDisplayBackendFamily::Generic,
        .input_backend_family = ZephyrBoardInputBackendFamily::Generic,
    };
}

}  // namespace

bool ZephyrBoardRuntime::expander_ready() const {
    return false;
}

bool ZephyrBoardRuntime::coordination_domain_ready(ZephyrBoardCoordinationDomain domain) const {
    switch (domain) {
        case ZephyrBoardCoordinationDomain::Unknown:
            return false;
        default:
            return ready();
    }
}

std::string ZephyrBoardRuntime::coordination_domain_coordinator_name(
    ZephyrBoardCoordinationDomain domain) const {
    switch (domain) {
        case ZephyrBoardCoordinationDomain::DisplayPipeline:
            return "display-pipeline";
        case ZephyrBoardCoordinationDomain::RadioSession:
            return "radio-session";
        case ZephyrBoardCoordinationDomain::StorageSession:
            return "storage-session";
        case ZephyrBoardCoordinationDomain::NfcSession:
            return "nfc-session";
        case ZephyrBoardCoordinationDomain::BoardControl:
            return "board-control";
        case ZephyrBoardCoordinationDomain::Unknown:
        default:
            return "none";
    }
}

std::string ZephyrBoardRuntime::coordination_domain_owner_name(
    ZephyrBoardCoordinationDomain domain) const {
    return coordination_domain_ready(domain) ? "board-runtime" : "none";
}

bool ZephyrBoardRuntime::keyboard_ready() const {
    return false;
}

bool ZephyrBoardRuntime::touch_ready() const {
    return false;
}

bool ZephyrBoardRuntime::battery_ready() const {
    return false;
}

int ZephyrBoardRuntime::battery_percent() const {
    return -1;
}

int ZephyrBoardRuntime::battery_voltage_mv() const {
    return -1;
}

bool ZephyrBoardRuntime::battery_charging() const {
    return false;
}

bool ZephyrBoardRuntime::radio_ready() const {
    return ready();
}

bool ZephyrBoardRuntime::gps_ready() const {
    return ready();
}

bool ZephyrBoardRuntime::nfc_ready() const {
    return ready();
}

bool ZephyrBoardRuntime::storage_ready() const {
    return ready();
}

bool ZephyrBoardRuntime::audio_ready() const {
    return ready();
}

bool ZephyrBoardRuntime::hostlink_ready() const {
    return ready();
}

bool ZephyrBoardRuntime::sd_card_present() const {
    return storage_ready();
}

bool ZephyrBoardRuntime::set_display_backlight_enabled(bool enabled) const {
    (void)enabled;
    return false;
}

bool ZephyrBoardRuntime::set_display_brightness_percent(uint8_t percent) const {
    (void)percent;
    return false;
}

bool ZephyrBoardRuntime::set_keyboard_backlight_enabled(bool enabled) const {
    (void)enabled;
    return false;
}

bool ZephyrBoardRuntime::set_bluetooth_enabled(bool enabled) {
#if defined(CONFIG_BT)
    ensure_bluetooth_mutex_ready();
    if (k_mutex_lock(&g_bluetooth_state_mutex, K_MSEC(250)) != 0) {
        return false;
    }

    const bool already_enabled = g_bluetooth_stack_enabled;
    int rc = 0;
    if (already_enabled != enabled) {
        rc = enabled ? bt_enable(nullptr) : bt_disable();
        if (rc == 0) {
            g_bluetooth_stack_enabled = enabled;
        }
    }

    (void)k_mutex_unlock(&g_bluetooth_state_mutex);
    return rc == 0;
#else
    (void)enabled;
    return false;
#endif
}

bool ZephyrBoardRuntime::bluetooth_enabled() const {
#if defined(CONFIG_BT)
    return g_bluetooth_stack_enabled;
#else
    return false;
#endif
}

bool ZephyrBoardRuntime::set_gps_enabled(bool enabled) {
    (void)enabled;
    return false;
}

bool ZephyrBoardRuntime::gps_enabled() const {
    return true;
}

ZephyrShellDisplayBackendProfile ZephyrBoardRuntime::shell_display_backend_profile() const {
    switch (config().display_backend_family) {
        case ZephyrBoardDisplayBackendFamily::Pager:
            return ZephyrShellDisplayBackendProfile::Pager;
        case ZephyrBoardDisplayBackendFamily::TDeck:
            return ZephyrShellDisplayBackendProfile::TDeck;
        case ZephyrBoardDisplayBackendFamily::Generic:
        default:
            return ZephyrShellDisplayBackendProfile::Generic;
    }
}

ZephyrShellInputBackendProfile ZephyrBoardRuntime::shell_input_backend_profile() const {
    switch (config().input_backend_family) {
        case ZephyrBoardInputBackendFamily::PagerDirect:
            return ZephyrShellInputBackendProfile::PagerDirect;
        case ZephyrBoardInputBackendFamily::TDeckDirect:
            return ZephyrShellInputBackendProfile::TDeckDirect;
        case ZephyrBoardInputBackendFamily::Generic:
        default:
            return ZephyrShellInputBackendProfile::Generic;
    }
}

bool ZephyrBoardRuntime::keyboard_irq_asserted() const {
    return false;
}

bool ZephyrBoardRuntime::keyboard_pending_event_count(uint8_t& pending) const {
    pending = 0;
    return false;
}

bool ZephyrBoardRuntime::keyboard_read_event(uint8_t& raw_event) const {
    raw_event = 0;
    return false;
}

bool ZephyrBoardRuntime::keyboard_read_character(uint8_t& raw_character) const {
    raw_character = 0;
    return false;
}

bool ZephyrBoardRuntime::touch_read_point(int16_t& x, int16_t& y, bool& pressed) const {
    x = 0;
    y = 0;
    pressed = false;
    return false;
}

int ZephyrBoardRuntime::with_coordination_domain(ZephyrBoardCoordinationDomain domain,
                                                 k_timeout_t timeout,
                                                 std::string_view operation,
                                                 const std::function<int()>& action) const {
    (void)domain;
    (void)timeout;
    (void)operation;
    if (action == nullptr) {
        return -EINVAL;
    }
    return action();
}

ZephyrGenericBoardRuntime::ZephyrGenericBoardRuntime(ZephyrBoardBackendConfig config)
    : config_(std::move(config)) {}

const ZephyrBoardBackendConfig& ZephyrGenericBoardRuntime::config() const {
    return config_;
}

bool ZephyrGenericBoardRuntime::initialize() {
    initialized_ = true;
    return true;
}

bool ZephyrGenericBoardRuntime::ready() const {
    return initialized_;
}

void ZephyrGenericBoardRuntime::log_state(std::string_view stage) const {
    (void)stage;
}

void ZephyrGenericBoardRuntime::signal_boot_stage(int stage) const {
    (void)stage;
}

void ZephyrGenericBoardRuntime::heartbeat_pulse() const {}

ZephyrBoardRuntime& generic_zephyr_board_runtime(std::string_view package_id) {
    static ZephyrGenericBoardRuntime device_a_runtime(config_for_package("zephyr_device_a"));
    static ZephyrGenericBoardRuntime device_b_runtime(config_for_package("zephyr_device_b"));
    static ZephyrGenericBoardRuntime fallback_runtime(config_for_package("zephyr_generic"));

    if (package_id == "zephyr_device_a") {
        return device_a_runtime;
    }
    if (package_id == "zephyr_device_b") {
        return device_b_runtime;
    }
    return fallback_runtime;
}

void set_active_zephyr_board_runtime(ZephyrBoardRuntime* runtime) {
    g_active_runtime = runtime;
}

ZephyrBoardRuntime* try_active_zephyr_board_runtime() {
    return g_active_runtime;
}

ZephyrBoardRuntime& require_active_zephyr_board_runtime() {
    if (g_active_runtime == nullptr) {
        throw std::runtime_error("active Zephyr board runtime is not bound");
    }
    return *g_active_runtime;
}

}  // namespace aegis::ports::zephyr
