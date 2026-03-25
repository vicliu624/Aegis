#include "ports/zephyr/zephyr_board_packages.hpp"

#include <memory>

#include "ports/zephyr/zephyr_board_backend_config.hpp"
#include "ports/zephyr/zephyr_board_descriptors.hpp"
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

std::shared_ptr<services::ITextInputService> make_text_input_service(const ZephyrBoardDescriptor& descriptor) {
    switch (descriptor.text_input_strategy) {
        case ZephyrBoardTextInputStrategy::ZephyrService:
            return std::make_shared<services::ZephyrTextInputService>(descriptor.config);
        case ZephyrBoardTextInputStrategy::InMemoryKeyboard:
            return std::make_shared<services::InMemoryTextInputService>(
                services::TextInputState {
                    .available = descriptor.config.keyboard_input,
                    .text_entry = descriptor.config.keyboard_input,
                    .last_key_code = 0,
                    .modifier_mask = 0,
                    .last_text = "",
                    .source_name = descriptor.text_input_source_name,
                });
        case ZephyrBoardTextInputStrategy::InMemoryUnavailable:
        default:
            return std::make_shared<services::InMemoryTextInputService>(
                services::TextInputState {
                    .available = false,
                    .text_entry = false,
                    .last_key_code = 0,
                    .modifier_mask = 0,
                    .last_text = "",
                    .source_name = descriptor.text_input_source_name,
                });
    }
}

void bind_descriptor_services(const ZephyrBoardDescriptor& descriptor,
                              device::ServiceBindingRegistry& bindings,
                              platform::Logger& logger) {
    const auto& config = descriptor.config;
    bindings.bind_display(make_display_service(config, descriptor.profile.display_topology_name));
    bindings.bind_input(make_input_service(config, descriptor.profile.input_topology_name));
    bindings.bind_text_input(make_text_input_service(descriptor));
    bindings.bind_radio(std::make_shared<services::ZephyrRadioService>(config));
    bindings.bind_gps(std::make_shared<services::ZephyrGpsService>(config));
    bindings.bind_audio(std::make_shared<services::ZephyrAudioService>(config));
    bindings.bind_storage(std::make_shared<services::ZephyrStorageService>(kAppFsMountPoint, config));
    bindings.bind_power(std::make_shared<services::ZephyrPowerService>(config));
    bindings.bind_time(std::make_shared<services::ZephyrTimeService>());
    bindings.bind_hostlink(std::make_shared<services::ZephyrHostlinkService>(config));
    bind_common_services(bindings, logger);
}

}  // namespace

std::string_view ZephyrDeviceAPackage::package_id() const {
    return "zephyr_device_a";
}

void ZephyrDeviceAPackage::initialize_board(platform::Logger& logger) {
    const auto& descriptor = descriptor_for_package(package_id());
    (void)initialize_board_runtime(package_id(), logger);
    logger.info("board", descriptor.bringup.initialization_banner);
}

device::DeviceProfile ZephyrDeviceAPackage::create_profile() const {
    return descriptor_for_package(package_id()).profile;
}

device::ShellSurfaceProfile ZephyrDeviceAPackage::create_shell_surface_profile() const {
    return descriptor_for_package(package_id()).shell_surface_profile;
}

void ZephyrDeviceAPackage::bind_services(device::ServiceBindingRegistry& bindings,
                                         platform::Logger& logger) const {
    bind_descriptor_services(descriptor_for_package(package_id()), bindings, logger);
}

std::string_view ZephyrDeviceBPackage::package_id() const {
    return "zephyr_device_b";
}

void ZephyrDeviceBPackage::initialize_board(platform::Logger& logger) {
    const auto& descriptor = descriptor_for_package(package_id());
    (void)initialize_board_runtime(package_id(), logger);
    logger.info("board", descriptor.bringup.initialization_banner);
}

device::DeviceProfile ZephyrDeviceBPackage::create_profile() const {
    return descriptor_for_package(package_id()).profile;
}

