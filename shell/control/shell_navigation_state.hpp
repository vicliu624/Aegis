#pragma once

#include <string>

namespace aegis::shell {

enum class ShellSurface {
    Home,
    Launcher,
    Settings,
    Notifications,
    AppForeground,
    Recovery,
};

class ShellNavigationState {
public:
    void set_surface(ShellSurface surface);
    void enter_app(std::string app_id);
    void recover_from_app();

    [[nodiscard]] ShellSurface surface() const;
    [[nodiscard]] const std::string& active_app_id() const;

private:
    ShellSurface surface_ {ShellSurface::Home};
    std::string active_app_id_;
};

}  // namespace aegis::shell
