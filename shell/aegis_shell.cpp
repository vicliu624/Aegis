#include "shell/aegis_shell.hpp"

#include <array>
#include <iterator>

namespace aegis::shell {

namespace {

ShellSoftkeySpec make_system_softkey(std::string id,
                                     std::string label,
                                     ShellSoftkeySpec::Role role,
                                     ShellNavigationAction action,
                                     bool emphasized = false) {
    return ShellSoftkeySpec {
        .id = std::move(id),
        .label = std::move(label),
        .visibility = ShellSoftkeySpec::Visibility::Visible,
        .enablement = ShellSoftkeySpec::Enablement::Enabled,
        .role = role,
        .target = ShellSoftkeySpec::DispatchTarget::System,
        .system_action = action,
        .page_command_id = {},
        .emphasized = emphasized,
    };
}

ShellSoftkeySpec make_page_softkey(std::string id,
                                   std::string label,
                                   ShellSoftkeySpec::Role role,
                                   std::string page_command_id,
                                   bool enabled = true,
                                   bool emphasized = false) {
    return ShellSoftkeySpec {
        .id = std::move(id),
        .label = std::move(label),
        .visibility = ShellSoftkeySpec::Visibility::Visible,
        .enablement = enabled ? ShellSoftkeySpec::Enablement::Enabled
                              : ShellSoftkeySpec::Enablement::Disabled,
        .role = role,
        .target = ShellSoftkeySpec::DispatchTarget::Page,
        .system_action = std::nullopt,
        .page_command_id = std::move(page_command_id),
        .emphasized = emphasized,
    };
}

ShellSoftkeySpec::Role role_from_foreground_softkey(std::uint32_t role) {
    switch (role) {
        case AEGIS_UI_SOFTKEY_ROLE_PRIMARY:
            return ShellSoftkeySpec::Role::Primary;
        case AEGIS_UI_SOFTKEY_ROLE_SECONDARY:
            return ShellSoftkeySpec::Role::Secondary;
        case AEGIS_UI_SOFTKEY_ROLE_BACK:
            return ShellSoftkeySpec::Role::Back;
        case AEGIS_UI_SOFTKEY_ROLE_MENU:
            return ShellSoftkeySpec::Role::Menu;
        case AEGIS_UI_SOFTKEY_ROLE_CONFIRM:
            return ShellSoftkeySpec::Role::Confirm;
        case AEGIS_UI_SOFTKEY_ROLE_INFO:
            return ShellSoftkeySpec::Role::Info;
        case AEGIS_UI_SOFTKEY_ROLE_CUSTOM:
        default:
            return ShellSoftkeySpec::Role::Custom;
    }
}

ShellPresentationTemplate template_from_page_template(runtime::ForegroundPageTemplate layout_template) {
    switch (layout_template) {
        case runtime::ForegroundPageTemplate::SimpleList:
            return ShellPresentationTemplate::SimpleList;
        case runtime::ForegroundPageTemplate::ValueList:
            return ShellPresentationTemplate::ValueList;
        case runtime::ForegroundPageTemplate::FileList:
            return ShellPresentationTemplate::FileList;
        case runtime::ForegroundPageTemplate::TextLog:
        default:
            return ShellPresentationTemplate::TextLog;
    }
}

ShellPresentationAccessory accessory_from_page_item(runtime::ForegroundPageItemAccessory accessory) {
    switch (accessory) {
        case runtime::ForegroundPageItemAccessory::File:
            return ShellPresentationAccessory::File;
        case runtime::ForegroundPageItemAccessory::Folder:
            return ShellPresentationAccessory::Folder;
        case runtime::ForegroundPageItemAccessory::None:
        default:
            return ShellPresentationAccessory::None;
    }
}

ShellPresentationIconSource icon_source_from_page_item(runtime::ForegroundPageItemIconSource source) {
    switch (source) {
        case runtime::ForegroundPageItemIconSource::AppAsset:
            return ShellPresentationIconSource::AppPackage;
        case runtime::ForegroundPageItemIconSource::None:
        default:
            return ShellPresentationIconSource::None;
    }
}

std::array<ShellSoftkeySpec, 3> softkeys_from_page(const runtime::ForegroundPagePresentation& page);

const LauncherEntry* find_launcher_entry_by_app_id(const LauncherModel& launcher, std::string_view app_id) {
    for (const auto& entry : launcher.entries()) {
        if (entry.descriptor.manifest.app_id == app_id) {
            return &entry;
        }
    }
    return nullptr;
}

bool launcher_entry_actionable(const LauncherEntry& entry) {
    return entry.state == LauncherEntryState::Visible;
}

std::string launcher_entry_detail(const LauncherEntry& entry) {
    switch (entry.state) {
        case LauncherEntryState::Visible:
            return entry.descriptor.manifest.description;
        case LauncherEntryState::Hidden:
            return "hidden";
        case LauncherEntryState::Incompatible:
        case LauncherEntryState::Invalid:
            return entry.reason.empty() ? "unavailable" : entry.reason;
    }
    return {};
}

void apply_page_to_frame(const runtime::ForegroundPagePresentation& page, ShellPresentationFrame& frame) {
    if (!page.page_id.empty()) {
        frame.page_id = page.page_id;
    }
    if (!page.page_state_token.empty()) {
        frame.page_state_token = page.page_state_token;
    }
    if (!page.title.empty()) {
        frame.headline = page.title;
    }
    frame.context = page.context;
    frame.page_template = template_from_page_template(page.layout_template);
    frame.items.clear();
    frame.lines.clear();
    frame.items.reserve(page.items.size());
    for (const auto& item : page.items) {
        frame.items.push_back({
            .item_id = item.item_id,
            .label = item.label,
            .detail = item.detail,
            .focused = item.focused,
            .emphasized = item.emphasized,
            .warning = item.warning,
            .disabled = item.disabled,
            .accessory = accessory_from_page_item(item.accessory),
            .actionable = false,
            .command_id = {},
            .icon_source = icon_source_from_page_item(item.icon_source),
            .icon_key = item.icon_key,
        });
        if (frame.page_template == ShellPresentationTemplate::TextLog) {
            std::string line = item.label;
            if (!item.detail.empty()) {
                line += " [" + item.detail + "]";
            }
            frame.lines.push_back(
                {.text = std::move(line), .emphasized = item.focused || item.emphasized});
        }
    }
    frame.dialog = {
        .visible = page.dialog.visible,
        .title = page.dialog.title,
        .body = page.dialog.body,
    };
}

void finalize_frame_from_page(const runtime::ForegroundPagePresentation& page,
                              ShellPresentationFrame& frame) {
    apply_page_to_frame(page, frame);
    if (frame.page_state_token.empty()) {
        frame.page_state_token = page.page_state_token;
    }
    frame.softkeys = softkeys_from_page(page);
}

std::array<ShellSoftkeySpec, 3> softkeys_from_page(const runtime::ForegroundPagePresentation& page) {
    std::array<ShellSoftkeySpec, 3> softkeys {
        make_system_softkey("page-left-hidden",
                            "",
                            ShellSoftkeySpec::Role::Custom,
                            ShellNavigationAction::OpenMenu),
        make_system_softkey("page-center-hidden",
                            "",
                            ShellSoftkeySpec::Role::Custom,
                            ShellNavigationAction::OpenMenu),
        make_system_softkey("page-back",
                            "Back",
                            ShellSoftkeySpec::Role::Back,
                            ShellNavigationAction::Back),
    };
    for (std::size_t index = 0; index < 2; ++index) {
        const auto& source = page.softkeys[index];
        softkeys[index] = make_page_softkey(index == 0 ? "page-left" : "page-center",
                                            source.label,
                                            role_from_foreground_softkey(source.role),
                                            source.command_id,
                                            source.enabled,
                                            source.role == AEGIS_UI_SOFTKEY_ROLE_PRIMARY);
        softkeys[index].visibility = source.visible ? ShellSoftkeySpec::Visibility::Visible
                                                    : ShellSoftkeySpec::Visibility::Hidden;
    }
    return softkeys;
}

}  // namespace

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
                 "app catalog loaded entries=" + std::to_string(launcher_.entries().size()));
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
    const bool structured_page_active =
        app_foreground_.page.has_value() &&
        app_foreground_.page->layout_template != runtime::ForegroundPageTemplate::TextLog;
    if (controller_.navigation_state().surface() == ShellSurface::AppForeground &&
        !structured_page_active) {
        present_frame("app",
                      app_foreground_.root_name.empty()
                          ? "foreground"
                          : "root=" + app_foreground_.root_name);
    }
}