device::ShellSurfaceProfile ZephyrDeviceBPackage::create_shell_surface_profile() const {
    return descriptor_for_package(package_id()).shell_surface_profile;
}

void ZephyrDeviceBPackage::bind_services(device::ServiceBindingRegistry& bindings,
                                         platform::Logger& logger) const {
    bind_descriptor_services(descriptor_for_package(package_id()), bindings, logger);
}

std::string_view ZephyrTloraPagerSx1262Package::package_id() const {
    return "zephyr_tlora_pager_sx1262";
}

void ZephyrTloraPagerSx1262Package::initialize_board(platform::Logger& logger) {
    const auto& descriptor = descriptor_for_package(package_id());
    logger.info("board", descriptor.bringup.initialization_banner);
    const bool board_ready = initialize_board_runtime(package_id(), logger);
    logger.info("board",
                std::string("board package runtime ") + (board_ready ? "ready" : "degraded") +
                    " package=" + descriptor.config.backend_id);
}

device::DeviceProfile ZephyrTloraPagerSx1262Package::create_profile() const {
    return descriptor_for_package(package_id()).profile;
}

device::ShellSurfaceProfile ZephyrTloraPagerSx1262Package::create_shell_surface_profile() const {
    return descriptor_for_package(package_id()).shell_surface_profile;
}

void ZephyrTloraPagerSx1262Package::bind_services(device::ServiceBindingRegistry& bindings,
                                                  platform::Logger& logger) const {
    const auto& descriptor = descriptor_for_package(package_id());
    bind_descriptor_services(descriptor, bindings, logger);
    const auto& config = descriptor.config;
    logger.info("board",
                "services bound for " + config.backend_id + " display=" + config.display_device_name +
                    " keyboard=" + config.keyboard_device_name +
                    " rotary=" + config.rotary_device_name +
                    " gps=" + config.gps_device_name +
                    " radio=" + config.radio_device_name);
}

std::string_view ZephyrTdeckSx1262Package::package_id() const {
    return "zephyr_tdeck_sx1262";
}

void ZephyrTdeckSx1262Package::initialize_board(platform::Logger& logger) {
    const auto& descriptor = descriptor_for_package(package_id());
    logger.info("board", descriptor.bringup.initialization_banner);
    const bool board_ready = initialize_board_runtime(package_id(), logger);
    logger.info("board",
                std::string("board package runtime ") + (board_ready ? "ready" : "degraded") +
                    " package=" + descriptor.config.backend_id);
}

device::DeviceProfile ZephyrTdeckSx1262Package::create_profile() const {
    return descriptor_for_package(package_id()).profile;
}

device::ShellSurfaceProfile ZephyrTdeckSx1262Package::create_shell_surface_profile() const {
    return descriptor_for_package(package_id()).shell_surface_profile;
}

void ZephyrTdeckSx1262Package::bind_services(device::ServiceBindingRegistry& bindings,
                                             platform::Logger& logger) const {
    const auto& descriptor = descriptor_for_package(package_id());
    bind_descriptor_services(descriptor, bindings, logger);
    const auto& config = descriptor.config;
    logger.info("board",
                "services bound for " + config.backend_id + " display=" + config.display_device_name +
                    " keyboard=" + config.keyboard_device_name +
                    " touch-irq=" + std::to_string(config.touch_irq_pin) +
                    " gps=" + config.gps_device_name +
                    " radio=" + config.radio_device_name);
}

std::vector<device::BoardPackagePtr> make_zephyr_board_packages() {
    std::vector<device::BoardPackagePtr> packages;
    packages.push_back(std::make_shared<ZephyrTloraPagerSx1262Package>());
    packages.push_back(std::make_shared<ZephyrTdeckSx1262Package>());
    packages.push_back(std::make_shared<ZephyrDeviceAPackage>());
    packages.push_back(std::make_shared<ZephyrDeviceBPackage>());
    return packages;
}

}  // namespace aegis::ports::zephyr
