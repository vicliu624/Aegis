#pragma once

#include <string>
#include <vector>

#include "core/app_registry/app_manifest.hpp"
#include "core/app_registry/app_runtime_policy.hpp"
#include "device/common/profile/device_profile.hpp"

namespace aegis::core {

struct AppCompatibilityReport {
    std::vector<std::string> notes;
};

class AppCompatibilityEvaluator {
public:
    [[nodiscard]] AppCompatibilityReport evaluate(const AppDescriptor& descriptor,
                                                  const device::DeviceProfile& profile,
                                                  const AppRuntimePolicy& runtime_policy) const;
};

}  // namespace aegis::core
