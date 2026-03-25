#include "ports/zephyr/zephyr_shell_input_backend.hpp"

#include <utility>

#include <zephyr/device.h>

#if defined(AEGIS_ZEPHYR_HAS_TDECK_FAMILY)
#include "ports/zephyr/boards/tdeck/tdeck_shell_input_backend.hpp"
#endif
#if defined(AEGIS_ZEPHYR_HAS_TLORA_PAGER_FAMILY)
#include "ports/zephyr/boards/tlora_pager/tlora_pager_shell_input_backend.hpp"
#endif

namespace aegis::ports::zephyr {

namespace {

class GenericShellInputBackend final : public IZephyrShellInputBackend {
public:
    explicit GenericShellInputBackend(ZephyrBoardBackendConfig config)
        : config_(std::move(config)) {}

    [[nodiscard]] bool initialize() override {
        const auto* rotary = config_.rotary_device_name.empty()
                                 ? nullptr
                                 : device_get_binding(config_.rotary_device_name.c_str());
        const auto* keyboard = config_.keyboard_device_name.empty()
                                   ? nullptr
                                   : device_get_binding(config_.keyboard_device_name.c_str());
        const bool rotary_ready = rotary != nullptr && device_is_ready(rotary);
        const bool keyboard_ready = keyboard != nullptr && device_is_ready(keyboard);
        callback_input_enabled_ = true;
        return rotary_ready || keyboard_ready;
    }

    [[nodiscard]] bool callback_input_enabled() const override {
        return callback_input_enabled_;
    }

    void enable_interactive_mode() override {}

    [[nodiscard]] std::optional<shell::ShellNavigationAction> poll_action() override {
        return std::nullopt;
    }

private:
    ZephyrBoardBackendConfig config_;
    bool callback_input_enabled_ {true};
};

}  // namespace

std::unique_ptr<IZephyrShellInputBackend> make_zephyr_shell_input_backend(
    platform::Logger& logger,
    ZephyrBoardRuntime& runtime,
    ZephyrBoardBackendConfig config) {
    switch (runtime.shell_input_backend_profile()) {
        case ZephyrShellInputBackendProfile::TDeckDirect:
#if defined(AEGIS_ZEPHYR_HAS_TDECK_FAMILY)
            return make_tdeck_shell_input_backend(logger, runtime, std::move(config));
#else
            break;
#endif
        case ZephyrShellInputBackendProfile::PagerDirect:
#if defined(AEGIS_ZEPHYR_HAS_TLORA_PAGER_FAMILY)
            return make_tlora_pager_shell_input_backend(logger, runtime, std::move(config));
#else
            break;
#endif
        case ZephyrShellInputBackendProfile::Generic:
        default:
            break;
    }

    return std::make_unique<GenericShellInputBackend>(std::move(config));
}

}  // namespace aegis::ports::zephyr
