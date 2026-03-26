#include "shell/control/shell_input_model.hpp"

#include <algorithm>

namespace aegis::shell {

void ShellInputModel::set_supported_actions(std::vector<ShellNavigationAction> actions) {
    supported_actions_ = std::move(actions);
}

const std::vector<ShellNavigationAction>& ShellInputModel::supported_actions() const {
    return supported_actions_;
}

bool ShellInputModel::supports(ShellNavigationAction action) const {
    return std::find(supported_actions_.begin(), supported_actions_.end(), action) !=
           supported_actions_.end();
}

std::string_view to_string(ShellNavigationAction action) {
    switch (action) {
        case ShellNavigationAction::MoveNext:
            return "move_next";
        case ShellNavigationAction::MovePrevious:
            return "move_previous";
        case ShellNavigationAction::PrimaryAction:
            return "primary_action";
        case ShellNavigationAction::Select:
            return "select";
        case ShellNavigationAction::Back:
            return "back";
        case ShellNavigationAction::OpenMenu:
            return "open_menu";
        case ShellNavigationAction::OpenFiles:
            return "open_files";
        case ShellNavigationAction::OpenSettings:
            return "open_settings";
        case ShellNavigationAction::OpenNotifications:
            return "open_notifications";
    }

    return "unknown";
}

bool try_parse_shell_action(std::string_view value, ShellNavigationAction& action) {
    static constexpr ShellNavigationAction all[] {
        ShellNavigationAction::MoveNext,
        ShellNavigationAction::MovePrevious,
        ShellNavigationAction::PrimaryAction,
        ShellNavigationAction::Select,
        ShellNavigationAction::Back,
        ShellNavigationAction::OpenMenu,
        ShellNavigationAction::OpenFiles,
        ShellNavigationAction::OpenSettings,
        ShellNavigationAction::OpenNotifications,
    };

    for (const auto candidate : all) {
        if (to_string(candidate) == value) {
            action = candidate;
            return true;
        }
    }
    return false;
}

}  // namespace aegis::shell
