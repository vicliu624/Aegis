#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include <zephyr/input/input.h>

#include "platform/logging/logger.hpp"
#include "ports/zephyr/zephyr_board_backend_config.hpp"
#include "ports/zephyr/zephyr_board_runtime.hpp"
#include "ports/zephyr/zephyr_shell_input_backend.hpp"
#include "shell/control/shell_input_model.hpp"

namespace aegis::ports::zephyr {

class ZephyrShellInputAdapter {
public:
    ZephyrShellInputAdapter(platform::Logger& logger,
                            ZephyrBoardRuntime& runtime,
                            ZephyrBoardBackendConfig config);

    [[nodiscard]] bool initialize();
    void enable_interactive_mode();
    [[nodiscard]] std::optional<shell::ShellNavigationAction> poll_action();
    void record_mapped_action(const input_event& event, shell::ShellNavigationAction action);
    [[nodiscard]] std::optional<shell::ShellNavigationAction> map_input_event(
        const input_event& event);
    void accept_callback_action(shell::ShellNavigationAction action);

private:
    [[nodiscard]] std::optional<shell::ShellNavigationAction> map_matrix_key(
        int row,
        int col,
        bool pressed) const;

    platform::Logger& logger_;
    ZephyrBoardRuntime& runtime_;
    ZephyrBoardBackendConfig config_;
    std::unique_ptr<IZephyrShellInputBackend> backend_;
    bool interactive_mode_enabled_ {false};
    bool callback_input_enabled_ {true};
    int keyboard_event_row_ {-1};
    int keyboard_event_col_ {-1};
};

}  // namespace aegis::ports::zephyr
