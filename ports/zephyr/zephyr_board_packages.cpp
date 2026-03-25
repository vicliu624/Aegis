#include "ports/zephyr/zephyr_board_packages.hpp"

#include <memory>

#include "device/common/binding/mock_services.hpp"
#include "device/common/capability/capability_set.hpp"
#include "device/packages/mock_device_packages.hpp"
#include "ports/zephyr/zephyr_board_backend_config.hpp"
#include "ports/zephyr/zephyr_board_support.hpp"
#include "ports/zephyr/zephyr_build_config.hpp"
#include "services/audio/zephyr_audio_service.hpp"
#include "services/gps/zephyr_gps_service.hpp"
#include "services/hostlink/zephyr_hostlink_service.hpp"
#include "services/logging/console_logging_service.hpp"
#include "services/notification/zephyr_notification_service.hpp"
#include "services/power/zephyr_power_service.hpp"
#include "services/radio/zephyr_radio_service.hpp"
#include "services/settings/zephyr_settings_service.hpp"
#include "services/storage/zephyr_storage_service.hpp"
#include "services/text/in_memory_text_input_service.hpp"
#include "services/text/zephyr_text_input_service.hpp"
#include "services/time/zephyr_time_service.hpp"
#include "services/timer/zephyr_timer_service.hpp"
#include "services/ui/profile_ui_service.hpp"
#include "services/ui/zephyr_ui_service.hpp"

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

device::DeviceProfile make_profile_a() {
    device::MockDeviceAPackage package;
    return package.create_profile();
}

device::ShellSurfaceProfile make_shell_profile_a() {
    device::MockDeviceAPackage package;
    return package.create_shell_surface_profile();
}

device::DeviceProfile make_profile_b() {
    device::MockDeviceBPackage package;
    return package.create_profile();
}

device::ShellSurfaceProfile make_shell_profile_b() {
    device::MockDeviceBPackage package;
    return package.create_shell_surface_profile();
}

device::DeviceProfile make_tlora_pager_profile() {
    const auto config = make_tlora_pager_sx1262_backend_config();
    return device::DeviceProfile {
        .device_id = config.profile_device_id,
        .board_family = config.board_family,
        .display_topology_name = "single-display-480x222-st7796",
        .input_topology_name = "rotary-plus-tca8418-keyboard",
        .display = {.width = config.display_width,
                    .height = config.display_height,
                    .touch = false,
                    .layout_class = "pager-landscape"},
        .input = {.keyboard = true,
                  .pointer = false,
                  .joystick = true,
                  .primary_input = "keyboard+rotary"},
        .storage = {.persistent_storage = true, .removable_storage = config.removable_storage},
        .power = {.battery_status = true, .low_power_mode = true},
        .comm = {.radio = true, .gps = true, .usb_hostlink = true},
        .capabilities =
            device::CapabilitySet {
                cap(device::CapabilityId::Display,
                    device::CapabilityLevel::Full,
                    "ST7796 480x222 landscape display"),
                cap(device::CapabilityId::KeyboardInput,
                    device::CapabilityLevel::Full,
                    "TCA8418 keyboard through Zephyr input driver and board keymap"),
                cap(device::CapabilityId::JoystickInput,
                    device::CapabilityLevel::Full,
                    "gpio-qdec rotary encoder plus center key navigation"),
                cap(device::CapabilityId::RadioMessaging,
                    device::CapabilityLevel::Full,
                    "SX1262 LoRa transceiver"),
                cap(device::CapabilityId::GpsFix,
                    device::CapabilityLevel::Full,
                    "UBlox MIA-M10Q GPS"),
                cap(device::CapabilityId::AudioOutput,
                    device::CapabilityLevel::Full,
                    "ES8311 codec + NS4150B amplifier"),
                cap(device::CapabilityId::MicrophoneInput,
                    device::CapabilityLevel::Degraded,
                    "ES8311 codec microphone path"),
                cap(device::CapabilityId::BatteryStatus,
                    device::CapabilityLevel::Full,
                    "BQ25896/BQ27220 telemetry"),
                cap(device::CapabilityId::RemovableStorage,
                    device::CapabilityLevel::Full,
                    "microSD slot"),
                cap(device::CapabilityId::UsbHostlink,
                    device::CapabilityLevel::Full,
                    "USB CDC hostlink path"),
            },
        .shell_hints = {.launcher_style = "pager-grid", .status_density = "dense"},
        .runtime_limits = {.max_app_memory_bytes = 512 * 1024,
                           .max_ui_memory_bytes = 128 * 1024,
                           .max_foreground_apps = 1},
    };
}

