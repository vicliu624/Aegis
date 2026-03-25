#include "ports/zephyr/zephyr_shell_input_backend.hpp"

#include <cstdint>
#include <optional>
#include <utility>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>

#include "ports/zephyr/zephyr_gpio_pin.hpp"

namespace aegis::ports::zephyr {

namespace {

K_MSGQ_DEFINE(g_rotary_action_queue, sizeof(shell::ShellNavigationAction), 16, 4);
K_THREAD_STACK_DEFINE(g_rotary_sampler_stack, 2048);
struct k_thread g_rotary_sampler_thread_data;

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
constexpr uint8_t kRotaryDirCw = 0x10;
constexpr uint8_t kRotaryDirCcw = 0x20;
constexpr uint32_t kRotaryActionCooldownMs = 35;
constexpr uint32_t kRotaryCenterDebounceMs = 30;
constexpr uint32_t kSelectActionCooldownMs = 220;

constexpr uint8_t kRotaryStateTable[7][4] = {
    {kRotaryStart, kRotaryCwBegin, kRotaryCcwBegin, kRotaryStart},
    {kRotaryCwNext, kRotaryStart, kRotaryCwFinal, kRotaryStart | kRotaryDirCw},
    {kRotaryCwNext, kRotaryCwBegin, kRotaryStart, kRotaryStart},
    {kRotaryCwNext, kRotaryCwBegin, kRotaryCwFinal, kRotaryStart},
    {kRotaryCcwNext, kRotaryStart, kRotaryCcwBegin, kRotaryStart},
    {kRotaryCcwNext, kRotaryCcwFinal, kRotaryStart, kRotaryStart | kRotaryDirCcw},
    {kRotaryCcwNext, kRotaryCcwFinal, kRotaryCcwBegin, kRotaryStart},
};

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

class PagerShellInputBackend final : public IZephyrShellInputBackend {
public:
    PagerShellInputBackend(platform::Logger& logger,
                           ZephyrBoardRuntime& runtime,
                           ZephyrBoardBackendConfig config)
        : logger_(logger), runtime_(runtime), config_(std::move(config)) {}

    [[nodiscard]] bool initialize() override {
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
                deferred_rotary_state_ = static_cast<uint8_t>((b << 1) | a);
                deferred_rotary_state_valid_ = true;
            }
            if (rotary_center_gpio_.port != nullptr && device_is_ready(rotary_center_gpio_.port)) {
                rotary_center_last_pressed_ = gpio_pin_get_dt(&rotary_center_gpio_) == 0;
                rotary_center_debounced_state_ = rotary_center_last_pressed_;
                rotary_center_last_reading_ = rotary_center_last_pressed_;
                rotary_center_press_latched_ = rotary_center_last_pressed_;
                deferred_center_pressed_ = rotary_center_last_pressed_;
                deferred_center_valid_ = true;
            }
        }

        logger_.info("input",
                     "pager input backend rotary=" + std::string(rotary_ready ? "ready" : "missing") +
                         " keyboard=" + std::string(keyboard_ready ? "ready" : "missing") +
                         " direct-gpio=" + std::string(direct_gpio_ready ? "ready" : "missing") +
                         " direct-kbd=" + std::string(direct_keyboard_ready ? "ready" : "missing"));
        return rotary_ready || keyboard_ready || direct_gpio_ready || direct_keyboard_ready;
    }

    [[nodiscard]] bool callback_input_enabled() const override {
        return false;
    }

    void enable_interactive_mode() override {
        if (interactive_mode_enabled_ || !rotary_direct_ready_) {
            interactive_mode_enabled_ = true;
            return;
        }

        interactive_mode_enabled_ = true;
        rotary_sampler_started_ = true;
        rotary_sampler_stop_requested_ = false;
        rotary_sampler_thread_ = k_thread_create(&g_rotary_sampler_thread_data,
                                                 g_rotary_sampler_stack,
                                                 K_THREAD_STACK_SIZEOF(g_rotary_sampler_stack),
                                                 rotary_sampler_thread_entry,
                                                 this,
                                                 nullptr,
                                                 nullptr,
                                                 8,
                                                 0,
                                                 K_NO_WAIT);
        k_thread_name_set(rotary_sampler_thread_, "aegis_rotary");
        logger_.info("input", "pager input backend rotary sampler started");
    }

