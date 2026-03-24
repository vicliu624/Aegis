#pragma once

#include "device/common/profile/device_profile.hpp"
#include "shell/launcher/launcher_model.hpp"
#include "shell/settings/settings_model.hpp"

namespace aegis::shell {

class AegisShell {
public:
    void configure_for_device(const device::DeviceProfile& profile);
    void present_launcher(const LauncherModel& launcher) const;
    void return_from_app(const std::string& app_id) const;

    [[nodiscard]] const SettingsModel& settings_model() const;

private:
    SettingsModel settings_;
};

}  // namespace aegis::shell
