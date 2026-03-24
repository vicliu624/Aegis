#pragma once

#include <string>
#include <vector>

#include "core/app_registry/app_manifest.hpp"
#include "core/app_registry/app_registry.hpp"
#include "core/app_registry/app_runtime_policy.hpp"
#include "device/common/profile/device_profile.hpp"

namespace aegis::core {

struct AppAdmissionContext {
    const AppRegistry& registry;
    const device::DeviceProfile& profile;
    std::uint32_t runtime_abi_version {0};
    AppRuntimePolicy runtime_policy;
    std::vector<std::string> active_app_ids;
};

class AppAdmissionPolicy {
public:
    [[nodiscard]] AppAdmissionDecision evaluate(const AppDescriptor& descriptor,
                                                const AppAdmissionContext& context) const;
};

}  // namespace aegis::core
