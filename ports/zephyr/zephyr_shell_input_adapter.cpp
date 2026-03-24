#include "ports/zephyr/zephyr_shell_input_adapter.hpp"

#include <optional>
#include <utility>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include "ports/zephyr/zephyr_gpio_pin.hpp"

namespace aegis::ports::zephyr {

namespace {

ZephyrShellInputAdapter* g_active_adapter = nullptr;
K_MSGQ_DEFINE(g_shell_action_queue, sizeof(shell::ShellNavigationAction), 16, 4);

constexpr uint8_t kTca8418RegKeyLockEventCount = 0x03;
constexpr uint8_t kTca8418RegKeyEventA = 0x04;
constexpr uint8_t kTca8418ReleaseFlag = 0x80;
constexpr uint8_t kRotaryStart = 0x0;
constexpr uint8_t kRotaryCwFinal = 0x1;
constexpr uint8_t kRotaryCwBegin = 0x2;
constexpr uint8_t kRotaryCwNext = 0x3;
constexpr uint8_t kRotaryCcwBegin = 0x4;
constexpr uint8_t kRotaryCcwFinal = 0x5;
constexpr uint8_t kRotaryCcwNext = 0x6;
constexpr uint8_t kRotaryDirNone = 0x0;
constexpr uint8_t kRotaryDirCw = 0x10;
constexpr uint8_t kRotaryDirCcw = 0x20;
constexpr uint32_t kRotaryActionCooldownMs = 35;
constexpr uint32_t kRotaryCenterDebounceMs = 20;

constexpr uint8_t kRotaryStateTable[7][4] = {
    {kRotaryStart, kRotaryCwBegin, kRotaryCcwBegin, kRotaryStart},
    {kRotaryCwNext, kRotaryStart, kRotaryCwFinal, kRotaryStart | kRotaryDirCw},
    {kRotaryCwNext, kRotaryCwBegin, kRotaryStart, kRotaryStart},
    {kRotaryCwNext, kRotaryCwBegin, kRotaryCwFinal, kRotaryStart},
    {kRotaryCcwNext, kRotaryStart, kRotaryCcwBegin, kRotaryStart},
    {kRotaryCcwNext, kRotaryCcwFinal, kRotaryStart, kRotaryStart | kRotaryDirCcw},
    {kRotaryCcwNext, kRotaryCcwFinal, kRotaryCcwBegin, kRotaryStart},
};

void shell_input_event_callback(input_event* event, void* user_data) {
    ARG_UNUSED(user_data);
    if (g_active_adapter == nullptr) {
        return;
    }

    const auto action = g_active_adapter->map_input_event(*event);
    if (!action.has_value()) {
        return;
    }

    auto copy = *action;
    g_active_adapter->accept_callback_action(copy);
    g_active_adapter->record_mapped_action(*event, copy);
}

INPUT_CALLBACK_DEFINE(NULL, shell_input_event_callback, NULL);

}  // namespace

ZephyrShellInputAdapter::ZephyrShellInputAdapter(platform::Logger& logger,
                                                 ZephyrBoardBackendConfig config)
    : logger_(logger), config_(std::move(config)) {}

bool ZephyrShellInputAdapter::initialize() {
    g_active_adapter = nullptr;
#if DT_NODE_EXISTS(DT_NODELABEL(i2c0))
    keyboard_i2c_device_ = DEVICE_DT_GET(DT_NODELABEL(i2c0));
#endif

    const auto* rotary = config_.rotary_device_name.empty()
                             ? nullptr
                             : device_get_binding(config_.rotary_device_name.c_str());
    const bool rotary_ready = rotary != nullptr && device_is_ready(rotary);
    const auto* keyboard = config_.keyboard_device_name.empty()
                               ? nullptr
                               : device_get_binding(config_.keyboard_device_name.c_str());
    const bool keyboard_ready = keyboard != nullptr && device_is_ready(keyboard);
    keyboard_driver_ready_ = keyboard_ready;
    const auto rotary_a_ref = resolve_gpio_pin(config_.rotary_a_pin);
    const auto rotary_b_ref = resolve_gpio_pin(config_.rotary_b_pin);
    const auto rotary_center_ref = resolve_gpio_pin(config_.rotary_center_pin);
    rotary_a_gpio_ = {.port = rotary_a_ref.device,
                      .pin = static_cast<gpio_pin_t>(rotary_a_ref.pin),
                      .dt_flags = 0};
    rotary_b_gpio_ = {.port = rotary_b_ref.device,
                      .pin = static_cast<gpio_pin_t>(rotary_b_ref.pin),
                      .dt_flags = 0};
    rotary_center_gpio_ = {.port = rotary_center_ref.device,
                           .pin = static_cast<gpio_pin_t>(rotary_center_ref.pin),
                           .dt_flags = 0};
    const bool direct_gpio_ready =
        (rotary_a_gpio_.port != nullptr && device_is_ready(rotary_a_gpio_.port)) ||
        (rotary_b_gpio_.port != nullptr && device_is_ready(rotary_b_gpio_.port)) ||
        (rotary_center_gpio_.port != nullptr && device_is_ready(rotary_center_gpio_.port));
    const bool direct_keyboard_ready = keyboard_i2c_device_ != nullptr &&
                                       device_is_ready(keyboard_i2c_device_);

    if (direct_gpio_ready) {
        if (rotary_a_gpio_.port != nullptr && device_is_ready(rotary_a_gpio_.port)) {
            (void)gpio_pin_configure_dt(&rotary_a_gpio_, GPIO_INPUT | GPIO_PULL_UP);
        }
        if (rotary_b_gpio_.port != nullptr && device_is_ready(rotary_b_gpio_.port)) {
            (void)gpio_pin_configure_dt(&rotary_b_gpio_, GPIO_INPUT | GPIO_PULL_UP);
        }
        if (rotary_center_gpio_.port != nullptr && device_is_ready(rotary_center_gpio_.port)) {
            (void)gpio_pin_configure_dt(&rotary_center_gpio_, GPIO_INPUT | GPIO_PULL_UP);
        }
        if (rotary_a_gpio_.port != nullptr && device_is_ready(rotary_a_gpio_.port) &&
            rotary_b_gpio_.port != nullptr && device_is_ready(rotary_b_gpio_.port)) {
            const auto a = gpio_pin_get_dt(&rotary_a_gpio_) > 0 ? 1U : 0U;
            const auto b = gpio_pin_get_dt(&rotary_b_gpio_) > 0 ? 1U : 0U;
            rotary_fsm_state_ = kRotaryStart;
            rotary_direct_ready_ = true;
            deferred_rotary_a_ = a;
            deferred_rotary_b_ = b;
            deferred_rotary_state_ = static_cast<uint8_t>((a << 1) | b);
            deferred_rotary_state_valid_ = true;
        }
        if (rotary_center_gpio_.port != nullptr && device_is_ready(rotary_center_gpio_.port)) {
            rotary_center_last_pressed_ = gpio_pin_get_dt(&rotary_center_gpio_) == 0;
            rotary_center_debounced_state_ = rotary_center_last_pressed_;
            rotary_center_last_reading_ = rotary_center_last_pressed_;
            deferred_center_pressed_ = rotary_center_last_pressed_;
            deferred_center_valid_ = true;
        }
    }

    if (rotary_direct_ready_) {
        k_work_init_delayable(&rotary_sampler_work_, ZephyrShellInputAdapter::rotary_sampler_work_callback);
    }

    logger_.info("input",
                 "shell input adapter rotary=" + std::string(rotary_ready ? "ready" : "missing") +
                     " keyboard=" + std::string(keyboard_ready ? "ready" : "missing") +
                     " direct-gpio=" + std::string(direct_gpio_ready ? "ready" : "missing") +
                     " direct-kbd=" + std::string(direct_keyboard_ready ? "ready" : "missing"));
    return rotary_ready || keyboard_ready || direct_gpio_ready || direct_keyboard_ready;
}

void ZephyrShellInputAdapter::enable_interactive_mode() {
    if (interactive_mode_enabled_) {
        return;
    }

    interactive_mode_enabled_ = true;
    g_active_adapter = this;
    start_rotary_sampler();
    logger_.info("input", "interactive input mode enabled");
}

void ZephyrShellInputAdapter::accept_callback_action(shell::ShellNavigationAction action) {
    const int rc = k_msgq_put(&g_shell_action_queue, &action, K_NO_WAIT);
    if (rc != 0) {
        printk("AEGIS TRACE: mapped action queue put failed rc=%d action=%s\n",
               rc,
               std::string(shell::to_string(action)).c_str());
    }
}

std::optional<shell::ShellNavigationAction> ZephyrShellInputAdapter::poll_action() {
    if (!interactive_mode_enabled_) {
        return std::nullopt;
    }

    flush_deferred_input_debug();

    shell::ShellNavigationAction action {};
    if (k_msgq_get(&g_shell_action_queue, &action, K_NO_WAIT) == 0) {
        logger_.info("input", "dispatch action " + std::string(shell::to_string(action)));
        return action;
    }
    if (const auto direct_keyboard = poll_direct_keyboard_action(); direct_keyboard.has_value()) {
        logger_.info("input",
                     "dispatch direct keyboard action " + std::string(shell::to_string(*direct_keyboard)));
        return direct_keyboard;
    }
    return std::nullopt;
}

void ZephyrShellInputAdapter::record_mapped_action(const input_event& event,
                                                   shell::ShellNavigationAction action) {
    logger_.info("input",
                 "mapped event type=" + std::to_string(event.type) + " code=" +
                     std::to_string(event.code) + " value=" + std::to_string(event.value) +
                     " action=" + std::string(shell::to_string(action)));
}

void ZephyrShellInputAdapter::rotary_sampler_work_callback(k_work* work) {
    ARG_UNUSED(work);
    if (g_active_adapter == nullptr || !g_active_adapter->interactive_mode_enabled_) {
        return;
    }

    g_active_adapter->sample_rotary_inputs();
    (void)k_work_schedule(&g_active_adapter->rotary_sampler_work_, K_MSEC(2));
}

void ZephyrShellInputAdapter::start_rotary_sampler() {
    if (!rotary_direct_ready_ || rotary_sampler_started_) {
        return;
    }

    rotary_sampler_started_ = true;
    (void)k_work_schedule(&rotary_sampler_work_, K_NO_WAIT);
    logger_.info("input", "direct rotary sampler started");
}

void ZephyrShellInputAdapter::sample_rotary_inputs() {
    if (!rotary_direct_ready_) {
        return;
    }

    const auto a = gpio_pin_get_dt(&rotary_a_gpio_) > 0 ? 1U : 0U;
    const auto b = gpio_pin_get_dt(&rotary_b_gpio_) > 0 ? 1U : 0U;
    const auto state = static_cast<uint8_t>((a << 1) | b);
    const bool center_pressed = rotary_center_gpio_.port != nullptr && device_is_ready(rotary_center_gpio_.port)
                                    ? (gpio_pin_get_dt(&rotary_center_gpio_) == 0)
                                    : false;

    if (!deferred_rotary_state_valid_ || deferred_rotary_state_ != state || deferred_rotary_a_ != a ||
        deferred_rotary_b_ != b) {
        deferred_rotary_a_ = a;
        deferred_rotary_b_ = b;
        deferred_rotary_state_ = state;
        deferred_rotary_state_valid_ = true;
        deferred_rotary_state_dirty_ = true;
    }
    if (!deferred_center_valid_ || deferred_center_pressed_ != center_pressed) {
        deferred_center_pressed_ = center_pressed;
        deferred_center_valid_ = true;
        deferred_center_dirty_ = true;
    }

    if (const auto direct_rotary = poll_direct_rotary_action(); direct_rotary.has_value()) {
        enqueue_action(*direct_rotary);
    }
}

void ZephyrShellInputAdapter::enqueue_action(shell::ShellNavigationAction action) {
    auto copy = action;
    if (k_msgq_put(&g_shell_action_queue, &copy, K_NO_WAIT) != 0) {
        logger_.info("input", "shell action queue full dropping=" + std::string(shell::to_string(action)));
    }
}

std::optional<shell::ShellNavigationAction> ZephyrShellInputAdapter::map_input_event(
    const input_event& event) {
    if (event.type == INPUT_EV_ABS) {
        if (event.code == INPUT_ABS_X) {
            keyboard_event_col_ = event.value;
        } else if (event.code == INPUT_ABS_Y) {
            keyboard_event_row_ = event.value;
        }
        return std::nullopt;
    }

    if (event.type == INPUT_EV_REL && event.code == INPUT_REL_WHEEL) {
        if (rotary_direct_ready_) {
            return std::nullopt;
        }
        if (event.value > 0) {
            return shell::ShellNavigationAction::MoveNext;
        }
        if (event.value < 0) {
            return shell::ShellNavigationAction::MovePrevious;
        }
        return std::nullopt;
    }

    if (event.type != INPUT_EV_KEY || event.value == 0) {
        return std::nullopt;
    }

    if (event.code == INPUT_BTN_TOUCH && keyboard_event_row_ >= 0 && keyboard_event_col_ >= 0) {
        return map_matrix_key(keyboard_event_row_, keyboard_event_col_, event.value != 0);
    }

    switch (event.code) {
        case INPUT_KEY_ENTER:
            return shell::ShellNavigationAction::Select;
        case INPUT_KEY_LEFT:
            return shell::ShellNavigationAction::MovePrevious;
        case INPUT_KEY_RIGHT:
            return shell::ShellNavigationAction::MoveNext;
        case INPUT_KEY_ESC:
        case INPUT_KEY_BACK:
        case INPUT_KEY_BACKSPACE:
            return shell::ShellNavigationAction::Back;
        case INPUT_KEY_MENU:
            return shell::ShellNavigationAction::OpenMenu;
        case INPUT_KEY_F1:
            return shell::ShellNavigationAction::OpenSettings;
        case INPUT_KEY_F2:
            return shell::ShellNavigationAction::OpenNotifications;
        default:
            return std::nullopt;
    }
}

std::optional<shell::ShellNavigationAction> ZephyrShellInputAdapter::map_matrix_key(
    int row,
    int col,
    bool pressed) const {
    if (!pressed) {
        return std::nullopt;
    }

    logger_.info("input",
                 "keyboard matrix row=" + std::to_string(row) +
                     " col=" + std::to_string(col));

    if (row == config_.shell_select_row && col == config_.shell_select_col) {
        return shell::ShellNavigationAction::Select;
    }
    if (row == config_.shell_back_row && col == config_.shell_back_col) {
        return shell::ShellNavigationAction::Back;
    }
    if (row == config_.shell_menu_row && col == config_.shell_menu_col) {
        return shell::ShellNavigationAction::OpenMenu;
    }
    if (row == config_.shell_settings_row && col == config_.shell_settings_col) {
        return shell::ShellNavigationAction::OpenSettings;
    }
    if (row == config_.shell_notifications_row && col == config_.shell_notifications_col) {
        return shell::ShellNavigationAction::OpenNotifications;
    }

    return std::nullopt;
}

std::optional<shell::ShellNavigationAction> ZephyrShellInputAdapter::poll_direct_rotary_action() {
    if (!rotary_direct_ready_ || config_.rotary_a_pin < 0 || config_.rotary_b_pin < 0) {
        return std::nullopt;
    }

    if (rotary_a_gpio_.port == nullptr || !device_is_ready(rotary_a_gpio_.port) ||
        rotary_b_gpio_.port == nullptr || !device_is_ready(rotary_b_gpio_.port)) {
        return std::nullopt;
    }

    const uint32_t now_ms = k_uptime_get_32();
    const auto a = gpio_pin_get_dt(&rotary_a_gpio_) > 0 ? 1U : 0U;
    const auto b = gpio_pin_get_dt(&rotary_b_gpio_) > 0 ? 1U : 0U;
    const uint8_t pin_state = static_cast<uint8_t>((b << 1) | a);
    rotary_fsm_state_ = kRotaryStateTable[rotary_fsm_state_ & 0x0F][pin_state];
    const uint8_t emit = static_cast<uint8_t>(rotary_fsm_state_ & 0x30);

    if (emit == kRotaryDirCw && (now_ms - last_rotary_action_ms_) >= kRotaryActionCooldownMs) {
        last_rotary_action_ms_ = now_ms;
        deferred_step_action_ = shell::ShellNavigationAction::MoveNext;
        deferred_step_dirty_ = true;
        return shell::ShellNavigationAction::MoveNext;
    }
    if (emit == kRotaryDirCcw && (now_ms - last_rotary_action_ms_) >= kRotaryActionCooldownMs) {
        last_rotary_action_ms_ = now_ms;
        deferred_step_action_ = shell::ShellNavigationAction::MovePrevious;
        deferred_step_dirty_ = true;
        return shell::ShellNavigationAction::MovePrevious;
    }

    if (config_.rotary_center_pin >= 0) {
        const bool reading = rotary_center_gpio_.port != nullptr && device_is_ready(rotary_center_gpio_.port)
                                 ? (gpio_pin_get_dt(&rotary_center_gpio_) == 0)
                                 : false;
        if (reading != rotary_center_last_reading_) {
            rotary_center_last_debounce_ms_ = now_ms;
            rotary_center_last_reading_ = reading;
        }
        if ((now_ms - rotary_center_last_debounce_ms_) >= kRotaryCenterDebounceMs &&
            reading != rotary_center_debounced_state_) {
            rotary_center_debounced_state_ = reading;
            if (rotary_center_debounced_state_ && !rotary_center_last_pressed_) {
                rotary_center_last_pressed_ = true;
                return shell::ShellNavigationAction::Select;
            }
            if (!rotary_center_debounced_state_) {
                rotary_center_last_pressed_ = false;
            }
        }
    }

    return std::nullopt;
}

std::optional<shell::ShellNavigationAction> ZephyrShellInputAdapter::poll_direct_keyboard_action() {
    if (!interactive_mode_enabled_ || keyboard_driver_ready_) {
        return std::nullopt;
    }
    if (keyboard_i2c_device_ == nullptr || !device_is_ready(keyboard_i2c_device_) ||
        config_.keyboard_i2c_address <= 0) {
        return std::nullopt;
    }

    uint8_t pending = 0;
    if (i2c_reg_read_byte(keyboard_i2c_device_,
                          static_cast<uint16_t>(config_.keyboard_i2c_address),
                          kTca8418RegKeyLockEventCount,
                          &pending) != 0) {
        return std::nullopt;
    }

    pending &= 0x0F;
    if (pending == 0U) {
        return std::nullopt;
    }

    uint8_t raw_event = 0;
    if (i2c_reg_read_byte(keyboard_i2c_device_,
                          static_cast<uint16_t>(config_.keyboard_i2c_address),
                          kTca8418RegKeyEventA,
                          &raw_event) != 0) {
        return std::nullopt;
    }

    const bool pressed = (raw_event & kTca8418ReleaseFlag) == 0U;
    const auto code = static_cast<uint8_t>(raw_event & 0x7FU);
    logger_.info("input",
                 "direct keyboard raw=" + std::to_string(code) +
                     " pressed=" + std::string(pressed ? "1" : "0"));
    return map_direct_key(code, pressed);
}

void ZephyrShellInputAdapter::flush_deferred_input_debug() {
    if (deferred_rotary_state_dirty_) {
        deferred_rotary_state_dirty_ = false;
        logger_.info("input",
                     "rotary raw a=" + std::to_string(deferred_rotary_a_) +
                         " b=" + std::to_string(deferred_rotary_b_) +
                         " state=" + std::to_string(deferred_rotary_state_));
    }

    if (deferred_center_dirty_) {
        deferred_center_dirty_ = false;
        logger_.info("input",
                     std::string("rotary center raw pressed=") +
                         (deferred_center_pressed_ ? "1" : "0"));
    }

    if (deferred_step_dirty_) {
        deferred_step_dirty_ = false;
        logger_.info("input",
                     "direct rotary step=" + std::string(shell::to_string(deferred_step_action_)));
    }
}

std::optional<shell::ShellNavigationAction> ZephyrShellInputAdapter::map_direct_key(uint8_t code,
                                                                                     bool pressed) {
    if (!pressed) {
        return std::nullopt;
    }

    switch (code) {
        case 1:
            return shell::ShellNavigationAction::MovePrevious;
        case 2:
            return shell::ShellNavigationAction::MoveNext;
        case 20:
            return shell::ShellNavigationAction::Select;
        case 21:
            return shell::ShellNavigationAction::OpenMenu;
        case 29:
            return shell::ShellNavigationAction::OpenSettings;
        case 30:
            return shell::ShellNavigationAction::Back;
        case 31:
            return shell::ShellNavigationAction::OpenNotifications;
        default:
            return std::nullopt;
    }
}

}  // namespace aegis::ports::zephyr
