#include "shell/control/shell_input_model.hpp"

#include <algorithm>
#include <cstring>

namespace aegis::shell {

namespace {

template <std::size_t N>
void copy_text(std::array<char, N>& out, std::string_view value) {
    out.fill('\0');
    const auto length = std::min<std::size_t>(N - 1, value.size());
    if (length > 0) {
        std::memcpy(out.data(), value.data(), length);
    }
}

template <std::size_t N>
std::string_view view_text(const std::array<char, N>& value) {
    const auto* start = value.data();
    std::size_t length = 0;
    while (length < N && start[length] != '\0') {
        ++length;
    }
    return std::string_view(start, length);
}

}  // namespace

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

ShellInputInvocation make_system_invocation(ShellNavigationAction action) {
    return ShellInputInvocation {
        .target = ShellInputInvocationTarget::SystemAction,
        .system_action = action,
    };
}

ShellInputInvocation make_page_command_invocation(std::string_view page_id,
                                                  std::string_view page_command_id,
                                                  std::string_view page_state_token) {
    ShellInputInvocation invocation {
        .target = ShellInputInvocationTarget::PageCommand,
        .system_action = ShellNavigationAction::OpenMenu,
    };
    copy_text(invocation.page_id, page_id);
    copy_text(invocation.page_command_id, page_command_id);
    copy_text(invocation.page_state_token, page_state_token);
    return invocation;
}

std::string_view invocation_page_id(const ShellInputInvocation& invocation) {
    return view_text(invocation.page_id);
}

std::string_view invocation_page_command_id(const ShellInputInvocation& invocation) {
    return view_text(invocation.page_command_id);
}

std::string_view invocation_page_state_token(const ShellInputInvocation& invocation) {
    return view_text(invocation.page_state_token);
}

}  // namespace aegis::shell
