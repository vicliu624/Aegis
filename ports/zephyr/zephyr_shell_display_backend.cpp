#include "ports/zephyr/zephyr_shell_display_backend.hpp"

#include <utility>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>

#if defined(AEGIS_ZEPHYR_HAS_TDECK_FAMILY)
#include "ports/zephyr/boards/tdeck/tdeck_shell_display_backend.hpp"
#endif
#if defined(AEGIS_ZEPHYR_HAS_TLORA_PAGER_FAMILY)
#include "ports/zephyr/boards/tlora_pager/tlora_pager_shell_display_backend.hpp"
#endif

namespace aegis::ports::zephyr {

namespace {

const ::device* find_device(const std::string& name) {
    if (name.empty()) {
        return nullptr;
    }
    return device_get_binding(name.c_str());
}

const ::device* dt_display_device() {
#if DT_NODE_EXISTS(DT_ALIAS(display0)) && DT_NODE_HAS_STATUS(DT_ALIAS(display0), okay)
    return device_get_binding(DEVICE_DT_NAME(DT_ALIAS(display0)));
#else
    return nullptr;
#endif
}

class GenericDisplayBackend final : public IZephyrShellDisplayBackend {
public:
    explicit GenericDisplayBackend(ZephyrBoardBackendConfig config)
        : config_(std::move(config)) {}

    [[nodiscard]] bool initialize() override {
        display_device_ = find_device(config_.display_device_name);
        if (display_device_ == nullptr) {
            display_device_ = dt_display_device();
        }
        if (display_device_ == nullptr || !device_is_ready(display_device_)) {
            return false;
        }
        (void)display_blanking_off(display_device_);
        return true;
    }

    void write_region(int x,
                      int y,
                      int width,
                      int height,
                      const uint16_t* pixels,
                      std::size_t count) const override {
        if (display_device_ == nullptr || !device_is_ready(display_device_) || pixels == nullptr || count == 0) {
            return;
        }
        display_buffer_descriptor desc {
            .buf_size = static_cast<uint32_t>(count * sizeof(uint16_t)),
            .width = static_cast<uint16_t>(width),
            .height = static_cast<uint16_t>(height),
            .pitch = static_cast<uint16_t>(width),
            .frame_incomplete = false,
        };
        display_write(display_device_, x, y, &desc, pixels);
    }

private:
    ZephyrBoardBackendConfig config_;
    const struct device* display_device_ {nullptr};
};

}  // namespace

std::unique_ptr<IZephyrShellDisplayBackend> make_zephyr_shell_display_backend(
    platform::Logger& logger,
    ZephyrBoardRuntime& runtime,
    ZephyrBoardBackendConfig config) {
    switch (runtime.shell_display_backend_profile()) {
        case ZephyrShellDisplayBackendProfile::TDeck:
#if defined(AEGIS_ZEPHYR_HAS_TDECK_FAMILY)
            return make_tdeck_shell_display_backend(logger, runtime, std::move(config));
#else
            break;
#endif
        case ZephyrShellDisplayBackendProfile::Pager:
#if defined(AEGIS_ZEPHYR_HAS_TLORA_PAGER_FAMILY)
            return make_tlora_pager_shell_display_backend(logger, runtime, std::move(config));
#else
            break;
#endif
        case ZephyrShellDisplayBackendProfile::Generic:
        default:
            break;
    }

    return std::make_unique<GenericDisplayBackend>(std::move(config));
}

}  // namespace aegis::ports::zephyr
