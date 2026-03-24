#pragma once

#include "device/packages/board_package.hpp"

namespace aegis::device {

class MockDeviceAPackage : public BoardPackage {
public:
    [[nodiscard]] std::string_view package_id() const override;
    void initialize_board() override;
    [[nodiscard]] DeviceProfile create_profile() const override;
    void bind_services(ServiceBindingRegistry& bindings) const override;
};

class MockDeviceBPackage : public BoardPackage {
public:
    [[nodiscard]] std::string_view package_id() const override;
    void initialize_board() override;
    [[nodiscard]] DeviceProfile create_profile() const override;
    void bind_services(ServiceBindingRegistry& bindings) const override;
};

}  // namespace aegis::device
