#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "platform/logging/logger.hpp"
#include "ports/zephyr/zephyr_board_backend_config.hpp"
#include "ports/zephyr/zephyr_board_runtime.hpp"

namespace aegis::ports::zephyr {

class IZephyrShellDisplayBackend {
public:
    virtual ~IZephyrShellDisplayBackend() = default;

    [[nodiscard]] virtual bool initialize() = 0;
    virtual void write_region(int x,
                              int y,
                              int width,
                              int height,
                              const std::vector<uint16_t>& pixels) const = 0;
};

[[nodiscard]] std::unique_ptr<IZephyrShellDisplayBackend> make_zephyr_shell_display_backend(
    platform::Logger& logger,
    ZephyrBoardRuntime& runtime,
    ZephyrBoardBackendConfig config);

}  // namespace aegis::ports::zephyr
