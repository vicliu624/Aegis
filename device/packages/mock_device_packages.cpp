#include "device/packages/mock_device_packages.hpp"

#include <iostream>
#include <memory>

#include "device/common/binding/mock_services.hpp"
#include "services/logging/console_logging_service.hpp"
#include "services/notification/console_notification_service.hpp"
#include "services/settings/in_memory_settings_service.hpp"

namespace aegis::device {

namespace {

CapabilityDescriptor cap(CapabilityId id, CapabilityLevel level, std::string detail) {
    return CapabilityDescriptor {.id = id, .level = level, .flags = 0, .detail = std::move(detail)};
}

}  // namespace

std::string_view MockDeviceAPackage::package_id() const {
    return "mock_device_a";
}

void MockDeviceAPackage::initialize_board() {
    std::cout << "[board] initialize mock device A orchestration\n";
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
        .runtime_limits = {.max_app_memory_bytes = 256 * 1024, .max_foreground_apps = 1},
    };
}

void MockDeviceAPackage::bind_services(ServiceBindingRegistry& bindings) const {
    bindings.bind_display(std::make_shared<MockDisplayService>("320x240 compact display"));
    bindings.bind_input(std::make_shared<MockInputService>("joystick navigation"));
    bindings.bind_radio(std::make_shared<MockRadioService>(true, "lora-backend"));
    bindings.bind_gps(std::make_shared<MockGpsService>(true, "uart-gps-backend"));
    bindings.bind_logging(std::make_shared<services::ConsoleLoggingService>());
    bindings.bind_settings(std::make_shared<services::InMemorySettingsService>());
    bindings.bind_notification(std::make_shared<services::ConsoleNotificationService>());
}

std::string_view MockDeviceBPackage::package_id() const {
    return "mock_device_b";
}

void MockDeviceBPackage::initialize_board() {
    std::cout << "[board] initialize mock device B orchestration\n";
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
        .runtime_limits = {.max_app_memory_bytes = 384 * 1024, .max_foreground_apps = 1},
    };
}

void MockDeviceBPackage::bind_services(ServiceBindingRegistry& bindings) const {
    bindings.bind_display(std::make_shared<MockDisplayService>("480x320 wide display"));
    bindings.bind_input(std::make_shared<MockInputService>("hardware keyboard"));
    bindings.bind_radio(std::make_shared<MockRadioService>(false, "absent-backend"));
    bindings.bind_gps(std::make_shared<MockGpsService>(false, "absent-backend"));
    bindings.bind_logging(std::make_shared<services::ConsoleLoggingService>());
    bindings.bind_settings(std::make_shared<services::InMemorySettingsService>());
    bindings.bind_notification(std::make_shared<services::ConsoleNotificationService>());
}

}  // namespace aegis::device
