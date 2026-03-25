#include "ports/zephyr/zephyr_board_support.hpp"

#include <stdexcept>

#include "ports/zephyr/zephyr_board_descriptors.hpp"
#include "ports/zephyr/zephyr_board_packages.hpp"
#if defined(AEGIS_ZEPHYR_HAS_TDECK_FAMILY)
#include "ports/zephyr/boards/tdeck/tdeck_board_runtime.hpp"
#endif
#if defined(AEGIS_ZEPHYR_HAS_TLORA_PAGER_FAMILY)
#include "ports/zephyr/boards/tlora_pager/tlora_pager_board_runtime.hpp"
#endif

namespace aegis::ports::zephyr {

ZephyrBoardRuntime& runtime_for_package(std::string_view package_id, platform::Logger& logger) {
    const auto& descriptor = descriptor_for_package(package_id);
    switch (descriptor.config.runtime_family) {
        case ZephyrBoardRuntimeFamily::TDeck:
#if defined(AEGIS_ZEPHYR_HAS_TDECK_FAMILY)
            return tdeck_board_runtime(logger);
#else
            throw std::runtime_error("tdeck runtime not compiled into this Zephyr image");
#endif
        case ZephyrBoardRuntimeFamily::TloraPager:
#if defined(AEGIS_ZEPHYR_HAS_TLORA_PAGER_FAMILY)
            return tlora_pager_board_runtime(logger);
#else
            throw std::runtime_error("tlora pager runtime not compiled into this Zephyr image");
#endif
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
