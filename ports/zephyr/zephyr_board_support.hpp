#pragma once

#include <memory>
#include <string_view>
#include <vector>

#include "device/packages/board_package.hpp"
#include "platform/logging/logger.hpp"
#include "ports/zephyr/zephyr_board_descriptors.hpp"
#include "ports/zephyr/zephyr_board_runtime.hpp"

namespace aegis::ports::zephyr {

struct ZephyrBoardSupport {
    device::BoardPackagePtr package;
    const ZephyrBoardDescriptor* descriptor {nullptr};
    ZephyrBoardRuntime* runtime {nullptr};
};

[[nodiscard]] std::vector<device::BoardPackagePtr> make_zephyr_board_packages();
[[nodiscard]] ZephyrBoardSupport resolve_zephyr_board_support(std::string_view package_id,
                                                              platform::Logger& logger);
[[nodiscard]] ZephyrBoardRuntime& runtime_for_package(std::string_view package_id,
                                                      platform::Logger& logger);
[[nodiscard]] bool initialize_board_runtime(std::string_view package_id, platform::Logger& logger);

}  // namespace aegis::ports::zephyr
