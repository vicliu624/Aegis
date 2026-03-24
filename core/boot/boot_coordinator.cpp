#include "core/boot/boot_coordinator.hpp"

namespace aegis::core {

BootCoordinator::BootCoordinator(std::shared_ptr<AppPackageSource> app_source,
                                 platform::Logger& logger,
                                 std::vector<device::BoardPackagePtr> board_packages)
    : app_source_(std::move(app_source)),
      logger_(logger),
      device_registry_(std::move(board_packages)) {}

BootArtifacts BootCoordinator::boot(const std::string& package_id) {
    logger_.info("boot", "resolve device package " + package_id);
    auto package = device_registry_.resolve(package_id);
    logger_.info("boot", "initialize board package " + std::string(package->package_id()));
    package->initialize_board(logger_);

    device::ServiceBindingRegistry bindings;
    logger_.info("boot", "bind services");
    package->bind_services(bindings, logger_);
    logger_.info("boot", "create device profile");
    const auto profile = package->create_profile();
    logger_.info("boot", "create shell surface profile");
    const auto shell_surface_profile = package->create_shell_surface_profile();

    AppRegistry registry(app_source_);
    logger_.info("boot", "scan app registry");
    registry.scan();
    logger_.info("boot", "app registry scanned count=" + std::to_string(registry.apps().size()));

    return BootArtifacts {
        .board_package = std::move(package),
        .device_profile = profile,
        .shell_surface_profile = shell_surface_profile,
        .service_bindings = std::move(bindings),
        .app_registry = std::move(registry),
    };
}

std::vector<std::string> BootCoordinator::available_device_profiles() const {
    return device_registry_.known_package_ids();
}

}  // namespace aegis::core
