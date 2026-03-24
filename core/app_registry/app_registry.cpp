#include "core/app_registry/app_registry.hpp"

#include <stdexcept>

namespace aegis::core {

AppRegistry::AppRegistry(std::filesystem::path apps_root) : apps_root_(std::move(apps_root)) {}

void AppRegistry::scan() {
    apps_.clear();

    if (!std::filesystem::exists(apps_root_)) {
        throw std::runtime_error("apps directory does not exist: " + apps_root_.string());
    }

    for (const auto& entry : std::filesystem::directory_iterator(apps_root_)) {
        if (!entry.is_directory()) {
            continue;
        }

        const auto app_dir = entry.path();
        const auto manifest_path = app_dir / "manifest.json";
        if (!std::filesystem::exists(manifest_path)) {
            continue;
        }

        AppDescriptor descriptor;
        descriptor.app_dir = app_dir;
        descriptor.manifest_path = manifest_path;
        descriptor.binary_path = app_dir / "app.llext";
        descriptor.manifest = parser_.parse_file(manifest_path);
        descriptor.validated = true;
        apps_.push_back(std::move(descriptor));
    }
}

const std::vector<AppDescriptor>& AppRegistry::apps() const {
    return apps_;
}

const AppDescriptor* AppRegistry::find_by_id(const std::string& app_id) const {
    for (const auto& app : apps_) {
        if (app.manifest.app_id == app_id) {
            return &app;
        }
    }

    return nullptr;
}

bool AppRegistry::satisfies_requirements(const AppDescriptor& descriptor,
                                         const device::CapabilitySet& capabilities) const {
    for (const auto capability : descriptor.manifest.required_capabilities) {
        if (!capabilities.has(capability)) {
            return false;
        }
    }

    return true;
}

}  // namespace aegis::core