    [[nodiscard]] std::optional<shell::ShellNavigationAction> poll_action() override {
        if (!interactive_mode_enabled_) {
            return std::nullopt;
        }

        flush_debug();

        shell::ShellNavigationAction action {};
        if (k_msgq_get(&g_rotary_action_queue, &action, K_NO_WAIT) == 0) {
            return action;
        }

        return poll_direct_keyboard_action();
    }

private:
    static void rotary_sampler_thread_entry(void* p1, void* p2, void* p3) {
        ARG_UNUSED(p2);
        ARG_UNUSED(p3);
        auto* backend = static_cast<PagerShellInputBackend*>(p1);
        if (backend == nullptr) {
            return;
        }

        while (!backend->rotary_sampler_stop_requested_) {
            if (backend->interactive_mode_enabled_) {
                backend->sample_rotary_inputs();
            }
            k_sleep(K_MSEC(2));
        }
    }

    void enqueue_action(shell::ShellNavigationAction action) {
        auto copy = action;
        if (k_msgq_put(&g_rotary_action_queue, &copy, K_NO_WAIT) != 0) {
            logger_.info("input",
                         "pager input backend queue full dropping=" +
                             std::string(shell::to_string(action)));
        }
    }

    void sample_rotary_inputs() {
        if (!rotary_direct_ready_) {
            return;
        }

        const auto a = gpio_pin_get_dt(&rotary_a_gpio_) > 0 ? 1U : 0U;
        const auto b = gpio_pin_get_dt(&rotary_b_gpio_) > 0 ? 1U : 0U;
        const auto state = static_cast<uint8_t>((b << 1) | a);
        const bool center_pressed =
            rotary_center_gpio_.port != nullptr && device_is_ready(rotary_center_gpio_.port)
                ? (gpio_pin_get_dt(&rotary_center_gpio_) == 0)
                : false;
        const uint32_t now_ms = k_uptime_get_32();

        if (!deferred_rotary_state_valid_ || deferred_rotary_state_ != state ||
            deferred_rotary_a_ != a || deferred_rotary_b_ != b) {
            deferred_rotary_a_ = a;
            deferred_rotary_b_ = b;
            deferred_rotary_state_ = state;
            deferred_rotary_state_valid_ = true;
            deferred_rotary_state_dirty_ = true;
            process_direct_rotary_transition(now_ms);
        }

        if (!deferred_center_valid_ || deferred_center_pressed_ != center_pressed) {
            deferred_center_pressed_ = center_pressed;
            deferred_center_valid_ = true;
            deferred_center_dirty_ = true;
        }
        process_direct_rotary_center(now_ms);
    }

    void process_direct_rotary_transition(uint32_t now_ms) {
        const uint8_t pin_state = deferred_rotary_state_;
        rotary_fsm_state_ = kRotaryStateTable[rotary_fsm_state_ & 0x0F][pin_state];
        const uint8_t emit = static_cast<uint8_t>(rotary_fsm_state_ & 0x30);

        if (emit == kRotaryDirCw && (now_ms - last_rotary_action_ms_) >= kRotaryActionCooldownMs) {
            last_rotary_action_ms_ = now_ms;
            deferred_step_action_ = shell::ShellNavigationAction::MoveNext;
            deferred_step_dirty_ = true;
            enqueue_action(shell::ShellNavigationAction::MoveNext);
        } else if (emit == kRotaryDirCcw &&
                   (now_ms - last_rotary_action_ms_) >= kRotaryActionCooldownMs) {
            last_rotary_action_ms_ = now_ms;
            deferred_step_action_ = shell::ShellNavigationAction::MovePrevious;
            deferred_step_dirty_ = true;
            enqueue_action(shell::ShellNavigationAction::MovePrevious);
        }
    }

    void process_direct_rotary_center(uint32_t now_ms) {
        if (config_.rotary_center_pin < 0) {
            return;
        }

        const bool reading = deferred_center_valid_ ? deferred_center_pressed_ : false;
        if (reading != rotary_center_last_reading_) {
            rotary_center_last_debounce_ms_ = now_ms;
            rotary_center_last_reading_ = reading;
        }
        if ((now_ms - rotary_center_last_debounce_ms_) < kRotaryCenterDebounceMs ||
            reading == rotary_center_debounced_state_) {
            return;
        }

        rotary_center_debounced_state_ = reading;
        if (rotary_center_debounced_state_) {
            rotary_center_last_pressed_ = true;
            if (rotary_center_press_latched_) {
                return;
            }
            rotary_center_press_latched_ = true;
            if ((now_ms - last_select_action_ms_) >= kSelectActionCooldownMs) {
                last_select_action_ms_ = now_ms;
                enqueue_action(shell::ShellNavigationAction::Select);
            }
            return;
        }

        rotary_center_last_pressed_ = false;
        rotary_center_press_latched_ = false;
    }

