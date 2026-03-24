#include "core/device_registry/device_registry.hpp"

#include <stdexcept>

#include "device/packages/mock_device_packages.hpp"

namespace aegis::core {

DeviceRegistry::DeviceRegistry() {
    register_package(std::make_shared<device::MockDeviceAPackage>());
    register_package(std::make_shared<device::MockDeviceBPackage>());
}

void DeviceRegistry::register_package(device::BoardPackagePtr package) {
    packages_.push_back(std::move(package));
}

std::vector<std::string> DeviceRegistry::known_package_ids() const {
    std::vector<std::string> ids;
    ids.reserve(packages_.size());
    for (const auto& package : packages_) {
        ids.emplace_back(package->package_id());
    }
    return ids;
}

device::BoardPackagePtr DeviceRegistry::resolve(const std::string& package_id) const {
    for (const auto& package : packages_) {
        if (package->package_id() == package_id) {
            return package;
        }
    }

    throw std::runtime_error("unknown board package: " + package_id);
}

}  // namespace aegis::core
