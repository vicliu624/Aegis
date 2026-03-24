#include "core/aegis_core/aegis_core.hpp"

#include <iostream>
#include <stdexcept>

#include "runtime/host_api/host_api.hpp"

namespace aegis::core {

AegisCore::AegisCore(std::filesystem::path apps_root)
    : apps_root_(std::move(apps_root)), boot_(apps_root_) {}

void AegisCore::boot(const std::string& package_id) {
    boot_artifacts_ = boot_.boot(package_id);
    shell_.configure_for_device(boot_artifacts_->device_profile);

    shell::LauncherModel launcher;
    launcher.set_apps(boot_artifacts_->app_registry.apps());
    shell_.present_launcher(launcher);
    booted_ = true;
}

void AegisCore::run_first_compatible_app() {
    if (!booted_) {
        throw std::runtime_error("core must boot before starting apps");
    }
    if (!boot_artifacts_) {
        throw std::runtime_error("boot artifacts missing");
    }

    const AppDescriptor* selected = nullptr;
    for (const auto& app : boot_artifacts_->app_registry.apps()) {
        if (boot_artifacts_->app_registry.satisfies_requirements(app,
                                                                 boot_artifacts_->device_profile.capabilities)) {
            selected = &app;
            break;
        }
    }

    if (!selected) {
        std::cout << "[core] no compatible apps for profile " << boot_artifacts_->device_profile.device_id
                  << '\n';
        return;
    }

    run_descriptor(*selected);
}

void AegisCore::run_app(const std::string& app_id) {
    if (!booted_) {
        throw std::runtime_error("core must boot before starting apps");
    }
    if (!boot_artifacts_) {
        throw std::runtime_error("boot artifacts missing");
    }

    const auto* descriptor = boot_artifacts_->app_registry.find_by_id(app_id);
    if (!descriptor) {
        throw std::runtime_error("app not found: " + app_id);
    }
    if (!boot_artifacts_->app_registry.satisfies_requirements(*descriptor,
                                                              boot_artifacts_->device_profile.capabilities)) {
        throw std::runtime_error("app not compatible with active capability set: " + app_id);
    }

    run_descriptor(*descriptor);
}

void AegisCore::run_descriptor(const AppDescriptor& descriptor) {
    std::cout << "[core] starting app " << descriptor.manifest.app_id << '\n';
    AppSession session(descriptor);
    session.transition_to(runtime::AppLifecycleState::LoadRequested);

    runtime::HostApi host_api(boot_artifacts_->device_profile,
                              boot_artifacts_->service_bindings,
                              ownership_,
                              session.id());

    runtime_loader_.load(session);
    runtime_loader_.bringup(session, host_api);
    runtime_loader_.teardown(session, ownership_);
    runtime_loader_.unload(session);

    shell_.return_from_app(descriptor.manifest.app_id);
    std::cout << "[core] lifecycle final state=" << runtime::to_string(session.state()) << '\n';
}

}  // namespace aegis::core
