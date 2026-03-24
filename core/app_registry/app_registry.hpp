#pragma once

#include <memory>
#include <vector>

#include "core/app_registry/app_manifest.hpp"
#include "core/app_registry/app_package_source.hpp"
#include "device/common/profile/device_profile.hpp"

namespace aegis::core {

class AppRegistry {
public:
    explicit AppRegistry(std::shared_ptr<AppPackageSource> app_source);

    void scan();
    [[nodiscard]] const std::vector<AppDescriptor>& apps() const;
    [[nodiscard]] const AppDescriptor* find_by_id(const std::string& app_id) const;
    [[nodiscard]] bool satisfies_requirements(const AppDescriptor& descriptor,
                                              const device::CapabilitySet& capabilities) const;
    [[nodiscard]] bool is_abi_compatible(const AppDescriptor& descriptor,
                                         std::uint32_t runtime_abi_version) const;

private:
    std::shared_ptr<AppPackageSource> app_source_;
    ManifestParser parser_;
    std::vector<AppDescriptor> apps_;
};

}  // namespace aegis::core
