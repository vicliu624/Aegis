#pragma once

#include <memory>
#include <string_view>

#include "device/common/binding/service_binding_registry.hpp"
#include "device/common/profile/device_profile.hpp"
#include "device/common/profile/shell_surface_profile.hpp"
#include "platform/logging/logger.hpp"

namespace aegis::device {

class BoardPackage {
public:
    virtual ~BoardPackage() = default;

    [[nodiscard]] virtual std::string_view package_id() const = 0;
    virtual void initialize_board(platform::Logger& logger) = 0;
    [[nodiscard]] virtual DeviceProfile create_profile() const = 0;
    [[nodiscard]] virtual ShellSurfaceProfile create_shell_surface_profile() const = 0;
    virtual void bind_services(ServiceBindingRegistry& bindings, platform::Logger& logger) const = 0;
};

using BoardPackagePtr = std::shared_ptr<BoardPackage>;

}  // namespace aegis::device
