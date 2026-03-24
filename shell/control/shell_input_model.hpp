#pragma once

#include <string_view>
#include <vector>

namespace aegis::shell {

enum class ShellNavigationAction {
    MoveNext,
    MovePrevious,
    Select,
    Back,
    OpenMenu,
    OpenSettings,
    OpenNotifications,
};

class ShellInputModel {
public:
    void set_supported_actions(std::vector<ShellNavigationAction> actions);
    [[nodiscard]] const std::vector<ShellNavigationAction>& supported_actions() const;
    [[nodiscard]] bool supports(ShellNavigationAction action) const;

private:
    std::vector<ShellNavigationAction> supported_actions_;
};

[[nodiscard]] std::string_view to_string(ShellNavigationAction action);
[[nodiscard]] bool try_parse_shell_action(std::string_view value, ShellNavigationAction& action);

}  // namespace aegis::shell