device::ShellSurfaceProfile make_tlora_pager_shell_profile() {
    return device::ShellSurfaceProfile {
        .settings_entries =
            {
                {.id = "system", .label = "System"},
                {.id = "display", .label = "Display"},
                {.id = "radio", .label = "LoRa"},
                {.id = "gps", .label = "GPS"},
                {.id = "audio", .label = "Audio"},
                {.id = "storage", .label = "Storage"},
                {.id = "power", .label = "Power"},
            },
        .status_items =
            {
                {.id = "display", .value = "480x222-st7796"},
                {.id = "input", .value = "keyboard+rotary"},
                {.id = "keyboard", .value = "tca8418-input-driver"},
                {.id = "radio", .value = "sx1262"},
                {.id = "gps", .value = "mia-m10q"},
                {.id = "audio", .value = "es8311"},
            },
        .notification_entries =
            {
                {.title = "System Ready", .body = "Shell initialized for LilyGo T-LoRa-Pager"},
            },
        .navigation_actions =
            {
                shell::ShellNavigationAction::MoveNext,
                shell::ShellNavigationAction::MovePrevious,
                shell::ShellNavigationAction::Select,
                shell::ShellNavigationAction::Back,
                shell::ShellNavigationAction::OpenMenu,
                shell::ShellNavigationAction::OpenSettings,
                shell::ShellNavigationAction::OpenNotifications,
            },
    };
}

void bind_common_services(device::ServiceBindingRegistry& bindings, platform::Logger& logger) {
    bindings.bind_logging(std::make_shared<services::ConsoleLoggingService>(logger));
    bindings.bind_settings(std::make_shared<services::ZephyrSettingsService>());
    bindings.bind_timer(std::make_shared<services::ZephyrTimerService>());
    bindings.bind_notification(std::make_shared<services::ZephyrNotificationService>(logger));
}

std::shared_ptr<services::IDisplayService> make_display_service(
    const ZephyrBoardBackendConfig& config,
    std::string detail) {
    (void)detail;
    return std::make_shared<services::ZephyrDisplayService>(config);
}

std::shared_ptr<services::IInputService> make_input_service(const ZephyrBoardBackendConfig& config,
                                                            std::string detail) {
    (void)detail;
    return std::make_shared<services::ZephyrInputService>(config);
}

}  // namespace

std::string_view ZephyrDeviceAPackage::package_id() const {
    return "zephyr_device_a";
}

void ZephyrDeviceAPackage::initialize_board(platform::Logger& logger) {
    (void)initialize_board_runtime(package_id(), logger);
    logger.info("board", "initialize Zephyr device A orchestration");
}

device::DeviceProfile ZephyrDeviceAPackage::create_profile() const {
    return make_profile_a();
}

device::ShellSurfaceProfile ZephyrDeviceAPackage::create_shell_surface_profile() const {
    return make_shell_profile_a();
}

void ZephyrDeviceAPackage::bind_services(device::ServiceBindingRegistry& bindings,
                                         platform::Logger& logger) const {
    const auto config = make_device_a_backend_config();
    bindings.bind_display(make_display_service(config, "320x240 compact display"));
    bindings.bind_input(make_input_service(config, "joystick navigation"));
    bindings.bind_text_input(std::make_shared<services::InMemoryTextInputService>(
        services::TextInputState {
            .available = false,
            .text_entry = false,
            .last_key_code = 0,
            .modifier_mask = 0,
            .last_text = "",
            .source_name = "absent-text-input",
        }));
    bindings.bind_radio(std::make_shared<services::ZephyrRadioService>(config));
    bindings.bind_gps(std::make_shared<services::ZephyrGpsService>(config));
    bindings.bind_audio(std::make_shared<services::ZephyrAudioService>(config));
    bindings.bind_storage(std::make_shared<services::ZephyrStorageService>(kAppFsMountPoint, config));
    bindings.bind_power(std::make_shared<services::ZephyrPowerService>(config));
    bindings.bind_time(std::make_shared<services::ZephyrTimeService>());
    bindings.bind_hostlink(std::make_shared<services::ZephyrHostlinkService>(config));
    bind_common_services(bindings, logger);
}

std::string_view ZephyrDeviceBPackage::package_id() const {
    return "zephyr_device_b";
}

void ZephyrDeviceBPackage::initialize_board(platform::Logger& logger) {
    (void)initialize_board_runtime(package_id(), logger);
    logger.info("board", "initialize Zephyr device B orchestration");
}

