#pragma once

#include <vector>

#include "device/packages/board_package.hpp"

namespace aegis::device {

class MockDeviceAPackage : public BoardPackage {
public:
    [[nodiscard]] std::string_view package_id() const override;
    void initialize_board(platform::Logger& logger) override;
    [[nodiscard]] DeviceProfile create_profile() const override;
    [[nodiscard]] ShellSurfaceProfile create_shell_surface_profile() const override;
    void bind_services(ServiceBindingRegistry& bindings, platform::Logger& logger) const override;
};

class MockDeviceBPackage : public BoardPackage {
public:
    [[nodiscard]] std::string_view package_id() const override;
    void initialize_board(platform::Logger& logger) override;
    [[nodiscard]] DeviceProfile create_profile() const override;
    [[nodiscard]] ShellSurfaceProfile create_shell_surface_profile() const override;
    void bind_services(ServiceBindingRegistry& bindings, platform::Logger& logger) const override;
};

[[nodiscard]] std::vector<BoardPackagePtr> make_mock_board_packages();

}  // namespace aegis::device
