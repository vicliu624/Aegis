#pragma once

#include <array>
#include <string_view>
#include <vector>

namespace aegis::shell {

enum class ShellNavigationAction {
    MoveNext,
    MovePrevious,
    PrimaryAction,
    Select,
    Back,
    OpenMenu,
    OpenSettings,
    OpenNotifications,
};

enum class ShellInputInvocationTarget {
    SystemAction,
    PageCommand,
};

struct ShellInputInvocation {
    ShellInputInvocationTarget target {ShellInputInvocationTarget::SystemAction};
    ShellNavigationAction system_action {ShellNavigationAction::OpenMenu};
    std::array<char, 48> page_id {};
    std::array<char, 32> page_command_id {};
    std::array<char, 32> page_state_token {};
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
[[nodiscard]] ShellInputInvocation make_system_invocation(ShellNavigationAction action);
[[nodiscard]] ShellInputInvocation make_page_command_invocation(std::string_view page_id,
                                                                std::string_view page_command_id,
                                                                std::string_view page_state_token);
[[nodiscard]] std::string_view invocation_page_id(const ShellInputInvocation& invocation);
[[nodiscard]] std::string_view invocation_page_command_id(const ShellInputInvocation& invocation);
[[nodiscard]] std::string_view invocation_page_state_token(const ShellInputInvocation& invocation);

}  // namespace aegis::shell
