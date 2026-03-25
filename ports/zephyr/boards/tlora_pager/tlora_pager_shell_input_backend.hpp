#pragma once

#include <memory>

#include "platform/logging/logger.hpp"
#include "ports/zephyr/zephyr_board_backend_config.hpp"
#include "ports/zephyr/zephyr_board_runtime.hpp"
#include "ports/zephyr/zephyr_shell_input_backend.hpp"

namespace aegis::ports::zephyr {

[[nodiscard]] std::unique_ptr<IZephyrShellInputBackend> make_tlora_pager_shell_input_backend(
    platform::Logger& logger,
    ZephyrBoardRuntime& runtime,
    ZephyrBoardBackendConfig config);

}  // namespace aegis::ports::zephyr
