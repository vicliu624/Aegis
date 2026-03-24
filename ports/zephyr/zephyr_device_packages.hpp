#pragma once

#include <vector>

#include "device/packages/board_package.hpp"

namespace aegis::ports::zephyr {

class ZephyrDeviceAPackage : public device::BoardPackage {
public:
    [[nodiscard]] std::string_view package_id() const override;
    void initialize_board(platform::Logger& logger) override;
    [[nodiscard]] device::DeviceProfile create_profile() const override;
    [[nodiscard]] device::ShellSurfaceProfile create_shell_surface_profile() const override;
    void bind_services(device::ServiceBindingRegistry& bindings,
                       platform::Logger& logger) const override;
};

class ZephyrDeviceBPackage : public device::BoardPackage {
public:
    [[nodiscard]] std::string_view package_id() const override;
    void initialize_board(platform::Logger& logger) override;
    [[nodiscard]] device::DeviceProfile create_profile() const override;
    [[nodiscard]] device::ShellSurfaceProfile create_shell_surface_profile() const override;
    void bind_services(device::ServiceBindingRegistry& bindings,
                       platform::Logger& logger) const override;
};

class ZephyrTloraPagerSx1262Package : public device::BoardPackage {
public:
    [[nodiscard]] std::string_view package_id() const override;
    void initialize_board(platform::Logger& logger) override;
    [[nodiscard]] device::DeviceProfile create_profile() const override;
    [[nodiscard]] device::ShellSurfaceProfile create_shell_surface_profile() const override;
    void bind_services(device::ServiceBindingRegistry& bindings,
                       platform::Logger& logger) const override;
};

[[nodiscard]] std::vector<device::BoardPackagePtr> make_zephyr_board_packages();

}  // namespace aegis::ports::zephyr
