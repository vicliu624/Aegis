#include "core/device_registry/device_registry.hpp"

#include <stdexcept>

namespace aegis::core {

DeviceRegistry::DeviceRegistry(std::vector<device::BoardPackagePtr> packages)
    : packages_(std::move(packages)) {
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
