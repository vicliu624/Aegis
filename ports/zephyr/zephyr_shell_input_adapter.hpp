#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include <zephyr/drivers/gpio.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>

#include "platform/logging/logger.hpp"
#include "ports/zephyr/zephyr_board_backend_config.hpp"
#include "shell/control/shell_input_model.hpp"

namespace aegis::ports::zephyr {

class ZephyrShellInputAdapter {
public:
    ZephyrShellInputAdapter(platform::Logger& logger, ZephyrBoardBackendConfig config);

    [[nodiscard]] bool initialize();
    void enable_interactive_mode();
    [[nodiscard]] std::optional<shell::ShellNavigationAction> poll_action();
    void record_mapped_action(const input_event& event, shell::ShellNavigationAction action);
    [[nodiscard]] std::optional<shell::ShellNavigationAction> map_input_event(
        const input_event& event);
    void accept_callback_action(shell::ShellNavigationAction action);

private:
    static void rotary_sampler_work_callback(k_work* work);
    [[nodiscard]] std::optional<shell::ShellNavigationAction> map_matrix_key(
        int row,
        int col,
        bool pressed) const;
    void start_rotary_sampler();
    void sample_rotary_inputs();
    void enqueue_action(shell::ShellNavigationAction action);
    [[nodiscard]] std::optional<shell::ShellNavigationAction> poll_direct_rotary_action();
    [[nodiscard]] std::optional<shell::ShellNavigationAction> poll_direct_keyboard_action();
    [[nodiscard]] std::optional<shell::ShellNavigationAction> map_direct_key(uint8_t code,
                                                                             bool pressed);
    void flush_deferred_input_debug();

    platform::Logger& logger_;
    ZephyrBoardBackendConfig config_;
    const struct device* keyboard_i2c_device_ {nullptr};
    gpio_dt_spec rotary_a_gpio_ {};
    gpio_dt_spec rotary_b_gpio_ {};
    gpio_dt_spec rotary_center_gpio_ {};
    bool interactive_mode_enabled_ {false};
    bool keyboard_driver_ready_ {false};
    bool rotary_direct_ready_ {false};
    bool rotary_sampler_started_ {false};
    int keyboard_event_row_ {-1};
    int keyboard_event_col_ {-1};
    uint8_t rotary_fsm_state_ {0};
    bool rotary_center_last_pressed_ {false};
    bool rotary_center_debounced_state_ {false};
    bool rotary_center_last_reading_ {false};
    uint32_t rotary_center_last_debounce_ms_ {0};
    uint32_t last_rotary_action_ms_ {0};
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
    k_work_delayable rotary_sampler_work_ {};
};

}  // namespace aegis::ports::zephyr
