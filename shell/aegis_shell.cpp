#include "shell/aegis_shell.hpp"

#include <iterator>

namespace aegis::shell {

AegisShell::AegisShell(platform::Logger& logger) : logger_(logger), controller_(logger) {}

void AegisShell::set_presentation_sink(ShellPresentationSink* sink) {
    presentation_sink_ = sink;
}

void AegisShell::bind_services(const device::ServiceBindingRegistry& services) {
    settings_service_ = services.settings();
    notification_service_ = services.notifications();
    reload_settings_app();
}

void AegisShell::configure_for_device(const device::DeviceProfile& profile,
                                      const device::ShellSurfaceProfile& shell_surface_profile) {
    device_profile_ = profile;
    settings_.set_entries(shell_surface_profile.settings_entries);
    status_.set_items(shell_surface_profile.status_items);
    notifications_.set_entries(shell_surface_profile.notification_entries);
    input_.set_supported_actions(shell_surface_profile.navigation_actions);

    logger_.info("shell",
                 "profile=" + profile.device_id + " launcher=" + profile.shell_hints.launcher_style +
                     " input=" + profile.input.primary_input);
    std::string actions = "navigation actions:";
    for (const auto action : input_.supported_actions()) {
        actions += " ";
        actions += std::string(to_string(action));
    }
    logger_.info("shell", actions);
    initialize_system_launcher();
    initialize_settings_app();
    reload_settings_app();
    present_frame("home", "profile=" + profile.device_id + " input=" + profile.input.primary_input);
}

void AegisShell::present_home() const {
    if (startup_page_launcher_) {
        controller_.show_launcher();
        logger_.info("shell", "startup preference redirected home to launcher");
        log_launcher_focus();
        present_frame("launcher", "startup=apps");
        return;
    }
    controller_.boot_to_home();
    logger_.info("shell", "home surface ready");
    present_frame("home", "system ready");
}

void AegisShell::present_system_surfaces() const {
    controller_.show_settings();
    logger_.info("shell", "settings surfaces:");
    for (const auto& entry : settings_.entries()) {
        logger_.info("shell", "  - settings:" + entry.id + " label=" + entry.label);
    }

    controller_.show_notifications();
    logger_.info("shell", "status surfaces:");
    for (const auto& item : status_.items()) {
        logger_.info("shell", "  - status:" + item.id + "=" + item.value);
    }

    logger_.info("shell", "notification surfaces:");
    for (const auto& entry : notifications_.entries()) {
        logger_.info("shell", "  - notification:" + entry.title + " body=" + entry.body);
    }
    present_frame("notifications", "system surfaces initialized");
}

void AegisShell::present_launcher_catalog(const core::AppCatalog& catalog) {
    launcher_ = make_launcher_model(catalog);
    logger_.info("shell", "launcher apps:");
    for (const auto& entry : launcher_.entries()) {
        std::string line = "  - " + entry.descriptor.manifest.display_name + " (" +
                           entry.descriptor.manifest.app_id + ")";
        if (entry.state != LauncherEntryState::Visible && !entry.reason.empty()) {
            line += " [" + entry.reason + "]";
        }
        if (!entry.compatibility_notes.empty()) {
            line += " {";
            line += entry.compatibility_notes.front();
            line += "}";
        }
        logger_.info("shell", line);
        for (const auto& note : entry.compatibility_notes) {
            logger_.info("shell", "    note: " + note);
        }
    }
    controller_.show_launcher();
    log_launcher_focus();
    present_frame("launcher", "apps=" + std::to_string(launcher_.entries().size()));
}

void AegisShell::load_launcher_catalog(const core::AppCatalog& catalog) {
    launcher_ = make_launcher_model(catalog);
    logger_.info("shell",
                 "external app catalog loaded entries=" + std::to_string(launcher_.entries().size()) +
                     " launcher presents system built-ins");
}

void AegisShell::enter_app(const std::string& app_id) const {
    controller_.enter_app(app_id);
    if (presentation_sink_ != nullptr) {
        presentation_sink_->present(build_frame("app", "launch=" + app_id));
    }
}

void AegisShell::return_from_app(const std::string& app_id) const {
    controller_.return_from_app(app_id);
    logger_.info("shell", "returned from app " + app_id);
}

void AegisShell::set_app_foreground_root(std::string root_name) {
    app_foreground_.root_name = std::move(root_name);
    if (controller_.navigation_state().surface() == ShellSurface::AppForeground) {
        present_frame("app", "root=" + app_foreground_.root_name);
    }
}

void AegisShell::append_app_foreground_log(std::string tag, std::string message) {
    std::string line;
    if (!tag.empty()) {
        line += "[";
        line += tag;
        line += "] ";
    }
    line += message;
    app_foreground_.lines.push_back({.text = std::move(line)});
    constexpr std::size_t kMaxForegroundLines = 6;
    if (app_foreground_.lines.size() > kMaxForegroundLines) {
        const auto overflow = app_foreground_.lines.size() - kMaxForegroundLines;
        app_foreground_.lines.erase(app_foreground_.lines.begin(),
                                    std::next(app_foreground_.lines.begin(),
                                              static_cast<long long>(overflow)));
    }
    if (controller_.navigation_state().surface() == ShellSurface::AppForeground) {
        present_frame("app",
                      app_foreground_.root_name.empty()
                          ? "foreground"
                          : "root=" + app_foreground_.root_name);
    }
}

void AegisShell::clear_app_foreground_state() {
    app_foreground_.root_name.clear();
    app_foreground_.lines.clear();
}

std::optional<std::string> AegisShell::handle_action(ShellNavigationAction action) {
    if (!input_.supports(action)) {
        logger_.info("shell", "ignored unsupported action " + std::string(to_string(action)));
        return std::nullopt;
    }

    switch (action) {
        case ShellNavigationAction::MoveNext:
            if (controller_.navigation_state().surface() == ShellSurface::Launcher && focus_system_launcher_next()) {
                log_launcher_focus();
            }
            if (controller_.navigation_state().surface() == ShellSurface::Settings &&
                settings_app_.focus_next(launcher_focus_wrap_)) {
                present_frame("settings", "built-in settings");
            }
            return std::nullopt;
        case ShellNavigationAction::MovePrevious:
            if (controller_.navigation_state().surface() == ShellSurface::Launcher &&
                focus_system_launcher_previous()) {
                log_launcher_focus();
            }
            if (controller_.navigation_state().surface() == ShellSurface::Settings &&
                settings_app_.focus_previous(launcher_focus_wrap_)) {
                present_frame("settings", "built-in settings");
            }
            return std::nullopt;
        case ShellNavigationAction::PrimaryAction:
            if (controller_.navigation_state().surface() == ShellSurface::Settings) {
                apply_settings_app();
                present_frame("settings", "built-in settings");
            }
            if (controller_.navigation_state().surface() == ShellSurface::Launcher) {
                if (const auto* focused = focused_system_launcher_entry(); focused != nullptr &&
                    focused->target == SystemLauncherTarget::Settings) {
                    controller_.show_settings();
                    reload_settings_app();
                    log_surface_summary();
                    present_frame("settings", "built-in settings");
                }
            }
            return std::nullopt;
        case ShellNavigationAction::Select:
            if (controller_.navigation_state().surface() == ShellSurface::Home) {
                controller_.show_launcher();
                log_surface_summary();
                log_launcher_focus();
                present_frame("launcher", "opened from home");
                return std::nullopt;
            }
            if (controller_.navigation_state().surface() == ShellSurface::Launcher) {
                if (const auto* focused = focused_system_launcher_entry(); focused != nullptr) {
                    logger_.info("shell", "launcher selected builtin " + focused->id);
                    if (focused->target == SystemLauncherTarget::Settings) {
                        controller_.show_settings();
                        reload_settings_app();
                        log_surface_summary();
                        present_frame("settings", "built-in settings");
                    }
                    return std::nullopt;
                }
            }
            if (controller_.navigation_state().surface() == ShellSurface::Settings) {
                if (settings_app_.cycle_focused_option()) {
                    present_frame("settings", "built-in settings");
                }
            }
            return std::nullopt;
        case ShellNavigationAction::Back:
            switch (controller_.navigation_state().surface()) {
                case ShellSurface::Home:
                    controller_.boot_to_home();
                    log_surface_summary();
                    present_frame("home", "back");
                    break;
                case ShellSurface::Launcher:
                    controller_.boot_to_home();
                    log_surface_summary();
                    present_frame("home", "back");
                    break;
                case ShellSurface::Settings:
                case ShellSurface::Notifications:
                case ShellSurface::Recovery:
                    controller_.show_launcher();
                    log_surface_summary();
                    log_launcher_focus();
                    present_frame("launcher", "back");
                    break;
                case ShellSurface::AppForeground:
                    logger_.info("shell", "back ignored while app owns foreground");
                    break;
            }
            return std::nullopt;
        case ShellNavigationAction::OpenMenu:
            controller_.boot_to_home();
            log_surface_summary();
            present_frame("home", "menu");
            return std::nullopt;
        case ShellNavigationAction::OpenSettings:
            controller_.show_settings();
            reload_settings_app();
            log_surface_summary();
            present_frame("settings", "built-in settings");
            return std::nullopt;
        case ShellNavigationAction::OpenNotifications:
            controller_.show_notifications();
            log_surface_summary();
            present_frame("notifications", "inbox");
            return std::nullopt;
    }

    return std::nullopt;
}

const SettingsModel& AegisShell::settings_model() const {
    return settings_;
}

const SettingsAppModel& AegisShell::settings_app_model() const {
    return settings_app_;
}

const StatusModel& AegisShell::status_model() const {
    return status_;
}

const NotificationModel& AegisShell::notification_model() const {
    return notifications_;
}

const ShellNavigationState& AegisShell::navigation_state() const {
    return controller_.navigation_state();
}

const ShellInputModel& AegisShell::input_model() const {
    return input_;
}

LauncherModel AegisShell::make_launcher_model(const core::AppCatalog& catalog) {
    LauncherModel launcher;
    std::vector<LauncherEntry> entries;
    for (const auto& catalog_entry : catalog.entries()) {
        LauncherEntry entry {.descriptor = catalog_entry.descriptor};
        switch (catalog_entry.state) {
            case core::AppCatalogEntryState::Visible:
                entry.state = LauncherEntryState::Visible;
                break;
            case core::AppCatalogEntryState::Hidden:
                entry.state = LauncherEntryState::Hidden;
                break;
            case core::AppCatalogEntryState::Incompatible:
                entry.state = LauncherEntryState::Incompatible;
                break;
            case core::AppCatalogEntryState::Invalid:
                entry.state = LauncherEntryState::Invalid;
                break;
        }
        entry.reason = catalog_entry.reason;
        entry.compatibility_notes = catalog_entry.compatibility_notes;
        entries.push_back(std::move(entry));
    }
    launcher.set_entries(std::move(entries));
    return launcher;
}

void AegisShell::present_frame(std::string headline, std::string detail) const {
    if (presentation_sink_ == nullptr) {
        return;
    }

    presentation_sink_->present(build_frame(std::move(headline), std::move(detail)));
}

ShellPresentationFrame AegisShell::build_frame(std::string headline, std::string detail) const {
    ShellPresentationFrame frame {
        .surface = controller_.navigation_state().surface(),
        .headline = std::move(headline),
        .detail = std::move(detail),
    };

    switch (frame.surface) {
        case ShellSurface::Home:
            frame.context = "resident shell";
            frame.lines.push_back({.text = "Aegis ready", .emphasized = true});
            frame.lines.push_back({.text = "Select opens launcher"});
            frame.lines.push_back({.text = "F1 settings  F2 notifications"});
            frame.lines.push_back({.text = "Menu returns home"});
            break;
        case ShellSurface::Launcher: {
            frame.context = "system built-ins";
            frame.headline = "launcher";
            frame.detail = "apps=" + std::to_string(system_launcher_.size());
            for (std::size_t index = 0; index < system_launcher_.size() && frame.lines.size() < 8; ++index) {
                const auto& entry = system_launcher_[index];
                std::string line =
                    (index == system_launcher_focus_index_ ? "[*] " : "[ ] ") +
                    std::to_string(index + 1) + " " + entry.label;
                if (!entry.subtitle.empty()) {
                    line += " [" + entry.subtitle + "]";
                }
                frame.lines.push_back({.text = std::move(line),
                                       .emphasized = index == system_launcher_focus_index_});
            }
            if (frame.lines.empty()) {
                frame.lines.push_back({.text = "No launcher entries", .emphasized = true});
            }
            break;
        }
        case ShellSurface::Settings:
            frame.context = "built-in settings app";
            frame.detail = "dirty=" + std::to_string(settings_app_.dirty_count());
            for (std::size_t index = 0; index < settings_app_.entries().size() && frame.lines.size() < 8; ++index) {
                const auto& entry = settings_app_.entries()[index];
                if (!entry.visible) {
                    continue;
                }
                std::string line =
                    (index == settings_app_.focus_index() ? "[*] " : "[ ] ") +
                    entry.label + ": " + settings_app_.value_for(entry);
                if (entry.draft_index != entry.committed_index) {
                    line += " *";
                }
                frame.lines.push_back(
                    {.text = std::move(line), .emphasized = index == settings_app_.focus_index()});
            }
            if (show_advanced_settings_) {
                for (const auto& entry : settings_.entries()) {
                    if (!entry.visible || frame.lines.size() >= 8) {
                        continue;
                    }
                    frame.lines.push_back({.text = "Section: " + entry.label});
                }
            }
            break;
        case ShellSurface::Notifications:
            frame.context = "status and alerts";
            for (const auto& item : status_.items()) {
                if (frame.lines.size() >= 4) {
                    break;
                }
                frame.lines.push_back({.text = item.id + ": " + item.value});
            }
            for (const auto& entry : notifications_.entries()) {
                if (frame.lines.size() >= 8) {
                    break;
                }
                frame.lines.push_back({.text = entry.title + ": " + entry.body});
            }
            break;
        case ShellSurface::AppForeground:
            frame.context = "native app running";
            if (!controller_.navigation_state().active_app_id().empty()) {
                frame.lines.push_back(
                    {.text = "App: " + controller_.navigation_state().active_app_id(),
                     .emphasized = true});
            }
            if (!app_foreground_.root_name.empty()) {
                frame.lines.push_back({.text = "Root: " + app_foreground_.root_name});
            }
            if (!app_foreground_.lines.empty()) {
                for (const auto& line : app_foreground_.lines) {
                    if (frame.lines.size() >= 8) {
                        break;
                    }
                    frame.lines.push_back(line);
                }
            } else {
                frame.lines.push_back({.text = "App foreground acquired"});
                frame.lines.push_back({.text = "Shell retains host control"});
                frame.lines.push_back({.text = "Back path is system-governed"});
            }
            break;
        case ShellSurface::Recovery:
            frame.context = "return path";
            frame.lines.push_back({.text = "Recovered to launcher", .emphasized = true});
            frame.lines.push_back({.text = "Foreground ownership reclaimed"});
            break;
    }

    return frame;
}

void AegisShell::initialize_system_launcher() {
    system_launcher_ = {
        {.id = "apps", .label = "Apps", .subtitle = "catalog soon"},
        {.id = "files", .label = "Files", .subtitle = "storage soon"},
        {.id = "settings", .label = "Settings", .subtitle = "built-in app", .available = true, .target = SystemLauncherTarget::Settings},
        {.id = "radio", .label = "Radio", .subtitle = "soon"},
        {.id = "tools", .label = "Tools", .subtitle = "soon"},
        {.id = "device", .label = "Device", .subtitle = "soon"},
    };
    system_launcher_focus_index_ = 0;
    for (std::size_t index = 0; index < system_launcher_.size(); ++index) {
        if (system_launcher_[index].available) {
            system_launcher_focus_index_ = index;
            break;
        }
    }
}

const AegisShell::SystemLauncherEntry* AegisShell::focused_system_launcher_entry() const {
    if (system_launcher_.empty() || system_launcher_focus_index_ >= system_launcher_.size()) {
        return nullptr;
    }
    return &system_launcher_[system_launcher_focus_index_];
}

bool AegisShell::focus_system_launcher_next() {
    if (system_launcher_.empty()) {
        return false;
    }
    if (launcher_focus_wrap_) {
        system_launcher_focus_index_ = (system_launcher_focus_index_ + 1) % system_launcher_.size();
        return true;
    }
    if ((system_launcher_focus_index_ + 1) >= system_launcher_.size()) {
        return false;
    }
    ++system_launcher_focus_index_;
    return true;
}

bool AegisShell::focus_system_launcher_previous() {
    if (system_launcher_.empty()) {
        return false;
    }
    if (launcher_focus_wrap_) {
        system_launcher_focus_index_ =
            (system_launcher_focus_index_ + system_launcher_.size() - 1) % system_launcher_.size();
        return true;
    }
    if (system_launcher_focus_index_ == 0) {
        return false;
    }
    --system_launcher_focus_index_;
    return true;
}

void AegisShell::initialize_settings_app() {
    settings_app_.set_entries({
        {.id = "screen_brightness", .label = "Screen Brightness", .options = {"20%", "40%", "60%", "80%", "100%"}, .committed_index = 3, .draft_index = 3},
        {.id = "keyboard_backlight", .label = "Keyboard Backlight", .options = {"Off", "On"}, .committed_index = 1, .draft_index = 1},
        {.id = "screen_timeout", .label = "Screen Timeout", .options = {"15 sec", "30 sec", "1 min", "2 min", "5 min", "Never"}, .committed_index = 2, .draft_index = 2},
        {.id = "bluetooth", .label = "Bluetooth", .options = {"Off", "On"}, .committed_index = 0, .draft_index = 0},
        {.id = "gps", .label = "GPS", .options = {"Off", "On"}, .committed_index = 1, .draft_index = 1},
        {.id = "timezone", .label = "Time Zone", .options = {"UTC-08:00", "UTC", "Asia/Shanghai", "Asia/Tokyo", "Europe/Berlin", "America/New_York"}, .committed_index = 2, .draft_index = 2},
        {.id = "time_format", .label = "Time Format", .options = {"12-hour", "24-hour"}, .committed_index = 1, .draft_index = 1},
    });
}

void AegisShell::reload_settings_app() {
    settings_app_.load(settings_service_);

    if (const auto* startup_page = settings_app_.find_entry("startup_page"); startup_page != nullptr) {
        startup_page_launcher_ = settings_app_.value_for(*startup_page) == "Apps";
    }
    if (const auto* focus_wrap = settings_app_.find_entry("focus_wrap"); focus_wrap != nullptr) {
        launcher_focus_wrap_ = settings_app_.value_for(*focus_wrap) == "On";
    }
    if (const auto* advanced_sections = settings_app_.find_entry("advanced_sections");
        advanced_sections != nullptr) {
        show_advanced_settings_ = settings_app_.value_for(*advanced_sections) == "On";
    }
}

void AegisShell::apply_settings_app() {
    if (!settings_app_.apply(settings_service_)) {
        logger_.info("shell", "settings app apply skipped no changes");
        return;
    }
    reload_settings_app();
    std::string summary = "settings app applied";
    for (const auto& entry : settings_app_.entries()) {
        summary += " ";
        summary += entry.id;
        summary += "=";
        summary += settings_app_.value_for(entry);
    }
    logger_.info("shell", summary);
    if (notification_service_ != nullptr) {
        notification_service_->notify("Settings", "Built-in settings applied");
    }
}

void AegisShell::log_launcher_focus() const {
    if (const auto* focused = focused_system_launcher_entry()) {
        logger_.info("shell",
                     "launcher focus=" + focused->id + " index=" +
                         std::to_string(system_launcher_focus_index_));
        present_frame("launcher", "system");
    }
}

void AegisShell::log_surface_summary() const {
    switch (controller_.navigation_state().surface()) {
        case ShellSurface::Home:
            logger_.info("shell", "home surface active");
            break;
        case ShellSurface::Launcher:
            logger_.info("shell", "launcher surface active");
            break;
        case ShellSurface::Settings:
            logger_.info("shell", "settings surface active");
            break;
        case ShellSurface::Notifications:
            logger_.info("shell", "notifications surface active");
            break;
        case ShellSurface::AppForeground:
            logger_.info("shell", "app foreground surface active");
            break;
        case ShellSurface::Recovery:
            logger_.info("shell", "recovery surface active");
            break;
    }
}

}  // namespace aegis::shell
