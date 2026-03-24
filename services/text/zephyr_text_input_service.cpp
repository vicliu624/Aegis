#include "services/text/zephyr_text_input_service.hpp"

#include <algorithm>
#include <cctype>
#include <string>
#include <utility>

#include <zephyr/input/input.h>

namespace aegis::services {

namespace {

ZephyrTextInputService* g_active_text_input_service = nullptr;

bool is_shift_key(uint16_t key_code) {
    return key_code == INPUT_KEY_LEFTSHIFT || key_code == INPUT_KEY_RIGHTSHIFT;
}

void zephyr_text_input_callback(input_event* event, void* user_data) {
    ARG_UNUSED(user_data);
    if (g_active_text_input_service == nullptr || event == nullptr || event->dev == nullptr) {
        return;
    }

    if (event->type != INPUT_EV_KEY) {
        return;
    }

    g_active_text_input_service->handle_key_event(event->dev->name, event->code, event->value != 0);
}

INPUT_CALLBACK_DEFINE(NULL, zephyr_text_input_callback, NULL);

}  // namespace

ZephyrTextInputService::ZephyrTextInputService(ports::zephyr::ZephyrBoardBackendConfig config)
    : config_(std::move(config)) {
    k_mutex_init(&mutex_);
    g_active_text_input_service = this;
    state_.available = config_.keyboard_input;
    state_.text_entry = config_.keyboard_input;
    state_.source_name = config_.text_input_device_name.empty() ? config_.keyboard_device_name
                                                                : config_.text_input_device_name;
    focus_.available = state_.available;
    focus_.text_entry = state_.text_entry;
    focus_.route_name = state_.available ? "inactive" : "unavailable";
}

TextInputState ZephyrTextInputService::state() const {
    k_mutex_lock(&mutex_, K_FOREVER);
    const auto snapshot = state_;
    k_mutex_unlock(&mutex_);
    return snapshot;
}

TextInputFocusState ZephyrTextInputService::focus_state() const {
    k_mutex_lock(&mutex_, K_FOREVER);
    auto snapshot = focus_;
    snapshot.available = state_.available;
    snapshot.text_entry = state_.text_entry;
    k_mutex_unlock(&mutex_);
    return snapshot;
}

bool ZephyrTextInputService::request_focus_for_session(const std::string& session_id) {
    if (session_id.empty()) {
        return false;
    }

    k_mutex_lock(&mutex_, K_FOREVER);
    if (!state_.available || !state_.text_entry) {
        k_mutex_unlock(&mutex_);
        return false;
    }

    focus_.available = state_.available;
    focus_.focused = true;
    focus_.text_entry = state_.text_entry;
    focus_.owner = TextInputFocusOwner::AppSession;
    focus_.owner_session_id = session_id;
    focus_.route_name = "app_foreground";
    k_mutex_unlock(&mutex_);
    return true;
}

bool ZephyrTextInputService::release_focus_for_session(const std::string& session_id) {
    k_mutex_lock(&mutex_, K_FOREVER);
    if (focus_.owner != TextInputFocusOwner::AppSession || focus_.owner_session_id != session_id) {
        k_mutex_unlock(&mutex_);
        return false;
    }

    focus_.focused = false;
    focus_.owner = TextInputFocusOwner::None;
    focus_.owner_session_id.clear();
    focus_.route_name = state_.available ? "inactive" : "unavailable";
    k_mutex_unlock(&mutex_);
    return true;
}

void ZephyrTextInputService::assign_shell_focus() {
    k_mutex_lock(&mutex_, K_FOREVER);
    focus_.available = state_.available;
    focus_.text_entry = state_.text_entry;
    focus_.owner_session_id.clear();
    if (!state_.available || !state_.text_entry) {
        focus_.focused = false;
        focus_.owner = TextInputFocusOwner::None;
        focus_.route_name = "unavailable";
        k_mutex_unlock(&mutex_);
        return;
    }

    focus_.focused = true;
    focus_.owner = TextInputFocusOwner::Shell;
    focus_.route_name = "shell_foreground";
    k_mutex_unlock(&mutex_);
}

std::string ZephyrTextInputService::describe_backend() const {
    return state().source_name;
}

void ZephyrTextInputService::handle_key_event(const std::string& source_name,
                                              uint16_t key_code,
                                              bool pressed) {
    if (!matches_source(source_name)) {
        return;
    }

    k_mutex_lock(&mutex_, K_FOREVER);
    state_.available = true;
    state_.text_entry = true;
    state_.last_key_code = key_code;
    update_modifier(key_code, pressed);
    update_text(key_code, pressed);
    state_.modifier_mask = 0;
    if (shift_pressed_) {
        state_.modifier_mask |= static_cast<uint32_t>(TextInputModifier::Shift);
    }
    if (caps_lock_) {
        state_.modifier_mask |= static_cast<uint32_t>(TextInputModifier::CapsLock);
    }
    if (symbol_pressed_) {
        state_.modifier_mask |= static_cast<uint32_t>(TextInputModifier::Symbol);
    }
    if (alt_pressed_) {
        state_.modifier_mask |= static_cast<uint32_t>(TextInputModifier::Alt);
    }
    k_mutex_unlock(&mutex_);
}

bool ZephyrTextInputService::matches_source(const std::string& source_name) const {
    if (source_name.empty()) {
        return false;
    }

    return source_name == config_.text_input_device_name || source_name == config_.keyboard_device_name;
}

void ZephyrTextInputService::update_modifier(uint16_t key_code, bool pressed) {
    if (is_shift_key(key_code)) {
        shift_pressed_ = pressed;
        return;
    }

    if (key_code == INPUT_KEY_LEFTALT || key_code == INPUT_KEY_RIGHTALT) {
        alt_pressed_ = pressed;
        symbol_pressed_ = pressed;
        return;
    }

    if (key_code == INPUT_KEY_CAPSLOCK && pressed) {
        caps_lock_ = !caps_lock_;
    }
}

void ZephyrTextInputService::update_text(uint16_t key_code, bool pressed) {
    if (!pressed) {
        return;
    }

    if (key_code == INPUT_KEY_ENTER) {
        state_.last_text = "\n";
        return;
    }
    if (key_code == INPUT_KEY_SPACE) {
        state_.last_text = " ";
        return;
    }
    if (key_code == INPUT_KEY_BACKSPACE) {
        state_.last_text = "\b";
        return;
    }

    char value = symbol_pressed_ ? translate_symbol(key_code) : translate_plain(key_code);
    if (value == '\0') {
        return;
    }

    if (!symbol_pressed_ && std::isalpha(static_cast<unsigned char>(value)) != 0) {
        const bool uppercase = shift_pressed_ != caps_lock_;
        value = static_cast<char>(uppercase ? std::toupper(static_cast<unsigned char>(value))
                                            : std::tolower(static_cast<unsigned char>(value)));
    }

    state_.last_text.assign(1, value);
}

char ZephyrTextInputService::translate_plain(uint16_t key_code) const {
    switch (key_code) {
        case INPUT_KEY_A: return 'a';
        case INPUT_KEY_B: return 'b';
        case INPUT_KEY_C: return 'c';
        case INPUT_KEY_D: return 'd';
        case INPUT_KEY_E: return 'e';
        case INPUT_KEY_F: return 'f';
        case INPUT_KEY_G: return 'g';
        case INPUT_KEY_H: return 'h';
        case INPUT_KEY_I: return 'i';
        case INPUT_KEY_J: return 'j';
        case INPUT_KEY_K: return 'k';
        case INPUT_KEY_L: return 'l';
        case INPUT_KEY_M: return 'm';
        case INPUT_KEY_N: return 'n';
        case INPUT_KEY_O: return 'o';
        case INPUT_KEY_P: return 'p';
        case INPUT_KEY_Q: return 'q';
        case INPUT_KEY_R: return 'r';
        case INPUT_KEY_S: return 's';
        case INPUT_KEY_T: return 't';
        case INPUT_KEY_U: return 'u';
        case INPUT_KEY_V: return 'v';
        case INPUT_KEY_W: return 'w';
        case INPUT_KEY_X: return 'x';
        case INPUT_KEY_Y: return 'y';
        case INPUT_KEY_Z: return 'z';
        default: return '\0';
    }
}

char ZephyrTextInputService::translate_symbol(uint16_t key_code) const {
    switch (key_code) {
        case INPUT_KEY_Q: return '1';
        case INPUT_KEY_W: return '2';
        case INPUT_KEY_E: return '3';
        case INPUT_KEY_R: return '4';
        case INPUT_KEY_T: return '5';
        case INPUT_KEY_Y: return '6';
        case INPUT_KEY_U: return '7';
        case INPUT_KEY_I: return '8';
        case INPUT_KEY_O: return '9';
        case INPUT_KEY_P: return '0';
        case INPUT_KEY_A: return '*';
        case INPUT_KEY_S: return '/';
        case INPUT_KEY_D: return '+';
        case INPUT_KEY_F: return '-';
        case INPUT_KEY_G: return '=';
        case INPUT_KEY_H: return ':';
        case INPUT_KEY_J: return '\'';
        case INPUT_KEY_K: return '"';
        case INPUT_KEY_L: return '@';
        case INPUT_KEY_X: return '_';
        case INPUT_KEY_C: return '$';
        case INPUT_KEY_V: return ';';
        case INPUT_KEY_B: return '?';
        case INPUT_KEY_N: return '!';
        case INPUT_KEY_M: return '.';
        case INPUT_KEY_Z: return '\0';
        default: return '\0';
    }
}

}  // namespace aegis::services
