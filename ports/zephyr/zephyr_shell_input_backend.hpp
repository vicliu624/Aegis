#pragma once

#include <memory>
#include <optional>

#include "platform/logging/logger.hpp"
#include "ports/zephyr/zephyr_board_backend_config.hpp"
#include "ports/zephyr/zephyr_board_runtime.hpp"
#include "shell/control/shell_input_model.hpp"

namespace aegis::ports::zephyr {

class IZephyrShellInputBackend {
public:
    virtual ~IZephyrShellInputBackend() = default;

    [[nodiscard]] virtual bool initialize() = 0;
    [[nodiscard]] virtual bool callback_input_enabled() const = 0;
    virtual void enable_interactive_mode() = 0;
    [[nodiscard]] virtual std::optional<shell::ShellNavigationAction> poll_action() = 0;
};

[[nodiscard]] std::unique_ptr<IZephyrShellInputBackend> make_zephyr_shell_input_backend(
    platform::Logger& logger,
    ZephyrBoardRuntime& runtime,
    ZephyrBoardBackendConfig config);

}  // namespace aegis::ports::zephyr
