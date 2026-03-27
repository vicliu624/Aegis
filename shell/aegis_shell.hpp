#pragma once

#include <optional>
#include <vector>

#include "runtime/host_api/host_api.hpp"
#include "core/app_registry/app_catalog.hpp"
#include "device/common/binding/service_binding_registry.hpp"
#include "device/common/profile/shell_surface_profile.hpp"
#include "device/common/profile/device_profile.hpp"
#include "platform/logging/logger.hpp"
#include "shell/control/shell_controller.hpp"
#include "shell/control/shell_input_model.hpp"
#include "shell/launcher/launcher_model.hpp"
#include "shell/presentation/shell_presentation_sink.hpp"
#include "apps/builtin/settings/settings_app_model.hpp"
#include "shell/settings/settings_model.hpp"
#include "shell/status/notification_model.hpp"
#include "shell/status/status_model.hpp"

namespace aegis::shell {

class AegisShell {
public:
    explicit AegisShell(platform::Logger& logger);

    void set_presentation_sink(ShellPresentationSink* sink);
    void bind_services(const device::ServiceBindingRegistry& services);
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
    void set_app_foreground_page(runtime::ForegroundPagePresentation page);
    void clear_app_foreground_state();
    [[nodiscard]] std::optional<std::string> handle_action(ShellNavigationAction action);
    [[nodiscard]] std::optional<std::string> handle_input_invocation(const ShellInputInvocation& invocation);

    [[nodiscard]] const SettingsModel& settings_model() const;
    [[nodiscard]] const SettingsAppModel& settings_app_model() const;
    [[nodiscard]] const StatusModel& status_model() const;
    [[nodiscard]] const NotificationModel& notification_model() const;
    [[nodiscard]] const ShellNavigationState& navigation_state() const;
    [[nodiscard]] const ShellInputModel& input_model() const;

private:
    struct SystemLauncherEntry {
        std::string id;
        std::string label;
        std::string subtitle;
        std::string fallback_label;
        std::string fallback_subtitle;
        bool available {false};
        bool opens_shell_settings {false};
        std::string command_id;
        ShellPresentationIconSource icon_source {ShellPresentationIconSource::None};
        std::string icon_key;
        std::string package_app_id;
    };

    void initialize_system_launcher();
    void sync_system_launcher_with_catalog(const core::AppCatalog& catalog);
    void initialize_settings_app();
    [[nodiscard]] const SystemLauncherEntry* focused_system_launcher_entry() const;
    bool focus_system_launcher_next();
    bool focus_system_launcher_previous();
    void reload_settings_app();
    void apply_settings_app();
    [[nodiscard]] std::optional<std::string> dispatch_system_launcher_entry(std::size_t index);
    [[nodiscard]] std::optional<std::string> dispatch_system_launcher_command(std::string_view command_id);
    [[nodiscard]] static LauncherModel make_launcher_model(const core::AppCatalog& catalog);
    [[nodiscard]] std::optional<std::string> handle_page_command(std::string_view page_id,
                                                                 std::string_view page_command_id,
                                                                 std::string_view page_state_token);
    void present_frame(std::string headline, std::string detail) const;
    [[nodiscard]] ShellPresentationFrame build_frame(std::string headline, std::string detail) const;
    void log_launcher_focus() const;
    void log_surface_summary() const;

    platform::Logger& logger_;
    device::DeviceProfile device_profile_;
    struct AppForegroundState {
        std::string root_name;
        std::optional<runtime::ForegroundPagePresentation> page;
        std::vector<ShellPresentationLine> lines;
    };
    mutable ShellController controller_;
    ShellPresentationSink* presentation_sink_ {nullptr};
    ShellInputModel input_;
    SettingsModel settings_;
    SettingsAppModel settings_app_;
    StatusModel status_;
    NotificationModel notifications_;
    LauncherModel launcher_;
    std::vector<SystemLauncherEntry> system_launcher_;
    std::size_t system_launcher_focus_index_ {0};
    std::shared_ptr<services::ISettingsService> settings_service_;
    std::shared_ptr<services::INotificationService> notification_service_;
    bool startup_page_launcher_ {false};
    bool launcher_focus_wrap_ {true};
    bool show_advanced_settings_ {false};
    AppForegroundState app_foreground_;
};

}  // namespace aegis::shell
