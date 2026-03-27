#include "shell/control/shell_controller.hpp"

namespace aegis::shell {

namespace {

std::string_view to_string(ShellSurface surface) {
    switch (surface) {
        case ShellSurface::Home:
            return "home";
        case ShellSurface::Launcher:
            return "launcher";
        case ShellSurface::Settings:
            return "settings";
        case ShellSurface::Notifications:
            return "notifications";
        case ShellSurface::AppForeground:
            return "app_foreground";
        case ShellSurface::Recovery:
            return "recovery";
    }

    return "unknown";
}

}  // namespace

ShellController::ShellController(platform::Logger& logger) : logger_(logger) {}

void ShellController::boot_to_home() {
    navigation_.set_surface(ShellSurface::Home);
    log_surface("boot");
}

void ShellController::show_launcher() {
    navigation_.set_surface(ShellSurface::Launcher);
    log_surface("show");
}

void ShellController::show_settings() {
    navigation_.set_surface(ShellSurface::Settings);
    log_surface("show");
}

void ShellController::show_notifications() {
    navigation_.set_surface(ShellSurface::Notifications);
    log_surface("show");
}

void ShellController::enter_app(const std::string& app_id) {
    navigation_.enter_app(app_id);
    logger_.info("shell", "enter app foreground " + app_id);
    log_surface("show");
}

void ShellController::return_from_app(const std::string& app_id) {
    navigation_.recover_from_app();
    logger_.info("shell", "recovered foreground from app " + app_id);
    log_surface("show");
    show_launcher();
}

const ShellNavigationState& ShellController::navigation_state() const {
    return navigation_;
}

void ShellController::log_surface(std::string_view action) const {
    logger_.info("shell",
                 std::string(action) + " shell surface " +
                     std::string(to_string(navigation_.surface())));
}

}  // namespace aegis::shell
