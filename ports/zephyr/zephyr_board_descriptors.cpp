#include "ports/zephyr/zephyr_board_descriptors.hpp"

#include <stdexcept>

namespace aegis::ports::zephyr {

namespace {

device::CapabilityDescriptor cap(device::CapabilityId id,
                                 device::CapabilityLevel level,
                                 std::string detail) {
    return device::CapabilityDescriptor {
        .id = id,
        .level = level,
        .flags = 0,
        .detail = std::move(detail),
    };
}

device::DeviceProfile make_device_a_profile(const ZephyrBoardBackendConfig& config) {
    return device::DeviceProfile {
        .device_id = config.profile_device_id,
        .board_family = config.board_family,
        .display_topology_name = "single-display-320x240",
        .input_topology_name = "joystick-primary",
        .display = {.width = config.display_width,
                    .height = config.display_height,
                    .touch = config.display_touch,
                    .layout_class = config.display_layout_class},
        .input = {.keyboard = config.keyboard_input,
                  .pointer = config.pointer_input,
                  .joystick = config.joystick_input,
                  .primary_input = config.primary_input_name},
        .storage = {.persistent_storage = true, .removable_storage = config.removable_storage},
        .power = {.battery_status = true, .low_power_mode = true},
        .comm = {.radio = true, .gps = true, .usb_hostlink = true},
        .capabilities =
            device::CapabilitySet {
                cap(device::CapabilityId::Display, device::CapabilityLevel::Full, "320x240 compact Zephyr display"),
                cap(device::CapabilityId::KeyboardInput, device::CapabilityLevel::Absent, "No hardware keyboard path"),
                cap(device::CapabilityId::PointerInput, device::CapabilityLevel::Absent, "No pointer path"),
                cap(device::CapabilityId::JoystickInput, device::CapabilityLevel::Full, "Primary joystick navigation"),
                cap(device::CapabilityId::RadioMessaging, device::CapabilityLevel::Full, "Radio service bound through Zephyr backend"),
                cap(device::CapabilityId::GpsFix, device::CapabilityLevel::Full, "GPS service bound through Zephyr backend"),
                cap(device::CapabilityId::AudioOutput, device::CapabilityLevel::Full, "Audio output path bound through Zephyr backend"),
                cap(device::CapabilityId::MicrophoneInput, device::CapabilityLevel::Full, "Audio input path bound through Zephyr backend"),
                cap(device::CapabilityId::BatteryStatus, device::CapabilityLevel::Full, "Battery telemetry available"),
                cap(device::CapabilityId::RemovableStorage, device::CapabilityLevel::Absent, "No removable storage expected"),
                cap(device::CapabilityId::UsbHostlink, device::CapabilityLevel::Full, "USB hostlink path available"),
            },
        .shell_hints = {.launcher_style = "compact-grid", .status_density = "normal"},
        .runtime_limits = {.max_app_memory_bytes = 384 * 1024, .max_ui_memory_bytes = 128 * 1024, .max_foreground_apps = 1},
    };
}

device::ShellSurfaceProfile make_device_a_shell_profile() {
    return device::ShellSurfaceProfile {
        .settings_entries = {{.id = "system", .label = "System"}, {.id = "radio", .label = "Radio"}, {.id = "gps", .label = "GPS"}},
        .status_items = {{.id = "display", .value = "320x240"}, {.id = "input", .value = "joystick"}},
        .notification_entries = {{.title = "System Ready", .body = "Shell initialized for Zephyr Device A"}},
        .navigation_actions = {shell::ShellNavigationAction::MoveNext,
                               shell::ShellNavigationAction::MovePrevious,
                               shell::ShellNavigationAction::Select,
                               shell::ShellNavigationAction::Back},
    };
}

device::DeviceProfile make_device_b_profile(const ZephyrBoardBackendConfig& config) {
    return device::DeviceProfile {
        .device_id = config.profile_device_id,
        .board_family = config.board_family,
        .display_topology_name = "single-display-480x320",
        .input_topology_name = "keyboard-primary",
        .display = {.width = config.display_width,
                    .height = config.display_height,
                    .touch = config.display_touch,
                    .layout_class = config.display_layout_class},
        .input = {.keyboard = config.keyboard_input,
                  .pointer = config.pointer_input,
                  .joystick = config.joystick_input,
                  .primary_input = config.primary_input_name},
        .storage = {.persistent_storage = true, .removable_storage = config.removable_storage},
        .power = {.battery_status = true, .low_power_mode = true},
        .comm = {.radio = true, .gps = true, .usb_hostlink = true},
        .capabilities =
            device::CapabilitySet {
                cap(device::CapabilityId::Display, device::CapabilityLevel::Full, "480x320 wide Zephyr display"),
                cap(device::CapabilityId::KeyboardInput, device::CapabilityLevel::Full, "Hardware keyboard path"),
                cap(device::CapabilityId::PointerInput, device::CapabilityLevel::Absent, "No pointer path"),
                cap(device::CapabilityId::JoystickInput, device::CapabilityLevel::Absent, "No joystick path"),
                cap(device::CapabilityId::RadioMessaging, device::CapabilityLevel::Full, "Radio service bound through Zephyr backend"),
                cap(device::CapabilityId::GpsFix, device::CapabilityLevel::Full, "GPS service bound through Zephyr backend"),
                cap(device::CapabilityId::AudioOutput, device::CapabilityLevel::Full, "Audio output path bound through Zephyr backend"),
                cap(device::CapabilityId::MicrophoneInput, device::CapabilityLevel::Full, "Audio input path bound through Zephyr backend"),
                cap(device::CapabilityId::BatteryStatus, device::CapabilityLevel::Full, "Battery telemetry available"),
                cap(device::CapabilityId::RemovableStorage, device::CapabilityLevel::Full, "Removable storage available"),
                cap(device::CapabilityId::UsbHostlink, device::CapabilityLevel::Full, "USB hostlink path available"),
            },
        .shell_hints = {.launcher_style = "wide-list", .status_density = "normal"},
        .runtime_limits = {.max_app_memory_bytes = 384 * 1024, .max_ui_memory_bytes = 128 * 1024, .max_foreground_apps = 1},
    };
}

device::ShellSurfaceProfile make_device_b_shell_profile() {
    return device::ShellSurfaceProfile {
        .settings_entries = {{.id = "system", .label = "System"},
                             {.id = "display", .label = "Display"},
                             {.id = "storage", .label = "Storage"},
                             {.id = "hostlink", .label = "Hostlink"}},
        .status_items = {{.id = "display", .value = "480x320"}, {.id = "input", .value = "keyboard"}},
        .notification_entries = {{.title = "System Ready", .body = "Shell initialized for Zephyr Device B"}},
        .navigation_actions = {shell::ShellNavigationAction::MoveNext,
                               shell::ShellNavigationAction::MovePrevious,
                               shell::ShellNavigationAction::Select,
                               shell::ShellNavigationAction::Back,
                               shell::ShellNavigationAction::OpenSettings},
    };
}

device::DeviceProfile make_tlora_pager_profile(const ZephyrBoardBackendConfig& config) {
    return device::DeviceProfile {
        .device_id = config.profile_device_id,
        .board_family = config.board_family,
        .display_topology_name = "single-display-480x222-st7796",
        .input_topology_name = "rotary-plus-tca8418-keyboard",
        .display = {.width = config.display_width,
                    .height = config.display_height,
                    .touch = false,
                    .layout_class = config.display_layout_class},
        .input = {.keyboard = true, .pointer = false, .joystick = true, .primary_input = config.primary_input_name},
        .storage = {.persistent_storage = true, .removable_storage = config.removable_storage},
        .power = {.battery_status = true, .low_power_mode = true},
        .comm = {.radio = true, .gps = true, .usb_hostlink = true},
        .capabilities =
            device::CapabilitySet {
                cap(device::CapabilityId::Display, device::CapabilityLevel::Full, "ST7796 480x222 landscape display"),
                cap(device::CapabilityId::KeyboardInput, device::CapabilityLevel::Full, "TCA8418 keyboard through Zephyr input driver and board keymap"),
                cap(device::CapabilityId::PointerInput, device::CapabilityLevel::Absent, "No pointer path"),
                cap(device::CapabilityId::JoystickInput, device::CapabilityLevel::Full, "GPIO rotary encoder plus center key navigation"),
                cap(device::CapabilityId::RadioMessaging, device::CapabilityLevel::Full, "SX1262 LoRa transceiver"),
                cap(device::CapabilityId::GpsFix, device::CapabilityLevel::Full, "UBlox MIA-M10Q GPS"),
                cap(device::CapabilityId::AudioOutput, device::CapabilityLevel::Full, "ES8311 codec plus NS4150B amplifier"),
                cap(device::CapabilityId::MicrophoneInput, device::CapabilityLevel::Degraded, "ES8311 codec microphone path"),
                cap(device::CapabilityId::BatteryStatus, device::CapabilityLevel::Full, "BQ25896/BQ27220 telemetry"),
                cap(device::CapabilityId::RemovableStorage, device::CapabilityLevel::Full, "microSD slot"),
                cap(device::CapabilityId::UsbHostlink, device::CapabilityLevel::Full, "USB CDC hostlink path"),
            },
        .shell_hints = {.launcher_style = "pager-grid", .status_density = "dense"},
        .runtime_limits = {.max_app_memory_bytes = 512 * 1024, .max_ui_memory_bytes = 128 * 1024, .max_foreground_apps = 1},
    };
}

device::DeviceProfile make_tdeck_profile(const ZephyrBoardBackendConfig& config) {
    return device::DeviceProfile {
        .device_id = config.profile_device_id,
        .board_family = config.board_family,
        .display_topology_name = "single-display-320x240-st7789-touch",
        .input_topology_name = "i2c-keyboard-trackball-touch",
        .display = {.width = config.display_width,
                    .height = config.display_height,
                    .touch = config.display_touch,
                    .layout_class = config.display_layout_class},
        .input = {.keyboard = config.keyboard_input,
                  .pointer = config.pointer_input,
                  .joystick = config.joystick_input,
                  .primary_input = config.primary_input_name},
        .storage = {.persistent_storage = true, .removable_storage = config.removable_storage},
        .power = {.battery_status = true, .low_power_mode = true},
        .comm = {.radio = true, .gps = true, .usb_hostlink = true},
        .capabilities =
            device::CapabilitySet {
                cap(device::CapabilityId::Display, device::CapabilityLevel::Full, "ST7789 320x240 landscape display"),
                cap(device::CapabilityId::KeyboardInput, device::CapabilityLevel::Full, "I2C character keyboard at 0x55"),
                cap(device::CapabilityId::PointerInput, device::CapabilityLevel::Full, "GT911 touch panel"),
                cap(device::CapabilityId::JoystickInput, device::CapabilityLevel::Full, "Trackball GPIO navigation"),
                cap(device::CapabilityId::RadioMessaging, device::CapabilityLevel::Full, "SX1262 LoRa transceiver"),
                cap(device::CapabilityId::GpsFix, device::CapabilityLevel::Full, "UART GPS path"),
                cap(device::CapabilityId::AudioOutput, device::CapabilityLevel::Full, "I2S speaker path"),
                cap(device::CapabilityId::MicrophoneInput, device::CapabilityLevel::Degraded, "I2S microphone path"),
                cap(device::CapabilityId::BatteryStatus, device::CapabilityLevel::Full, "AXP2101 battery telemetry"),
                cap(device::CapabilityId::RemovableStorage, device::CapabilityLevel::Full, "microSD slot on shared SPI"),
                cap(device::CapabilityId::UsbHostlink, device::CapabilityLevel::Full, "USB CDC hostlink path"),
            },
        .shell_hints = {.launcher_style = "tdeck-grid", .status_density = "dense"},
        .runtime_limits = {.max_app_memory_bytes = 512 * 1024,
                           .max_ui_memory_bytes = 160 * 1024,
                           .max_foreground_apps = 1},
    };
}

device::ShellSurfaceProfile make_tlora_pager_shell_profile() {
    return device::ShellSurfaceProfile {
        .settings_entries = {{.id = "system", .label = "System"},
                             {.id = "display", .label = "Display"},
                             {.id = "radio", .label = "LoRa"},
                             {.id = "gps", .label = "GPS"},
                             {.id = "audio", .label = "Audio"},
                             {.id = "storage", .label = "Storage"},
                             {.id = "power", .label = "Power"}},
        .status_items = {{.id = "display", .value = "480x222-st7796"},
                         {.id = "input", .value = "keyboard+rotary"},
                         {.id = "keyboard", .value = "tca8418-input-driver"},
                         {.id = "radio", .value = "sx1262"},
                         {.id = "gps", .value = "mia-m10q"},
                         {.id = "audio", .value = "es8311"}},
        .notification_entries = {{.title = "System Ready", .body = "Shell initialized for LilyGo T-LoRa-Pager"}},
        .navigation_actions = {shell::ShellNavigationAction::MoveNext,
                               shell::ShellNavigationAction::MovePrevious,
                               shell::ShellNavigationAction::Select,
                               shell::ShellNavigationAction::Back,
                               shell::ShellNavigationAction::OpenMenu,
                               shell::ShellNavigationAction::OpenSettings,
                               shell::ShellNavigationAction::OpenNotifications},
    };
}

device::ShellSurfaceProfile make_tdeck_shell_profile() {
    return device::ShellSurfaceProfile {
        .settings_entries = {{.id = "system", .label = "System"},
                             {.id = "display", .label = "Display"},
                             {.id = "radio", .label = "LoRa"},
                             {.id = "gps", .label = "GPS"},
                             {.id = "storage", .label = "Storage"},
                             {.id = "power", .label = "Power"},
                             {.id = "touch", .label = "Touch"}},
        .status_items = {{.id = "display", .value = "320x240-st7789"},
                         {.id = "input", .value = "keyboard+trackball+touch"},
                         {.id = "keyboard", .value = "i2c-char-kbd"},
                         {.id = "pointer", .value = "gt911"},
                         {.id = "radio", .value = "sx1262"},
                         {.id = "power", .value = "axp2101"}},
        .notification_entries = {{.title = "System Ready", .body = "Shell initialized for LilyGo T-Deck"}},
        .navigation_actions = {shell::ShellNavigationAction::MoveNext,
                               shell::ShellNavigationAction::MovePrevious,
                               shell::ShellNavigationAction::Select,
                               shell::ShellNavigationAction::Back,
                               shell::ShellNavigationAction::OpenMenu,
                               shell::ShellNavigationAction::OpenSettings,
                               shell::ShellNavigationAction::OpenNotifications},
    };
}

const ZephyrBoardDescriptor& device_a_descriptor() {
    static const ZephyrBoardDescriptor descriptor = []() {
        auto config = make_device_a_backend_config();
        return ZephyrBoardDescriptor {
            .package_id = "zephyr_device_a",
            .config = config,
            .profile = make_device_a_profile(config),
            .shell_surface_profile = make_device_a_shell_profile(),
            .bringup = {.initialization_banner = "initialize Zephyr device A orchestration",
                        .runtime_ready_stage = "runtime-ready",
                        .input_ready_stage = "post-input-init"},
            .text_input_strategy = ZephyrBoardTextInputStrategy::InMemoryUnavailable,
            .text_input_source_name = "absent-text-input",
        };
    }();
    return descriptor;
}

const ZephyrBoardDescriptor& device_b_descriptor() {
    static const ZephyrBoardDescriptor descriptor = []() {
        auto config = make_device_b_backend_config();
        return ZephyrBoardDescriptor {
            .package_id = "zephyr_device_b",
            .config = config,
            .profile = make_device_b_profile(config),
            .shell_surface_profile = make_device_b_shell_profile(),
            .bringup = {.initialization_banner = "initialize Zephyr device B orchestration",
                        .runtime_ready_stage = "runtime-ready",
                        .input_ready_stage = "post-input-init"},
            .text_input_strategy = ZephyrBoardTextInputStrategy::InMemoryKeyboard,
            .text_input_source_name = "zephyr-generic-keyboard",
        };
    }();
    return descriptor;
}

const ZephyrBoardDescriptor& tlora_pager_descriptor() {
    static const ZephyrBoardDescriptor descriptor = []() {
        auto config = make_tlora_pager_sx1262_backend_config();
        return ZephyrBoardDescriptor {
            .package_id = "zephyr_tlora_pager_sx1262",
            .config = config,
            .profile = make_tlora_pager_profile(config),
            .shell_surface_profile = make_tlora_pager_shell_profile(),
            .bringup = {.initialization_banner = "initialize " + config.backend_id + " on " + config.target_board,
                        .runtime_ready_stage = "pre-display",
                        .input_ready_stage = "post-input-init"},
            .text_input_strategy = ZephyrBoardTextInputStrategy::ZephyrService,
            .text_input_source_name = config.text_input_device_name,
        };
    }();
    return descriptor;
}

const ZephyrBoardDescriptor& tdeck_descriptor() {
    static const ZephyrBoardDescriptor descriptor = []() {
        auto config = make_tdeck_sx1262_backend_config();
        return ZephyrBoardDescriptor {
            .package_id = "zephyr_tdeck_sx1262",
            .config = config,
            .profile = make_tdeck_profile(config),
            .shell_surface_profile = make_tdeck_shell_profile(),
            .bringup = {.initialization_banner = "initialize " + config.backend_id + " on " + config.target_board,
                        .runtime_ready_stage = "pre-display",
                        .input_ready_stage = "post-input-init"},
            .text_input_strategy = ZephyrBoardTextInputStrategy::InMemoryKeyboard,
            .text_input_source_name = "tdeck-i2c-keyboard",
        };
    }();
    return descriptor;
}

}  // namespace

const ZephyrBoardDescriptor& descriptor_for_package(std::string_view package_id) {
    if (package_id == device_a_descriptor().package_id) {
        return device_a_descriptor();
    }
    if (package_id == device_b_descriptor().package_id) {
        return device_b_descriptor();
    }
    if (package_id == tlora_pager_descriptor().package_id) {
        return tlora_pager_descriptor();
    }
    if (package_id == tdeck_descriptor().package_id) {
        return tdeck_descriptor();
    }
    throw std::runtime_error("unknown Zephyr board descriptor: " + std::string(package_id));
}

std::vector<std::reference_wrapper<const ZephyrBoardDescriptor>> all_zephyr_board_descriptors() {
    return {std::cref(tlora_pager_descriptor()),
            std::cref(tdeck_descriptor()),
            std::cref(device_a_descriptor()),
            std::cref(device_b_descriptor())};
}

}  // namespace aegis::ports::zephyr