    [[nodiscard]] std::optional<shell::ShellNavigationAction> poll_direct_keyboard_action() {
        if (keyboard_i2c_device_ == nullptr || !device_is_ready(keyboard_i2c_device_) ||
            config_.keyboard_i2c_address <= 0) {
            return std::nullopt;
        }

        uint8_t pending = 0;
        bool pending_ok = false;
        if (runtime_.ready()) {
            if (!runtime_.keyboard_irq_asserted()) {
                return std::nullopt;
            }
            pending_ok = runtime_.keyboard_pending_event_count(pending);
        } else {
            pending_ok = i2c_reg_read_byte(keyboard_i2c_device_,
                                           static_cast<uint16_t>(config_.keyboard_i2c_address),
                                           kTca8418RegKeyLockEventCount,
                                           &pending) == 0;
        }
        if (!pending_ok) {
            return std::nullopt;
        }

        pending &= 0x0F;
        if (pending == 0U) {
            return std::nullopt;
        }

        std::optional<shell::ShellNavigationAction> first_action;
        for (uint8_t index = 0; index < pending; ++index) {
            uint8_t raw_event = 0;
            bool event_ok = false;
            if (runtime_.ready()) {
                event_ok = runtime_.keyboard_read_event(raw_event);
            } else {
                event_ok = i2c_reg_read_byte(keyboard_i2c_device_,
                                             static_cast<uint16_t>(config_.keyboard_i2c_address),
                                             kTca8418RegKeyEventA,
                                             &raw_event) == 0;
            }
            if (!event_ok) {
                break;
            }

            const bool pressed = (raw_event & kTca8418ReleaseFlag) == 0U;
            const auto mapped = map_direct_key(static_cast<uint8_t>(raw_event & 0x7FU), pressed);
            if (!mapped.has_value()) {
                continue;
            }
            if (!first_action.has_value()) {
                first_action = mapped;
            } else {
                enqueue_action(*mapped);
            }
        }
        return first_action;
    }

    [[nodiscard]] std::optional<shell::ShellNavigationAction> map_direct_key(uint8_t code,
                                                                             bool pressed) const {
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

    void flush_debug() {
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

    platform::Logger& logger_;
    ZephyrBoardRuntime& runtime_;
    ZephyrBoardBackendConfig config_;
    const struct device* keyboard_i2c_device_ {nullptr};
    gpio_dt_spec rotary_a_gpio_ {};
    gpio_dt_spec rotary_b_gpio_ {};
    gpio_dt_spec rotary_center_gpio_ {};
    bool interactive_mode_enabled_ {false};
    bool rotary_direct_ready_ {false};
    bool rotary_sampler_started_ {false};
    bool rotary_sampler_stop_requested_ {false};
    k_tid_t rotary_sampler_thread_ {nullptr};
    uint8_t rotary_fsm_state_ {0};
    bool rotary_center_last_pressed_ {false};
    bool rotary_center_debounced_state_ {false};
    bool rotary_center_last_reading_ {false};
    bool rotary_center_press_latched_ {false};
    uint32_t rotary_center_last_debounce_ms_ {0};
    uint32_t last_rotary_action_ms_ {0};
    uint32_t last_select_action_ms_ {0};
    bool deferred_rotary_state_valid_ {false};
    bool deferred_rotary_state_dirty_ {false};
    uint8_t deferred_rotary_a_ {0};
    uint8_t deferred_rotary_b_ {0};
    uint8_t deferred_rotary_state_ {0};
    bool deferred_center_valid_ {false};
    bool deferred_center_dirty_ {false};
    bool deferred_center_pressed_ {false};
    bool deferred_step_dirty_ {false};
    shell::ShellNavigationAction deferred_step_action_ {shell::ShellNavigationAction::Select};
};

}  // namespace

std::unique_ptr<IZephyrShellInputBackend> make_zephyr_shell_input_backend(
    platform::Logger& logger,
    ZephyrBoardRuntime& runtime,
    ZephyrBoardBackendConfig config) {
    if (runtime.board_direct_input_mode()) {
        return std::make_unique<PagerShellInputBackend>(logger, runtime, std::move(config));
    }
    return std::make_unique<GenericShellInputBackend>(std::move(config));
}

}  // namespace aegis::ports::zephyr
