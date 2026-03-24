#include "shell/aegis_shell.hpp"

#include <iostream>

namespace aegis::shell {

void AegisShell::configure_for_device(const device::DeviceProfile& profile) {
    std::vector<std::string> pages {"system", "display", "storage"};
    if (profile.comm.radio) {
        pages.push_back("radio");
    }
    if (profile.comm.gps) {
        pages.push_back("gps");
    }
    if (profile.comm.usb_hostlink) {
        pages.push_back("hostlink");
    }
    settings_.set_visible_pages(std::move(pages));

    std::cout << "[shell] profile=" << profile.device_id << " launcher="
              << profile.shell_hints.launcher_style << " input=" << profile.input.primary_input
              << '\n';
}

void AegisShell::present_launcher(const LauncherModel& launcher) const {
    std::cout << "[shell] launcher apps:\n";
    for (const auto& app : launcher.apps()) {
        std::cout << "  - " << app.manifest.display_name << " (" << app.manifest.app_id << ")\n";
    }
}

void AegisShell::return_from_app(const std::string& app_id) const {
    std::cout << "[shell] returned from app " << app_id << '\n';
}

const SettingsModel& AegisShell::settings_model() const {
    return settings_;
}

}  // namespace aegis::shell
