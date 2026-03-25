#include "device/packages/mock_device_packages.hpp"

#include <memory>

#include "device/common/binding/mock_services.hpp"
#include "services/logging/console_logging_service.hpp"
#include "services/notification/console_notification_service.hpp"
#include "services/settings/in_memory_settings_service.hpp"
#include "services/text/in_memory_text_input_service.hpp"
#include "services/timer/in_memory_timer_service.hpp"

namespace aegis::device {

namespace {

CapabilityDescriptor cap(CapabilityId id, CapabilityLevel level, std::string detail) {
    return CapabilityDescriptor {.id = id, .level = level, .flags = 0, .detail = std::move(detail)};
}

}  // namespace

std::string_view MockDeviceAPackage::package_id() const {
    return "mock_device_a";
}

void MockDeviceAPackage::initialize_board(platform::Logger& logger) {
    logger.info("board", "initialize mock device A orchestration");
}

DeviceProfile MockDeviceAPackage::create_profile() const {
    return DeviceProfile {
        .device_id = "device-a-handheld",
        .board_family = "board_x",
        .display_topology_name = "single-display-320x240",
        .input_topology_name = "5-way-joystick",
        .display = {.width = 320, .height = 240, .touch = false, .layout_class = "compact"},
        .input = {.keyboard = false, .pointer = false, .joystick = true, .primary_input = "joystick"},
        .storage = {.persistent_storage = true, .removable_storage = false},
        .power = {.battery_status = true, .low_power_mode = true},
        .comm = {.radio = true, .gps = true, .usb_hostlink = false},
        .capabilities =
            CapabilitySet {
                cap(CapabilityId::Display, CapabilityLevel::Full, "320x240 main display"),
                cap(CapabilityId::JoystickInput, CapabilityLevel::Full, "5-way joystick"),
                cap(CapabilityId::KeyboardInput, CapabilityLevel::Absent, "no keyboard"),
                cap(CapabilityId::RadioMessaging, CapabilityLevel::Full, "LoRa messaging path"),
                cap(CapabilityId::GpsFix, CapabilityLevel::Full, "dedicated GPS receiver"),
                cap(CapabilityId::BatteryStatus, CapabilityLevel::Full, "battery telemetry"),
                cap(CapabilityId::UsbHostlink, CapabilityLevel::Absent, "no hostlink"),
            },
        .shell_hints = {.launcher_style = "card-grid", .status_density = "compact"},
        .runtime_limits = {.max_app_memory_bytes = 256 * 1024,
                           .max_ui_memory_bytes = 64 * 1024,
                           .max_foreground_apps = 1},
    };
}

ShellSurfaceProfile MockDeviceAPackage::create_shell_surface_profile() const {
    return ShellSurfaceProfile {
        .settings_entries =
            {
                {.id = "system", .label = "System"},
                {.id = "display", .label = "Display"},
                {.id = "storage", .label = "Storage"},
                {.id = "radio", .label = "Radio"},
                {.id = "gps", .label = "GPS"},
            },
        .status_items =
            {
                {.id = "battery", .value = "battery-present"},
                {.id = "input", .value = "joystick"},
                {.id = "radio", .value = "enabled"},
                {.id = "gps", .value = "enabled"},
            },
        .notification_entries =
            {
                {.title = "System Ready", .body = "Shell initialized for device-a-handheld"},
            },
        .navigation_actions =
            {
                shell::ShellNavigationAction::MoveNext,
                shell::ShellNavigationAction::MovePrevious,
                shell::ShellNavigationAction::PrimaryAction,
                shell::ShellNavigationAction::Select,
                shell::ShellNavigationAction::Back,
                shell::ShellNavigationAction::OpenMenu,
                shell::ShellNavigationAction::OpenSettings,
            },
    };
}

void MockDeviceAPackage::bind_services(ServiceBindingRegistry& bindings,
                                       platform::Logger& logger) const {
    bindings.bind_display(std::make_shared<MockDisplayService>(services::DisplayInfo {
        .width = 320,
        .height = 240,
        .touch = false,
        .layout_class = "compact",
        .surface_description = "320x240 compact display",
    }));
    bindings.bind_input(std::make_shared<MockInputService>(services::InputInfo {
        .keyboard = false,
        .pointer = false,
        .joystick = true,
        .primary_input = "joystick",
        .input_mode = "joystick navigation",
    }));
    bindings.bind_text_input(std::make_shared<services::InMemoryTextInputService>(
        services::TextInputState {
            .available = false,
            .text_entry = false,
            .last_key_code = 0,
            .modifier_mask = 0,
            .last_text = "",
            .source_name = "absent-text-input",
        }));
    bindings.bind_radio(std::make_shared<MockRadioService>(true, "lora-backend"));
    bindings.bind_gps(std::make_shared<MockGpsService>(true, "uart-gps-backend"));
    bindings.bind_audio(std::make_shared<MockAudioService>(true, false, "speaker-only-codec"));
    bindings.bind_storage(std::make_shared<MockStorageService>(true, "internal-flash"));
    bindings.bind_power(std::make_shared<MockPowerService>(true, true, "battery telemetry ready"));
    bindings.bind_time(std::make_shared<MockTimeService>(true, "rtc-backed time source"));
    bindings.bind_hostlink(
        std::make_shared<MockHostlinkService>(false, false, "absent-transport", "no bridge"));
    bindings.bind_logging(std::make_shared<services::ConsoleLoggingService>(logger));
    bindings.bind_settings(std::make_shared<services::InMemorySettingsService>());
    bindings.bind_timer(std::make_shared<services::InMemoryTimerService>());
    bindings.bind_notification(std::make_shared<services::ConsoleNotificationService>(logger));
}

std::string_view MockDeviceBPackage::package_id() const {
    return "mock_device_b";
}

void MockDeviceBPackage::initialize_board(platform::Logger& logger) {
    logger.info("board", "initialize mock device B orchestration");
}

DeviceProfile MockDeviceBPackage::create_profile() const {
    return DeviceProfile {
        .device_id = "device-b-messenger",
        .board_family = "board_y",
        .display_topology_name = "single-display-480x320",
        .input_topology_name = "keyboard-dominant",
        .display = {.width = 480, .height = 320, .touch = false, .layout_class = "wide"},
        .input = {.keyboard = true, .pointer = false, .joystick = false, .primary_input = "keyboard"},
        .storage = {.persistent_storage = true, .removable_storage = true},
        .power = {.battery_status = true, .low_power_mode = false},
        .comm = {.radio = false, .gps = false, .usb_hostlink = true},
        .capabilities =
            CapabilitySet {
                cap(CapabilityId::Display, CapabilityLevel::Full, "480x320 landscape display"),
                cap(CapabilityId::KeyboardInput, CapabilityLevel::Full, "hardware keyboard"),
                cap(CapabilityId::JoystickInput, CapabilityLevel::Absent, "no joystick"),
                cap(CapabilityId::RadioMessaging, CapabilityLevel::Absent, "radio not populated"),
                cap(CapabilityId::GpsFix, CapabilityLevel::Absent, "gps not present"),
                cap(CapabilityId::BatteryStatus, CapabilityLevel::Full, "battery telemetry"),
                cap(CapabilityId::RemovableStorage, CapabilityLevel::Full, "microSD storage"),
                cap(CapabilityId::UsbHostlink, CapabilityLevel::Full, "usb hostlink"),
            },
        .shell_hints = {.launcher_style = "list-dense", .status_density = "expanded"},
        .runtime_limits = {.max_app_memory_bytes = 384 * 1024,
                           .max_ui_memory_bytes = 96 * 1024,
                           .max_foreground_apps = 1},
    };
}

ShellSurfaceProfile MockDeviceBPackage::create_shell_surface_profile() const {
    return ShellSurfaceProfile {
        .settings_entries =
            {
                {.id = "system", .label = "System"},
                {.id = "display", .label = "Display"},
                {.id = "storage", .label = "Storage"},
                {.id = "hostlink", .label = "Hostlink"},
            },
        .status_items =
            {
                {.id = "battery", .value = "battery-present"},
                {.id = "input", .value = "keyboard"},
            },
        .notification_entries =
            {
                {.title = "System Ready", .body = "Shell initialized for device-b-messenger"},
            },
        .navigation_actions =
            {
                shell::ShellNavigationAction::MoveNext,
                shell::ShellNavigationAction::MovePrevious,
                shell::ShellNavigationAction::PrimaryAction,
                shell::ShellNavigationAction::Select,
                shell::ShellNavigationAction::Back,
                shell::ShellNavigationAction::OpenSettings,
                shell::ShellNavigationAction::OpenNotifications,
            },
    };
}

void MockDeviceBPackage::bind_services(ServiceBindingRegistry& bindings,
                                       platform::Logger& logger) const {
    bindings.bind_display(std::make_shared<MockDisplayService>(services::DisplayInfo {
        .width = 480,
        .height = 320,
        .touch = false,
        .layout_class = "wide",
        .surface_description = "480x320 wide display",
    }));
    bindings.bind_input(std::make_shared<MockInputService>(services::InputInfo {
        .keyboard = true,
        .pointer = false,
        .joystick = false,
        .primary_input = "keyboard",
        .input_mode = "hardware keyboard",
    }));
    bindings.bind_text_input(std::make_shared<services::InMemoryTextInputService>(
        services::TextInputState {
            .available = true,
            .text_entry = true,
            .last_key_code = 0,
            .modifier_mask = 0,
            .last_text = "",
            .source_name = "mock-hardware-keyboard",
        }));
    bindings.bind_radio(std::make_shared<MockRadioService>(false, "absent-backend"));
    bindings.bind_gps(std::make_shared<MockGpsService>(false, "absent-backend"));
    bindings.bind_audio(std::make_shared<MockAudioService>(false, false, "absent-backend"));
    bindings.bind_storage(std::make_shared<MockStorageService>(true, "internal+removable-storage"));
    bindings.bind_power(std::make_shared<MockPowerService>(true, false, "battery telemetry ready"));
    bindings.bind_time(std::make_shared<MockTimeService>(true, "system-clock"));
    bindings.bind_hostlink(
        std::make_shared<MockHostlinkService>(true, true, "usb-cdc", "desktop-bridge"));
    bindings.bind_logging(std::make_shared<services::ConsoleLoggingService>(logger));
    bindings.bind_settings(std::make_shared<services::InMemorySettingsService>());
    bindings.bind_timer(std::make_shared<services::InMemoryTimerService>());
    bindings.bind_notification(std::make_shared<services::ConsoleNotificationService>(logger));
}

std::vector<BoardPackagePtr> make_mock_board_packages() {
    std::vector<BoardPackagePtr> packages;
    packages.push_back(std::make_shared<MockDeviceAPackage>());
    packages.push_back(std::make_shared<MockDeviceBPackage>());
    return packages;
}

}  // namespace aegis::device
