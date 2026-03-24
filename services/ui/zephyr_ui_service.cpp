#include "services/ui/zephyr_ui_service.hpp"

#include <string>
#include <utility>

#include <zephyr/device.h>
#include <zephyr/drivers/display.h>

namespace aegis::services {

namespace {

const ::device* find_device(const std::string& name) {
    if (name.empty()) {
        return nullptr;
    }
    return device_get_binding(name.c_str());
}

bool ready(const std::string& name) {
    const auto* device = find_device(name);
    return device != nullptr && device_is_ready(device);
}

}  // namespace

ZephyrDisplayService::ZephyrDisplayService(ports::zephyr::ZephyrBoardBackendConfig config)
    : config_(std::move(config)) {}

DisplayInfo ZephyrDisplayService::display_info() const {
    auto info = fallback_info();
    const auto* device = find_device(config_.display_device_name);
    if (device == nullptr || !device_is_ready(device)) {
        info.surface_description =
            "zephyr-display-unavailable:" + config_.display_device_name + " (" + config_.notes + ")";
        return info;
    }

    display_capabilities caps {};
    display_get_capabilities(device, &caps);
    info.width = static_cast<int>(caps.x_resolution);
    info.height = static_cast<int>(caps.y_resolution);
    info.surface_description = "zephyr-display:" + config_.display_device_name +
                               " backlight_gpio=" + std::to_string(config_.display_backlight_pin);
    return info;
}

std::string ZephyrDisplayService::describe_surface() const {
    return display_info().surface_description;
}

DisplayInfo ZephyrDisplayService::fallback_info() const {
    return DisplayInfo {
        .width = config_.display_width,
        .height = config_.display_height,
        .touch = config_.display_touch,
        .layout_class = config_.display_layout_class,
        .surface_description = "declared-display:" + config_.display_device_name,
    };
}

ZephyrInputService::ZephyrInputService(ports::zephyr::ZephyrBoardBackendConfig config)
    : config_(std::move(config)) {}

InputInfo ZephyrInputService::input_info() const {
    const bool has_rotary = rotary_ready();
    const bool has_keyboard = keyboard_ready();
    return InputInfo {
        .keyboard = has_keyboard,
        .pointer = false,
        .joystick = has_rotary,
        .primary_input = has_keyboard
                             ? (has_rotary ? "keyboard+rotary" : "keyboard")
                             : (has_rotary ? "rotary+center" : "buttons"),
        .input_mode =
            std::string("zephyr-input:rotary=") + (has_rotary ? "ready" : "missing") +
            ",keyboard=" + (has_keyboard ? "tca8418-ready" : "absent"),
    };
}

std::string ZephyrInputService::describe_input_mode() const {
    return input_info().input_mode;
}

bool ZephyrInputService::rotary_ready() const {
    return ready(config_.rotary_device_name);
}

bool ZephyrInputService::keyboard_ready() const {
    if (!config_.keyboard_input || config_.keyboard_device_name.empty()) {
        return false;
    }

    return ready(config_.keyboard_device_name);
}

}  // namespace aegis::services
