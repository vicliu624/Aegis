#pragma once

#include <optional>
#include <vector>

#include "core/app_registry/app_catalog.hpp"
#include "device/common/profile/shell_surface_profile.hpp"
#include "device/common/profile/device_profile.hpp"
#include "platform/logging/logger.hpp"
#include "shell/control/shell_controller.hpp"
#include "shell/control/shell_input_model.hpp"
#include "shell/launcher/launcher_model.hpp"
#include "shell/presentation/shell_presentation_sink.hpp"
#include "shell/settings/settings_model.hpp"
#include "shell/status/notification_model.hpp"
#include "shell/status/status_model.hpp"

namespace aegis::shell {

class AegisShell {
public:
    explicit AegisShell(platform::Logger& logger);

    void set_presentation_sink(ShellPresentationSink* sink);
    void configure_for_device(const device::DeviceProfile& profile,
                              const device::ShellSurfaceProfile& shell_surface_profile);
    void present_home() const;
    void present_system_surfaces() const;
    void present_launcher_catalog(const core::AppCatalog& catalog);
    void load_launcher_catalog(const core::AppCatalog& catalog);
    void enter_app(const std::string& app_id) const;
    void return_from_app(const std::string& app_id) const;
    void set_app_foreground_root(std::string root_name);
    void append_app_foreground_log(std::string tag, std::string message);
    void clear_app_foreground_state();
    [[nodiscard]] std::optional<std::string> handle_action(ShellNavigationAction action);

    [[nodiscard]] const SettingsModel& settings_model() const;
    [[nodiscard]] const StatusModel& status_model() const;
    [[nodiscard]] const NotificationModel& notification_model() const;
    [[nodiscard]] const ShellNavigationState& navigation_state() const;
    [[nodiscard]] const ShellInputModel& input_model() const;

private:
    [[nodiscard]] static LauncherModel make_launcher_model(const core::AppCatalog& catalog);
    void present_frame(std::string headline, std::string detail) const;
    [[nodiscard]] ShellPresentationFrame build_frame(std::string headline, std::string detail) const;
    void log_launcher_focus() const;
    void log_surface_summary() const;

    platform::Logger& logger_;
    struct AppForegroundState {
        std::string root_name;
        std::vector<ShellPresentationLine> lines;
    };
    mutable ShellController controller_;
    ShellPresentationSink* presentation_sink_ {nullptr};
    ShellInputModel input_;
    SettingsModel settings_;
    StatusModel status_;
    NotificationModel notifications_;
    LauncherModel launcher_;
    AppForegroundState app_foreground_;
};

}  // namespace aegis::shell