void AegisShell::set_app_foreground_page(runtime::ForegroundPagePresentation page) {
    app_foreground_.page = std::move(page);
    logger_.info("shell",
                 "foreground page declared id=" + app_foreground_.page->page_id +
                    " token=" + app_foreground_.page->page_state_token + " items=" +
                    std::to_string(app_foreground_.page->items.size()) + " template=" +
                    std::to_string(static_cast<int>(app_foreground_.page->layout_template)) +
                    " dialog=" + (app_foreground_.page->dialog.visible ? std::string("1")
                                                                       : std::string("0")));
    if (controller_.navigation_state().surface() == ShellSurface::AppForeground) {
        present_frame("app",
                      app_foreground_.root_name.empty()
                          ? "foreground"
                          : "root=" + app_foreground_.root_name);
    }
}

void AegisShell::clear_app_foreground_state() {
    app_foreground_.root_name.clear();
    app_foreground_.page.reset();
    app_foreground_.lines.clear();
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
            if (controller_.navigation_state().surface() == ShellSurface::Settings &&
                settings_app_.handle_routed_action(action, settings_service_)) {
                present_frame("settings", "built-in settings");
            }
            return std::nullopt;
        case ShellNavigationAction::MovePrevious:
            if (controller_.navigation_state().surface() == ShellSurface::Launcher &&
                launcher_.focus_previous()) {
                log_launcher_focus();
            }
            if (controller_.navigation_state().surface() == ShellSurface::Settings &&
                settings_app_.handle_routed_action(action, settings_service_)) {
                present_frame("settings", "built-in settings");
            }
            return std::nullopt;
        case ShellNavigationAction::PrimaryAction:
            if (controller_.navigation_state().surface() == ShellSurface::Settings) {
                const bool changed = settings_app_.handle_routed_action(action, settings_service_);
                if (changed) {
                    reload_settings_app();
                    if (notification_service_ != nullptr) {
                        notification_service_->notify("Settings", "Built-in settings applied");
                    }
                    present_frame("settings", "built-in settings");
                }
            }
            if (controller_.navigation_state().surface() == ShellSurface::Launcher) {
                if (const auto* focused = launcher_.focused_entry(); focused != nullptr &&
                    launcher_entry_actionable(*focused)) {
                    return focused->descriptor.manifest.app_id;
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
                if (const auto* focused = launcher_.focused_entry(); focused != nullptr &&
                    launcher_entry_actionable(*focused)) {
                    return focused->descriptor.manifest.app_id;
                }
            }
            if (controller_.navigation_state().surface() == ShellSurface::Settings) {
                if (settings_app_.handle_routed_action(action, settings_service_)) {
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
                    if (settings_app_.handle_routed_action(action, settings_service_)) {
                        present_frame("settings", "built-in settings");
                        break;
                    }
                    [[fallthrough]];
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
            if (const auto* settings_entry = find_launcher_entry_by_app_id(launcher_, "settings");
                settings_entry != nullptr && launcher_entry_actionable(*settings_entry)) {
                return std::string("settings");
            }
            controller_.show_settings();
            reload_settings_app();
            log_surface_summary();
            present_frame("settings", "resident settings fallback");
            return std::nullopt;
        case ShellNavigationAction::OpenNotifications:
            controller_.show_notifications();
            log_surface_summary();
            present_frame("notifications", "inbox");
            return std::nullopt;
    }

    return std::nullopt;
}

std::optional<std::string> AegisShell::handle_input_invocation(const ShellInputInvocation& invocation) {
    if (invocation.target == ShellInputInvocationTarget::SystemAction) {
        return handle_action(invocation.system_action);
    }

    return handle_page_command(invocation_page_id(invocation),
                               invocation_page_command_id(invocation),
                               invocation_page_state_token(invocation));
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
        .page_id = {},
        .page_state_token = {},
        .headline = std::move(headline),
        .detail = std::move(detail),
    };

    switch (frame.surface) {
        case ShellSurface::Home:
            frame.page_id = "shell.home";
            frame.page_state_token = "home:ready";
            frame.context = "resident shell";
            frame.lines.push_back({.text = "Aegis ready", .emphasized = true});
            frame.lines.push_back({.text = "Select opens launcher"});
            frame.lines.push_back({.text = "F1 settings  F2 notifications"});
            frame.lines.push_back({.text = "Menu returns home"});
            frame.softkeys = {
                make_system_softkey(
                    "home-open", "Open", ShellSoftkeySpec::Role::Primary, ShellNavigationAction::Select),
                make_system_softkey(
                    "home-menu", "Menu", ShellSoftkeySpec::Role::Menu, ShellNavigationAction::OpenMenu),
                make_system_softkey(
                    "home-back", "Back", ShellSoftkeySpec::Role::Back, ShellNavigationAction::Back),
            };
            break;
        case ShellSurface::Launcher: {
            frame.page_id = "shell.launcher";
            frame.page_state_token = "launcher:" + std::to_string(launcher_.focus_index());
            frame.context = "installed apps";
            frame.headline = "launcher";
            std::size_t visible_count = 0;
            for (std::size_t index = 0; index < launcher_.entries().size() && frame.lines.size() < 8; ++index) {
                const auto& entry = launcher_.entries()[index];
                if (!launcher_entry_actionable(entry)) {
                    continue;
                }
                ++visible_count;
                const bool focused = index == launcher_.focus_index();
                const auto detail_text = launcher_entry_detail(entry);
                frame.items.push_back({
                    .item_id = entry.descriptor.manifest.app_id,
                    .label = entry.descriptor.manifest.display_name,
                    .detail = detail_text,
                    .focused = focused,
                    .emphasized = focused,
                    .warning = false,
                    .disabled = false,
                    .accessory = ShellPresentationAccessory::None,
                    .actionable = true,
                    .command_id = entry.descriptor.manifest.app_id,
                    .icon_source = entry.descriptor.icon_exists
                                       ? ShellPresentationIconSource::AppPackage
                                       : ShellPresentationIconSource::None,
                    .icon_key = entry.descriptor.icon_exists ? entry.descriptor.icon_path
                                                             : std::string {},
                });
                std::string line =
                    (focused ? "[*] " : "[ ] ") +
                    std::to_string(index + 1) + " " + entry.descriptor.manifest.display_name;
                if (!detail_text.empty()) {
                    line += " [" + detail_text + "]";
                }
                frame.lines.push_back({.text = std::move(line), .emphasized = focused});
            }
            frame.detail = "apps=" + std::to_string(visible_count);
            if (frame.lines.empty()) {
                frame.lines.push_back({.text = "No apps installed", .emphasized = true});
            }
            frame.softkeys = {
                make_page_softkey("launcher-open",
                                  "Open",
                                  ShellSoftkeySpec::Role::Primary,
                                  "open_focused"),
                make_system_softkey(
                    "launcher-menu", "Menu", ShellSoftkeySpec::Role::Menu, ShellNavigationAction::OpenMenu),
                make_system_softkey(
                    "launcher-back", "Back", ShellSoftkeySpec::Role::Back, ShellNavigationAction::Back),
            };
            break;
        }
        case ShellSurface::Settings: {
            frame.detail = "dirty=" + std::to_string(settings_app_.dirty_count());
            const auto page = settings_app_.page_presentation();
            finalize_frame_from_page(page, frame);
            break;
        }
        case ShellSurface::Notifications:
            frame.page_id = "shell.notifications";
            frame.page_state_token = "notifications:" + std::to_string(notifications_.entries().size());
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
            frame.softkeys = {
                make_page_softkey(
                    "notifications-open", "Open", ShellSoftkeySpec::Role::Primary, "open"),
                make_page_softkey(
                    "notifications-clear", "Clear", ShellSoftkeySpec::Role::Secondary, "clear"),
                make_system_softkey("notifications-back",
                                    "Back",
                                    ShellSoftkeySpec::Role::Back,
                                    ShellNavigationAction::Back),
            };
            break;
        case ShellSurface::AppForeground:
            frame.page_id = controller_.navigation_state().active_app_id().empty()
                                ? "app.foreground"
                                : "app." + controller_.navigation_state().active_app_id() + ".foreground";
            frame.context = "native app running";
            if (app_foreground_.page.has_value()) {
                finalize_frame_from_page(*app_foreground_.page, frame);
                frame.detail = app_foreground_.page->page_state_token.empty()
                                   ? frame.detail
                                   : "token=" + app_foreground_.page->page_state_token;
            }
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
            if (frame.page_state_token.empty()) {
                frame.page_state_token =
                    "app:" + controller_.navigation_state().active_app_id() + ":" + app_foreground_.root_name;
            }
            if (frame.softkeys[0].label.empty() && frame.softkeys[1].label.empty() &&
                frame.softkeys[2].label.empty()) {
                frame.softkeys = {
                    make_page_softkey(
                        "app-primary", "Action", ShellSoftkeySpec::Role::Primary, "primary"),
                    make_page_softkey(
                        "app-select", "Select", ShellSoftkeySpec::Role::Confirm, "select"),
                    make_system_softkey(
                        "app-back", "Back", ShellSoftkeySpec::Role::Back, ShellNavigationAction::Back),
                };
            }
            break;
        case ShellSurface::Recovery:
            frame.page_id = "shell.recovery";
            frame.page_state_token = "recovery:return";
            frame.context = "return path";
            frame.lines.push_back({.text = "Recovered to launcher", .emphasized = true});
            frame.lines.push_back({.text = "Foreground ownership reclaimed"});
            frame.softkeys = {
                make_page_softkey(
                    "recovery-retry", "Retry", ShellSoftkeySpec::Role::Primary, "retry"),
                make_page_softkey(
                    "recovery-select", "Select", ShellSoftkeySpec::Role::Confirm, "select"),
                make_system_softkey(
                    "recovery-back", "Back", ShellSoftkeySpec::Role::Back, ShellNavigationAction::Back),
            };
            break;
    }

    return frame;
}

std::optional<std::string> AegisShell::handle_page_command(std::string_view page_id,
                                                           std::string_view page_command_id,
                                                           std::string_view page_state_token) {
    const auto current_frame = build_frame("", "");
    if (page_id != current_frame.page_id || page_state_token != current_frame.page_state_token) {
        logger_.info("shell",
                     "drop stale page command page=" + std::string(page_id) +
                         " command=" + std::string(page_command_id));
        return std::nullopt;
    }

    if (page_id == "shell.launcher") {
        if (page_command_id == "open_focused") {
            return handle_action(ShellNavigationAction::PrimaryAction);
        }
        if (!page_command_id.empty()) {
            if (const auto* entry = find_launcher_entry_by_app_id(launcher_, page_command_id);
                entry != nullptr && launcher_entry_actionable(*entry)) {
                return entry->descriptor.manifest.app_id;
            }
        }
        return std::nullopt;
    }

    if (page_id == "builtin.settings") {
        if (settings_app_.handle_page_command(page_command_id, settings_service_)) {
            if (page_command_id == "apply") {
                reload_settings_app();
                if (notification_service_ != nullptr) {
                    notification_service_->notify("Settings", "Built-in settings applied");
                }
            }
            present_frame("settings", "built-in settings");
        }
        return std::nullopt;
    }

    if (page_id == "shell.notifications") {
        logger_.info("shell",
                     "notification page command=" + std::string(page_command_id) + " not implemented yet");
        return std::nullopt;
    }

    if (page_id == "shell.recovery") {
        logger_.info("shell",
                     "recovery page command=" + std::string(page_command_id) + " not implemented yet");
        return std::nullopt;
    }

    if (page_id.rfind("app.", 0) == 0) {
        logger_.info("shell",
                     "foreground app page command pending runtime route page=" + std::string(page_id) +
                         " command=" + std::string(page_command_id));
        return std::nullopt;
    }

    if (page_id == "shell.home") {
        if (page_command_id == "open") {
            return handle_action(ShellNavigationAction::Select);
        }
        return std::nullopt;
    }

    logger_.info("shell",
                 "unhandled page command page=" + std::string(page_id) +
                     " command=" + std::string(page_command_id));
    return std::nullopt;
}

void AegisShell::initialize_system_launcher() {
    system_launcher_ = {
        {.id = "apps",
         .label = "Apps",
         .subtitle = "catalog soon",
         .fallback_label = "Apps",
         .fallback_subtitle = "catalog soon",
         .command_id = "launcher.apps",
         .icon_source = ShellPresentationIconSource::Symbol,
         .icon_key = "list"},
        {.id = "files",
         .label = "Files",
         .subtitle = "storage browser",
         .fallback_label = "Files",
         .fallback_subtitle = "storage browser",
         .available = false,
         .command_id = "launcher.files",
         .icon_source = ShellPresentationIconSource::AppPackage,
         .icon_key = {},
         .package_app_id = "files"},
        {.id = "settings",
         .label = "Settings",
         .subtitle = "system settings",
         .fallback_label = "Settings",
         .fallback_subtitle = "system settings",
         .available = false,
         .command_id = "launcher.settings",
         .icon_source = ShellPresentationIconSource::AppPackage,
         .icon_key = {},
         .package_app_id = "settings"},
        {.id = "radio",
         .label = "Radio",
         .subtitle = "soon",
         .fallback_label = "Radio",
         .fallback_subtitle = "soon",
         .command_id = "launcher.radio",
         .icon_source = ShellPresentationIconSource::Symbol,
         .icon_key = "wifi"},
        {.id = "tools",
         .label = "Tools",
         .subtitle = "soon",
         .fallback_label = "Tools",
         .fallback_subtitle = "soon",
         .command_id = "launcher.tools",
         .icon_source = ShellPresentationIconSource::Symbol,
         .icon_key = "edit"},
        {.id = "device",
         .label = "Device",
         .subtitle = "soon",
         .fallback_label = "Device",
         .fallback_subtitle = "soon",
         .command_id = "launcher.device",
         .icon_source = ShellPresentationIconSource::Symbol,
         .icon_key = "battery"},
    };
    system_launcher_focus_index_ = 0;
    for (std::size_t index = 0; index < system_launcher_.size(); ++index) {
        if (system_launcher_[index].available) {
            system_launcher_focus_index_ = index;
            break;
        }
    }
}

void AegisShell::sync_system_launcher_with_catalog(const core::AppCatalog& catalog) {
    for (auto& entry : system_launcher_) {
        if (!entry.fallback_label.empty()) {
            entry.label = entry.fallback_label;
        }
        if (!entry.fallback_subtitle.empty()) {
            entry.subtitle = entry.fallback_subtitle;
        }

        if (entry.package_app_id.empty()) {
            continue;
        }

        const auto it =
            std::find_if(catalog.entries().begin(),
                         catalog.entries().end(),
                         [&entry](const core::AppCatalogEntry& catalog_entry) {
                             return catalog_entry.descriptor.manifest.app_id == entry.package_app_id;
                         });
        if (it == catalog.entries().end()) {
            entry.available = false;
            entry.icon_key.clear();
            entry.subtitle = "not installed";
            continue;
        }

        if (!it->descriptor.manifest.display_name.empty()) {
            entry.label = it->descriptor.manifest.display_name;
        }
        entry.icon_key = it->descriptor.icon_exists ? it->descriptor.icon_path : std::string {};

        switch (it->state) {
            case core::AppCatalogEntryState::Visible:
                entry.available = true;
                if (!entry.fallback_subtitle.empty()) {
                    entry.subtitle = entry.fallback_subtitle;
                }
                break;
            case core::AppCatalogEntryState::Hidden:
                entry.available = false;
                entry.subtitle = "hidden";
                break;
            case core::AppCatalogEntryState::Incompatible:
                entry.available = false;
                entry.subtitle = it->reason.empty() ? "incompatible" : it->reason;
                break;
            case core::AppCatalogEntryState::Invalid:
                entry.available = false;
                entry.subtitle = it->reason.empty() ? "invalid" : it->reason;
                break;
        }
    }

    if (system_launcher_focus_index_ >= system_launcher_.size()) {
        system_launcher_focus_index_ = 0;
    }
    if (!system_launcher_.empty() && !system_launcher_[system_launcher_focus_index_].available) {
        for (std::size_t index = 0; index < system_launcher_.size(); ++index) {
            if (system_launcher_[index].available) {
                system_launcher_focus_index_ = index;
                break;
            }
        }
    }
}

std::optional<std::string> AegisShell::dispatch_system_launcher_entry(std::size_t index) {
    if (index >= system_launcher_.size()) {
        return std::nullopt;
    }

    system_launcher_focus_index_ = index;
    const auto& entry = system_launcher_[index];
    logger_.info("shell", "launcher selected entry " + entry.id);
    if (!entry.available) {
        logger_.info("shell", "launcher entry unavailable " + entry.id);
        return std::nullopt;
    }
    if (!entry.package_app_id.empty()) {
        return entry.package_app_id;
    }
    if (entry.opens_shell_settings) {
        controller_.show_settings();
        reload_settings_app();
        log_surface_summary();
        present_frame("settings", "built-in settings");
    }
    return std::nullopt;
}

std::optional<std::string> AegisShell::dispatch_system_launcher_command(std::string_view command_id) {
    for (std::size_t index = 0; index < system_launcher_.size(); ++index) {
        if (system_launcher_[index].command_id == command_id) {
            return dispatch_system_launcher_entry(index);
        }
    }

    logger_.info("shell", "unknown launcher command " + std::string(command_id));
    return std::nullopt;
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
    if (const auto* focused = launcher_.focused_entry()) {
        logger_.info("shell",
                     "launcher focus=" + focused->descriptor.manifest.app_id + " index=" +
                         std::to_string(launcher_.focus_index()));
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
