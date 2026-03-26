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
        interactive_mode_started_ms_ = k_uptime_get_32();
        up_idle_level_ = pin_level(trackball_up_gpio_);
        down_idle_level_ = pin_level(trackball_down_gpio_);
        left_idle_level_ = pin_level(trackball_left_gpio_);
        right_idle_level_ = pin_level(trackball_right_gpio_);
        click_idle_level_ = pin_level(trackball_click_gpio_);
        up_latched_ = false;
        down_latched_ = false;
        left_latched_ = false;
        right_latched_ = false;
        click_latched_ = false;
        last_logged_up_level_ = up_idle_level_;
        last_logged_down_level_ = down_idle_level_;
        last_logged_left_level_ = left_idle_level_;
        last_logged_right_level_ = right_idle_level_;
        last_logged_click_level_ = click_idle_level_;
        interactive_inputs_armed_ = false;
        keyboard_inputs_armed_ = false;
        stable_release_started_ms_ = 0;
        last_trackball_action_ms_ = interactive_mode_started_ms_;
        last_keyboard_action_ms_ = interactive_mode_started_ms_;
        logger_.info("input",
                     "tdeck trackball idle baseline up=" + std::to_string(up_idle_level_ ? 1 : 0) +
                         " down=" + std::to_string(down_idle_level_ ? 1 : 0) +
                         " left=" + std::to_string(left_idle_level_ ? 1 : 0) +
                         " right=" + std::to_string(right_idle_level_ ? 1 : 0) +
                         " click=" + std::to_string(click_idle_level_ ? 1 : 0));
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
        if ((now_ms - interactive_mode_started_ms_) < 500U) {
            return std::nullopt;
        }
        if ((now_ms - last_trackball_action_ms_) < 45U) {
            return std::nullopt;
        }

        const bool up_level = pin_level(trackball_up_gpio_);
        const bool down_level = pin_level(trackball_down_gpio_);
        const bool left_level = pin_level(trackball_left_gpio_);
        const bool right_level = pin_level(trackball_right_gpio_);
        const bool click_level = pin_level(trackball_click_gpio_);
        log_trackball_state_change(up_level, down_level, left_level, right_level, click_level);
        const bool up_pressed = up_level != up_idle_level_;
        const bool down_pressed = down_level != down_idle_level_;
        const bool left_pressed = left_level != left_idle_level_;
        const bool right_pressed = right_level != right_idle_level_;
        const bool click_pressed = click_level != click_idle_level_;
        const bool any_pressed =
            up_pressed || down_pressed || left_pressed || right_pressed || click_pressed;

        if (!interactive_inputs_armed_) {
            if (any_pressed) {
                stable_release_started_ms_ = 0;
                up_latched_ = up_pressed;
                down_latched_ = down_pressed;
                left_latched_ = left_pressed;
                right_latched_ = right_pressed;
                click_latched_ = click_pressed;
                return std::nullopt;
            }
            if (stable_release_started_ms_ == 0) {
                stable_release_started_ms_ = now_ms;
                return std::nullopt;
            }
            if ((now_ms - stable_release_started_ms_) < 120U) {
                return std::nullopt;
            }
            interactive_inputs_armed_ = true;
        }

        auto emit_direction = [&](bool pressed,
                                  bool& latched,
                                  shell::ShellNavigationAction action,
                                  const char* source) -> std::optional<shell::ShellNavigationAction> {
            if (pressed && !latched) {
                latched = true;
                last_trackball_action_ms_ = now_ms;
                logger_.info("input", std::string("tdeck trackball source=") + source);
                return action;
            }
            if (!pressed) {
                latched = false;
            }
            return std::nullopt;
        };

        if (auto action = emit_direction(up_pressed,
                                         up_latched_,
                                         shell::ShellNavigationAction::MovePrevious,
                                         "up");
            action.has_value()) {
            return action;
        }
        if (auto action = emit_direction(left_pressed,
                                         left_latched_,
                                         shell::ShellNavigationAction::MovePrevious,
                                         "left");
            action.has_value()) {
            return action;
        }
        if (auto action = emit_direction(down_pressed,
                                         down_latched_,
                                         shell::ShellNavigationAction::MoveNext,
                                         "down");
            action.has_value()) {
            return action;
        }
        if (auto action = emit_direction(right_pressed,
                                         right_latched_,
                                         shell::ShellNavigationAction::MoveNext,
                                         "right");
            action.has_value()) {
            return action;
        }
        if (auto action = emit_direction(click_pressed,
                                         click_latched_,
                                         shell::ShellNavigationAction::Select,
                                         "click");
            action.has_value()) {
            return action;
        }

        return std::nullopt;
    }

    void log_trackball_state_change(bool up_level,
                                    bool down_level,
                                    bool left_level,
                                    bool right_level,
                                    bool click_level) {
        if (up_level == last_logged_up_level_ && down_level == last_logged_down_level_ &&
            left_level == last_logged_left_level_ && right_level == last_logged_right_level_ &&
            click_level == last_logged_click_level_) {
            return;
        }

        last_logged_up_level_ = up_level;
        last_logged_down_level_ = down_level;
        last_logged_left_level_ = left_level;
        last_logged_right_level_ = right_level;
        last_logged_click_level_ = click_level;
        logger_.info("input",
                     "tdeck trackball raw-level up=" + std::to_string(up_level ? 1 : 0) +
                         " down=" + std::to_string(down_level ? 1 : 0) +
                         " left=" + std::to_string(left_level ? 1 : 0) +
                         " right=" + std::to_string(right_level ? 1 : 0) +
                         " click=" + std::to_string(click_level ? 1 : 0));
    }

    [[nodiscard]] std::optional<shell::ShellNavigationAction> poll_keyboard_action() {
        if (!runtime_.keyboard_ready()) {
            return std::nullopt;
        }
        const uint32_t now_ms = k_uptime_get_32();
        if (!keyboard_inputs_armed_) {
            uint8_t discarded_character = 0;
            if (runtime_.keyboard_read_character(discarded_character)) {
                last_keyboard_action_ms_ = now_ms;
            }
            if ((now_ms - interactive_mode_started_ms_) < 800U) {
                return std::nullopt;
            }
            keyboard_inputs_armed_ = true;
            return std::nullopt;
        }
        if ((now_ms - last_keyboard_action_ms_) < 30U) {
            return std::nullopt;
        }

        uint8_t raw_character = 0;
        if (!runtime_.keyboard_read_character(raw_character)) {
            return std::nullopt;
        }

        last_keyboard_action_ms_ = now_ms;
        logger_.info("input", "tdeck keyboard source raw=" + std::to_string(raw_character));
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

    [[nodiscard]] bool pin_level(const gpio_dt_spec& spec) const {
        return spec.port != nullptr && device_is_ready(spec.port) && gpio_pin_get_dt(&spec) > 0;
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
    bool up_latched_ {false};
    bool down_latched_ {false};
    bool left_latched_ {false};
    bool right_latched_ {false};
    bool click_latched_ {false};
    bool up_idle_level_ {false};
    bool down_idle_level_ {false};
    bool left_idle_level_ {false};
    bool right_idle_level_ {false};
    bool click_idle_level_ {false};
    bool last_logged_up_level_ {false};
    bool last_logged_down_level_ {false};
    bool last_logged_left_level_ {false};
    bool last_logged_right_level_ {false};
    bool last_logged_click_level_ {false};
    bool interactive_inputs_armed_ {false};
    bool keyboard_inputs_armed_ {false};
    uint32_t last_trackball_action_ms_ {0};
    uint32_t last_keyboard_action_ms_ {0};
    uint32_t interactive_mode_started_ms_ {0};
    uint32_t stable_release_started_ms_ {0};
};

}  // namespace

std::unique_ptr<IZephyrShellInputBackend> make_tdeck_shell_input_backend(
    platform::Logger& logger,
    ZephyrBoardRuntime& runtime,
    ZephyrBoardBackendConfig config) {
    return std::make_unique<TdeckShellInputBackend>(logger, runtime, std::move(config));
}

}  // namespace aegis::ports::zephyr
