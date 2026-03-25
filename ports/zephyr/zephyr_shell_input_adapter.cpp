#include "ports/zephyr/zephyr_shell_input_adapter.hpp"

#include <optional>
#include <utility>

#include <zephyr/device.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

namespace aegis::ports::zephyr {

namespace {

ZephyrShellInputAdapter* g_active_adapter = nullptr;
K_MSGQ_DEFINE(g_shell_action_queue, sizeof(shell::ShellNavigationAction), 16, 4);

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
                                                 ZephyrBoardRuntime& runtime,
                                                 ZephyrBoardBackendConfig config)
    : logger_(logger),
      runtime_(runtime),
      config_(std::move(config)),
      backend_(make_zephyr_shell_input_backend(logger_, runtime_, config_)) {}

bool ZephyrShellInputAdapter::initialize() {
    g_active_adapter = nullptr;
    callback_input_enabled_ = backend_ != nullptr ? backend_->callback_input_enabled() : true;
    const bool backend_ready = backend_ != nullptr ? backend_->initialize() : false;

    const auto* rotary = config_.rotary_device_name.empty()
                             ? nullptr
                             : device_get_binding(config_.rotary_device_name.c_str());
    const bool rotary_ready = rotary != nullptr && device_is_ready(rotary);
    const auto* keyboard = config_.keyboard_device_name.empty()
                               ? nullptr
                               : device_get_binding(config_.keyboard_device_name.c_str());
    const bool keyboard_ready = keyboard != nullptr && device_is_ready(keyboard);

    logger_.info("input",
                 "shell input adapter rotary=" + std::string(rotary_ready ? "ready" : "missing") +
                     " keyboard=" + std::string(keyboard_ready ? "ready" : "missing") +
                     " callback=" + std::string(callback_input_enabled_ ? "enabled" : "disabled") +
                     " backend-profile=" +
                     std::to_string(static_cast<int>(runtime_.shell_input_backend_profile())));
    return backend_ready || rotary_ready || keyboard_ready;
}

void ZephyrShellInputAdapter::enable_interactive_mode() {
    if (interactive_mode_enabled_) {
        return;
    }

    interactive_mode_enabled_ = true;
    g_active_adapter = this;
    if (backend_ != nullptr) {
        backend_->enable_interactive_mode();
    }
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

    shell::ShellNavigationAction action {};
    if (k_msgq_get(&g_shell_action_queue, &action, K_NO_WAIT) == 0) {
        logger_.info("input", "dispatch action " + std::string(shell::to_string(action)));
        return action;
    }

    if (backend_ != nullptr) {
        if (const auto backend_action = backend_->poll_action(); backend_action.has_value()) {
            logger_.info("input",
                         "dispatch backend action " + std::string(shell::to_string(*backend_action)));
            return backend_action;
        }
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

std::optional<shell::ShellNavigationAction> ZephyrShellInputAdapter::map_input_event(
    const input_event& event) {
    if (!callback_input_enabled_) {
        return std::nullopt;
    }

    if (event.type == INPUT_EV_ABS) {
        if (event.code == INPUT_ABS_X) {
            keyboard_event_col_ = event.value;
        } else if (event.code == INPUT_ABS_Y) {
            keyboard_event_row_ = event.value;
        }
        return std::nullopt;
    }

    if (event.type == INPUT_EV_REL && event.code == INPUT_REL_WHEEL) {
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

}  // namespace aegis::ports::zephyr
