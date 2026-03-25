#include "ports/zephyr/boards/tdeck/tdeck_shell_input_backend.hpp"

#include <cstdint>
#include <optional>
#include <utility>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#include "ports/zephyr/zephyr_gpio_pin.hpp"

namespace aegis::ports::zephyr {

namespace {

class TdeckShellInputBackend final : public IZephyrShellInputBackend {
public:
    TdeckShellInputBackend(platform::Logger& logger,
                           ZephyrBoardRuntime& runtime,
                           ZephyrBoardBackendConfig config)
        : logger_(logger), runtime_(runtime), config_(std::move(config)) {}

    [[nodiscard]] bool initialize() override {
        initialize_pin(trackball_up_gpio_, config_.trackball_up_pin);
        initialize_pin(trackball_down_gpio_, config_.trackball_down_pin);
        initialize_pin(trackball_left_gpio_, config_.trackball_left_pin);
        initialize_pin(trackball_right_gpio_, config_.trackball_right_pin);
        initialize_pin(trackball_click_gpio_, config_.trackball_click_pin);

        trackball_ready_ =
            trackball_up_gpio_.port != nullptr || trackball_down_gpio_.port != nullptr ||
            trackball_left_gpio_.port != nullptr || trackball_right_gpio_.port != nullptr ||
            trackball_click_gpio_.port != nullptr;
        logger_.info("input",
                     "tdeck input backend trackball=" +
                         std::string(trackball_ready_ ? "ready" : "missing") +
                         " keyboard=" + std::string(runtime_.keyboard_ready() ? "ready" : "missing"));
        return trackball_ready_ || runtime_.keyboard_ready();
    }

    [[nodiscard]] bool callback_input_enabled() const override {
        return false;
    }

    void enable_interactive_mode() override {
        interactive_mode_enabled_ = true;
    }

    [[nodiscard]] std::optional<shell::ShellNavigationAction> poll_action() override {
        if (!interactive_mode_enabled_) {
            return std::nullopt;
        }

        if (auto action = poll_trackball_action(); action.has_value()) {
            return action;
        }
        return poll_keyboard_action();
    }

private:
    void initialize_pin(gpio_dt_spec& spec, int pin) {
        if (pin < 0) {
            spec = {};
            return;
        }
        const auto pin_ref = resolve_gpio_pin(pin);
        spec = {.port = pin_ref.device, .pin = static_cast<gpio_pin_t>(pin_ref.pin), .dt_flags = 0};
        if (spec.port != nullptr && device_is_ready(spec.port)) {
            (void)gpio_pin_configure_dt(&spec, GPIO_INPUT | GPIO_PULL_UP);
        } else {
            spec = {};
        }
    }

    [[nodiscard]] std::optional<shell::ShellNavigationAction> poll_trackball_action() {
        if (!trackball_ready_) {
            return std::nullopt;
        }
        const uint32_t now_ms = k_uptime_get_32();
        if ((now_ms - last_trackball_action_ms_) < 90U) {
            return std::nullopt;
        }

        const bool up_pressed = pin_pressed(trackball_up_gpio_);
        const bool down_pressed = pin_pressed(trackball_down_gpio_);
        const bool left_pressed = pin_pressed(trackball_left_gpio_);
        const bool right_pressed = pin_pressed(trackball_right_gpio_);
        const bool click_pressed = pin_pressed(trackball_click_gpio_);

        auto emit_direction = [&](bool pressed,
                                  bool& latched,
                                  shell::ShellNavigationAction action) -> std::optional<shell::ShellNavigationAction> {
            if (pressed && !latched) {
                latched = true;
                last_trackball_action_ms_ = now_ms;
                return action;
            }
            if (!pressed) {
                latched = false;
            }
            return std::nullopt;
        };

        if (auto action = emit_direction(up_pressed || left_pressed,
                                         up_left_latched_,
                                         shell::ShellNavigationAction::MovePrevious);
            action.has_value()) {
            return action;
        }
        if (auto action = emit_direction(down_pressed || right_pressed,
                                         down_right_latched_,
                                         shell::ShellNavigationAction::MoveNext);
            action.has_value()) {
            return action;
        }
        if (auto action = emit_direction(click_pressed,
                                         click_latched_,
                                         shell::ShellNavigationAction::Select);
            action.has_value()) {
            return action;
        }

        return std::nullopt;
    }

    [[nodiscard]] std::optional<shell::ShellNavigationAction> poll_keyboard_action() {
        if (!runtime_.keyboard_ready()) {
            return std::nullopt;
        }
        const uint32_t now_ms = k_uptime_get_32();
        if ((now_ms - last_keyboard_action_ms_) < 30U) {
            return std::nullopt;
        }

        uint8_t raw_character = 0;
        if (!runtime_.keyboard_read_character(raw_character)) {
            return std::nullopt;
        }

        last_keyboard_action_ms_ = now_ms;
        logger_.info("input", "tdeck keyboard raw=" + std::to_string(raw_character));
        switch (raw_character) {
            case '\r':
            case '\n':
                return shell::ShellNavigationAction::Select;
            case '\b':
                return shell::ShellNavigationAction::Back;
            case ' ':
                return shell::ShellNavigationAction::OpenMenu;
            case '[':
                return shell::ShellNavigationAction::MovePrevious;
            case ']':
                return shell::ShellNavigationAction::MoveNext;
            case 's':
            case 'S':
                return shell::ShellNavigationAction::OpenSettings;
            case 'n':
            case 'N':
                return shell::ShellNavigationAction::OpenNotifications;
            default:
                return std::nullopt;
        }
    }

    [[nodiscard]] bool pin_pressed(const gpio_dt_spec& spec) const {
        return spec.port != nullptr && device_is_ready(spec.port) && gpio_pin_get_dt(&spec) == 0;
    }

    platform::Logger& logger_;
    ZephyrBoardRuntime& runtime_;
    ZephyrBoardBackendConfig config_;
    gpio_dt_spec trackball_up_gpio_ {};
    gpio_dt_spec trackball_down_gpio_ {};
    gpio_dt_spec trackball_left_gpio_ {};
    gpio_dt_spec trackball_right_gpio_ {};
    gpio_dt_spec trackball_click_gpio_ {};
    bool interactive_mode_enabled_ {false};
    bool trackball_ready_ {false};
    bool up_left_latched_ {false};
    bool down_right_latched_ {false};
    bool click_latched_ {false};
    uint32_t last_trackball_action_ms_ {0};
    uint32_t last_keyboard_action_ms_ {0};
};

}  // namespace

std::unique_ptr<IZephyrShellInputBackend> make_tdeck_shell_input_backend(
    platform::Logger& logger,
    ZephyrBoardRuntime& runtime,
    ZephyrBoardBackendConfig config) {
    return std::make_unique<TdeckShellInputBackend>(logger, runtime, std::move(config));
}

}  // namespace aegis::ports::zephyr
