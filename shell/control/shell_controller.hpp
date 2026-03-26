#pragma once

#include <string_view>

#include "platform/logging/logger.hpp"
#include "shell/control/shell_navigation_state.hpp"

namespace aegis::shell {

class ShellController {
public:
    explicit ShellController(platform::Logger& logger);

    void boot_to_home();
    void show_launcher();
    void show_files();
    void show_settings();
    void show_notifications();
    void enter_app(const std::string& app_id);
    void return_from_app(const std::string& app_id);

    [[nodiscard]] const ShellNavigationState& navigation_state() const;

private:
    void log_surface(std::string_view action) const;

    platform::Logger& logger_;
    ShellNavigationState navigation_;
};

}  // namespace aegis::shell
