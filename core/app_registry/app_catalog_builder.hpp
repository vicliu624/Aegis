#pragma once

#include <cstdint>
#include <vector>

#include "core/app_registry/app_admission_policy.hpp"
#include "core/app_registry/app_catalog.hpp"
#include "core/app_registry/app_compatibility_evaluator.hpp"
#include "core/app_registry/app_registry.hpp"
#include "device/common/profile/device_profile.hpp"

namespace aegis::runtime {
class RuntimeLoader;
}

namespace aegis::core {

class AppCatalogBuilder {
public:
    [[nodiscard]] AppCatalog build(const AppRegistry& registry,
                                   const device::DeviceProfile& profile,
                                   const AppAdmissionPolicy& admission_policy,
                                   const AppCompatibilityEvaluator& compatibility_evaluator,
                                   const runtime::RuntimeLoader& runtime_loader,
                                   const AppRuntimePolicy& runtime_policy,
                                   std::uint32_t runtime_abi_version,
                                   const std::vector<std::string>& active_app_ids) const;
};

}  // namespace aegis::core
