#include "core/boot/boot_coordinator.hpp"

namespace aegis::core {

BootCoordinator::BootCoordinator(std::filesystem::path apps_root) : apps_root_(std::move(apps_root)) {}

BootArtifacts BootCoordinator::boot(const std::string& package_id) {
    auto package = device_registry_.resolve(package_id);
    package->initialize_board();

    device::ServiceBindingRegistry bindings;
    package->bind_services(bindings);

    AppRegistry registry(apps_root_);
    registry.scan();

    return BootArtifacts {
        .board_package = std::move(package),
        .device_profile = device_registry_.resolve(package_id)->create_profile(),
        .service_bindings = std::move(bindings),
        .app_registry = std::move(registry),
    };
}

std::vector<std::string> BootCoordinator::available_device_profiles() const {
    return device_registry_.known_package_ids();
}

}  // namespace aegis::core
