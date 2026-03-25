#include "shell/aegis_shell.hpp"

namespace aegis::shell {

AegisShell::AegisShell(platform::Logger& logger) : logger_(logger), controller_(logger) {}

void AegisShell::set_presentation_sink(ShellPresentationSink* sink) {
    presentation_sink_ = sink;
}

void AegisShell::configure_for_device(const device::DeviceProfile& profile,
                                      const device::ShellSurfaceProfile& shell_surface_profile) {
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
    present_frame("home", "profile=" + profile.device_id + " input=" + profile.input.primary_input);
}

void AegisShell::present_home() const {
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
    logger_.info("shell", "launcher catalog loaded entries=" + std::to_string(launcher_.entries().size()));
    if (const auto* focused = launcher_.focused_entry()) {
        logger_.info("shell",
                     "launcher default focus=" + focused->descriptor.manifest.app_id + " index=" +
                         std::to_string(launcher_.focus_index()));
    }
}

void AegisShell::enter_app(const std::string& app_id) const {
    controller_.enter_app(app_id);
}

void AegisShell::return_from_app(const std::string& app_id) const {
    controller_.return_from_app(app_id);
    logger_.info("shell", "returned from app " + app_id);
}

std::optional<std::string> AegisShell::handle_action(ShellNavigationAction action) {
    if (!input_.supports(action)) {
        logger_.info("shell", "ignored unsupported action " + std::string(to_string(action)));
        return std::nullopt;
    }

    switch (action) {
        case ShellNavigationAction::MoveNext:
            if (controller_.navigation_state().surface() == ShellSurface::Launcher && launcher_.focus_next()) {
                log_launcher_focus();
            }
            return std::nullopt;
        case ShellNavigationAction::MovePrevious:
            if (controller_.navigation_state().surface() == ShellSurface::Launcher &&
                launcher_.focus_previous()) {
                log_launcher_focus();
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
                if (const auto* focused = launcher_.focused_entry();
                    focused != nullptr && focused->state == LauncherEntryState::Visible) {
                    logger_.info("shell",
                                 "launcher selected app " + focused->descriptor.manifest.app_id);
                    present_frame("app",
                                  "launch=" + focused->descriptor.manifest.display_name);
                    return focused->descriptor.manifest.app_id;
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
            log_surface_summary();
            present_frame("settings", "system settings");
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
            frame.context = "catalog rotary=nav center=launch";
            frame.headline = "launcher";
            frame.detail = "apps=" + std::to_string(launcher_.entries().size());
            const auto entries = launcher_.entries();
            for (std::size_t index = 0; index < entries.size() && frame.lines.size() < 8; ++index) {
                const auto& entry = entries[index];
                std::string line =
                    (index == launcher_.focus_index() ? "[*] " : "[ ] ") +
                    std::to_string(index + 1) + " " + entry.descriptor.manifest.display_name;
                if (entry.state != LauncherEntryState::Visible && !entry.reason.empty()) {
                    line += " [" + entry.reason + "]";
                }
                frame.lines.push_back(
                    {.text = std::move(line), .emphasized = index == launcher_.focus_index()});
            }
            if (frame.lines.empty()) {
                frame.lines.push_back({.text = "No launcher entries", .emphasized = true});
            } else if (const auto* focused = launcher_.focused_entry();
                       focused != nullptr && !focused->descriptor.manifest.description.empty() &&
                       frame.lines.size() < 8) {
                frame.lines.push_back(
                    shell::ShellPresentationLine {.text = "about: " +
                                                          focused->descriptor.manifest.description});
            }
            break;
        }
        case ShellSurface::Settings:
            frame.context = "device settings";
            for (const auto& entry : settings_.entries()) {
                if (!entry.visible || frame.lines.size() >= 8) {
                    continue;
                }
                frame.lines.push_back({.text = entry.label});
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
            frame.lines.push_back({.text = "Shell retains host control"});
            frame.lines.push_back({.text = "Back path is system-governed"});
            break;
        case ShellSurface::Recovery:
            frame.context = "return path";
            frame.lines.push_back({.text = "Recovered to launcher", .emphasized = true});
            frame.lines.push_back({.text = "Foreground ownership reclaimed"});
            break;
    }

    return frame;
}

void AegisShell::log_launcher_focus() const {
    if (const auto* focused = launcher_.focused_entry()) {
        logger_.info("shell",
                     "launcher focus=" + focused->descriptor.manifest.app_id + " index=" +
                         std::to_string(launcher_.focus_index()));
        present_frame("launcher", "catalog");
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
