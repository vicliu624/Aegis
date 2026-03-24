#pragma once

#include <memory>
#include <string>
#include <vector>

#include "device/packages/board_package.hpp"

namespace aegis::core {

class DeviceRegistry {
public:
    DeviceRegistry();

    void register_package(device::BoardPackagePtr package);
    [[nodiscard]] std::vector<std::string> known_package_ids() const;
    [[nodiscard]] device::BoardPackagePtr resolve(const std::string& package_id) const;

private:
    std::vector<device::BoardPackagePtr> packages_;
};

}  // namespace aegis::core
