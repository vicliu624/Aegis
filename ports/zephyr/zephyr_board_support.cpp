#include "ports/zephyr/zephyr_board_support.hpp"

#include <stdexcept>

#include "ports/zephyr/zephyr_board_descriptors.hpp"
#include "ports/zephyr/zephyr_board_packages.hpp"
#include "ports/zephyr/zephyr_tlora_pager_board_runtime.hpp"

namespace aegis::ports::zephyr {

ZephyrBoardRuntime& runtime_for_package(std::string_view package_id, platform::Logger& logger) {
    const auto& descriptor = descriptor_for_package(package_id);
    switch (descriptor.config.runtime_family) {
        case ZephyrBoardRuntimeFamily::TloraPager:
            return tlora_pager_board_runtime(logger);
        case ZephyrBoardRuntimeFamily::Generic:
        default:
            return generic_zephyr_board_runtime(package_id);
    }
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
    const auto& descriptor = descriptor_for_package(package_id);
    for (const auto& package : packages) {
        if (package->package_id() == package_id) {
            auto& runtime = runtime_for_package(package_id, logger);
            return ZephyrBoardSupport {
                .package = package,
                .descriptor = &descriptor,
                .runtime = &runtime,
            };
        }
    }

    throw std::runtime_error("unknown Zephyr board package: " + std::string(package_id));
}

}  // namespace aegis::ports::zephyr
