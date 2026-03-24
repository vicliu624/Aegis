#pragma once

#include <filesystem>
#include <vector>

#include "core/app_registry/app_manifest.hpp"
#include "device/common/profile/device_profile.hpp"

namespace aegis::core {

class AppRegistry {
public:
    explicit AppRegistry(std::filesystem::path apps_root);

    void scan();
    [[nodiscard]] const std::vector<AppDescriptor>& apps() const;
    [[nodiscard]] const AppDescriptor* find_by_id(const std::string& app_id) const;
    [[nodiscard]] bool satisfies_requirements(const AppDescriptor& descriptor,
                                              const device::CapabilitySet& capabilities) const;

private:
    std::filesystem::path apps_root_;
    ManifestParser parser_;
    std::vector<AppDescriptor> apps_;
};

}  // namespace aegis::core
