#include "shell/control/shell_navigation_state.hpp"

namespace aegis::shell {

void ShellNavigationState::set_surface(ShellSurface surface) {
    surface_ = surface;
}

void ShellNavigationState::enter_app(std::string app_id) {
    active_app_id_ = std::move(app_id);
    surface_ = ShellSurface::AppForeground;
}

void ShellNavigationState::recover_from_app() {
    active_app_id_.clear();
    surface_ = ShellSurface::Recovery;
}

ShellSurface ShellNavigationState::surface() const {
    return surface_;
}

const std::string& ShellNavigationState::active_app_id() const {
    return active_app_id_;
}

}  // namespace aegis::shell
