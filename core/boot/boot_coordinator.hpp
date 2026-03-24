#pragma once

#include <memory>
#include <string>

#include "core/app_registry/app_registry.hpp"
#include "core/device_registry/device_registry.hpp"
#include "device/common/binding/service_binding_registry.hpp"
#include "device/common/profile/device_profile.hpp"
#include "platform/logging/logger.hpp"

namespace aegis::core {

struct BootArtifacts {
    device::BoardPackagePtr board_package;
    device::DeviceProfile device_profile;
    device::ShellSurfaceProfile shell_surface_profile;
    device::ServiceBindingRegistry service_bindings;
    AppRegistry app_registry;
};

class BootCoordinator {
public:
    BootCoordinator(std::shared_ptr<AppPackageSource> app_source,
                    platform::Logger& logger,
                    std::vector<device::BoardPackagePtr> board_packages);

    [[nodiscard]] BootArtifacts boot(const std::string& package_id);
    [[nodiscard]] std::vector<std::string> available_device_profiles() const;

private:
    std::shared_ptr<AppPackageSource> app_source_;
    platform::Logger& logger_;
    DeviceRegistry device_registry_;
};

}  // namespace aegis::core
