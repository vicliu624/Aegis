#pragma once

#include <string>
#include <vector>

#include "core/app_registry/app_manifest.hpp"

namespace aegis::shell {

class LauncherModel {
public:
    void set_apps(std::vector<core::AppDescriptor> apps);
    [[nodiscard]] const std::vector<core::AppDescriptor>& apps() const;

private:
    std::vector<core::AppDescriptor> apps_;
};

}  // namespace aegis::shell