device::DeviceProfile ZephyrDeviceBPackage::create_profile() const {
    return make_profile_b();
}

device::ShellSurfaceProfile ZephyrDeviceBPackage::create_shell_surface_profile() const {
    return make_shell_profile_b();
}

void ZephyrDeviceBPackage::bind_services(device::ServiceBindingRegistry& bindings,
                                         platform::Logger& logger) const {
    const auto config = make_device_b_backend_config();
    bindings.bind_display(make_display_service(config, "480x320 wide display"));
    bindings.bind_input(make_input_service(config, "hardware keyboard"));
    bindings.bind_text_input(std::make_shared<services::InMemoryTextInputService>(
        services::TextInputState {
            .available = config.keyboard_input,
            .text_entry = config.keyboard_input,
            .last_key_code = 0,
            .modifier_mask = 0,
            .last_text = "",
            .source_name = "zephyr-generic-keyboard",
        }));
    bindings.bind_radio(std::make_shared<services::ZephyrRadioService>(config));
    bindings.bind_gps(std::make_shared<services::ZephyrGpsService>(config));
    bindings.bind_audio(std::make_shared<services::ZephyrAudioService>(config));
    bindings.bind_storage(std::make_shared<services::ZephyrStorageService>(kAppFsMountPoint, config));
    bindings.bind_power(std::make_shared<services::ZephyrPowerService>(config));
    bindings.bind_time(std::make_shared<services::ZephyrTimeService>());
    bindings.bind_hostlink(std::make_shared<services::ZephyrHostlinkService>(config));
    bind_common_services(bindings, logger);
}

std::string_view ZephyrTloraPagerSx1262Package::package_id() const {
    return "zephyr_tlora_pager_sx1262";
}

void ZephyrTloraPagerSx1262Package::initialize_board(platform::Logger& logger) {
    const auto config = make_tlora_pager_sx1262_backend_config();
    logger.info("board", "initialize " + config.backend_id + " on " + config.target_board);
    const bool board_ready = initialize_board_runtime(package_id(), logger);
    logger.info("board",
                std::string("board package runtime ") + (board_ready ? "ready" : "degraded") +
                    " package=" + config.backend_id);
}

device::DeviceProfile ZephyrTloraPagerSx1262Package::create_profile() const {
    return make_tlora_pager_profile();
}

device::ShellSurfaceProfile ZephyrTloraPagerSx1262Package::create_shell_surface_profile() const {
    return make_tlora_pager_shell_profile();
}

void ZephyrTloraPagerSx1262Package::bind_services(device::ServiceBindingRegistry& bindings,
                                                  platform::Logger& logger) const {
    const auto config = make_tlora_pager_sx1262_backend_config();
    bindings.bind_display(make_display_service(
        config,
        "st7796 display binding via " + config.display_device_name));
    bindings.bind_input(make_input_service(
        config,
        "keyboard+rotary binding via " + config.keyboard_device_name + "+" +
            config.rotary_device_name));
    bindings.bind_text_input(std::make_shared<services::ZephyrTextInputService>(config));
    bindings.bind_radio(std::make_shared<services::ZephyrRadioService>(config));
    bindings.bind_gps(std::make_shared<services::ZephyrGpsService>(config));
    bindings.bind_audio(std::make_shared<services::ZephyrAudioService>(config));
    bindings.bind_storage(std::make_shared<services::ZephyrStorageService>(kAppFsMountPoint, config));
    bindings.bind_power(std::make_shared<services::ZephyrPowerService>(config));
    bindings.bind_time(std::make_shared<services::ZephyrTimeService>());
    bindings.bind_hostlink(std::make_shared<services::ZephyrHostlinkService>(config));
    bind_common_services(bindings, logger);
    logger.info("board",
                "services bound for " + config.backend_id + " display=" + config.display_device_name +
                    " keyboard=" + config.keyboard_device_name +
                    " rotary=" + config.rotary_device_name +
                    " gps=" + config.gps_device_name +
                    " radio=" + config.radio_device_name);
}

std::vector<device::BoardPackagePtr> make_zephyr_board_packages() {
    std::vector<device::BoardPackagePtr> packages;
    packages.push_back(std::make_shared<ZephyrTloraPagerSx1262Package>());
    packages.push_back(std::make_shared<ZephyrDeviceAPackage>());
    packages.push_back(std::make_shared<ZephyrDeviceBPackage>());
    return packages;
}

}  // namespace aegis::ports::zephyr
