#include "ports/zephyr/zephyr_board_support.hpp"

#include <stdexcept>

#include "ports/zephyr/zephyr_board_packages.hpp"
#include "ports/zephyr/zephyr_tlora_pager_board_runtime.hpp"

namespace aegis::ports::zephyr {

ZephyrBoardRuntime& runtime_for_package(std::string_view package_id, platform::Logger& logger) {
    if (package_id == "zephyr_tlora_pager_sx1262") {
        return tlora_pager_board_runtime(logger);
    }
    return generic_zephyr_board_runtime(package_id);
}

bool initialize_board_runtime(std::string_view package_id, platform::Logger& logger) {
    auto& runtime = runtime_for_package(package_id, logger);
    set_active_zephyr_board_runtime(&runtime);
    if (!runtime.ready()) {
        return runtime.initialize();
    }
    runtime.log_state("reuse");
    return true;
}

ZephyrBoardSupport resolve_zephyr_board_support(std::string_view package_id, platform::Logger& logger) {
    auto packages = make_zephyr_board_packages();
    for (const auto& package : packages) {
        if (package->package_id() == package_id) {
            auto& runtime = runtime_for_package(package_id, logger);
            return ZephyrBoardSupport {
                .package = package,
                .runtime = &runtime,
            };
        }
    }

    throw std::runtime_error("unknown Zephyr board package: " + std::string(package_id));
}

}  // namespace aegis::ports::zephyr
