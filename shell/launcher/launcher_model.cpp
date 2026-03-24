#include "shell/launcher/launcher_model.hpp"

namespace aegis::shell {

void LauncherModel::set_apps(std::vector<core::AppDescriptor> apps) {
    apps_ = std::move(apps);
}

const std::vector<core::AppDescriptor>& LauncherModel::apps() const {
    return apps_;
}

}  // namespace aegis::shell
