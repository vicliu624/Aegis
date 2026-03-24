#pragma once

#include <filesystem>
#include <string>

#include "core/app_registry/app_registry.hpp"
#include "core/device_registry/device_registry.hpp"
#include "device/common/binding/service_binding_registry.hpp"
#include "device/common/profile/device_profile.hpp"

namespace aegis::core {

struct BootArtifacts {
    device::BoardPackagePtr board_package;
    device::DeviceProfile device_profile;
    device::ServiceBindingRegistry service_bindings;
    AppRegistry app_registry;
};

class BootCoordinator {
public:
    explicit BootCoordinator(std::filesystem::path apps_root);

    [[nodiscard]] BootArtifacts boot(const std::string& package_id);
    [[nodiscard]] std::vector<std::string> available_device_profiles() const;

private:
    std::filesystem::path apps_root_;
    DeviceRegistry device_registry_;
};

}  // namespace aegis::core
