#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include "device/common/profile/device_profile.hpp"
#include "device/common/profile/shell_surface_profile.hpp"
#include "ports/zephyr/zephyr_board_backend_config.hpp"

namespace aegis::ports::zephyr {

enum class ZephyrBoardTextInputStrategy : uint8_t {
    InMemoryUnavailable,
    InMemoryKeyboard,
    ZephyrService,
    TDeckService,
};

struct ZephyrBoardBringupPlan {
    std::string initialization_banner;
    std::string runtime_ready_stage;
    std::string input_ready_stage;
    int runtime_ready_signal_stage {1};
    int display_ready_signal_stage {2};
    int core_ready_signal_stage {3};
    int input_ready_signal_stage {4};
    int interactive_ready_signal_stage {5};
    bool run_shell_selftest {true};
};

struct ZephyrBoardDescriptor {
    std::string package_id;
    ZephyrBoardBackendConfig config;
    device::DeviceProfile profile;
    device::ShellSurfaceProfile shell_surface_profile;
    ZephyrBoardBringupPlan bringup;
    ZephyrBoardTextInputStrategy text_input_strategy {ZephyrBoardTextInputStrategy::InMemoryUnavailable};
    std::string text_input_source_name;
};

[[nodiscard]] const ZephyrBoardDescriptor& descriptor_for_package(std::string_view package_id);
[[nodiscard]] std::vector<std::reference_wrapper<const ZephyrBoardDescriptor>> all_zephyr_board_descriptors();

}  // namespace aegis::ports::zephyr
