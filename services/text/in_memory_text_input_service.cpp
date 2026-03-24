#include "services/text/in_memory_text_input_service.hpp"

#include <utility>

namespace aegis::services {

InMemoryTextInputService::InMemoryTextInputService(TextInputState state)
    : state_(std::move(state)),
      focus_ {.available = state_.available,
              .focused = false,
              .text_entry = state_.text_entry,
              .owner = TextInputFocusOwner::None,
              .owner_session_id = "",
              .route_name = state_.available ? "inactive" : "unavailable"} {}

TextInputState InMemoryTextInputService::state() const {
    return state_;
}

TextInputFocusState InMemoryTextInputService::focus_state() const {
    return focus_;
}

bool InMemoryTextInputService::request_focus_for_session(const std::string& session_id) {
    if (!state_.available || !state_.text_entry || session_id.empty()) {
        return false;
    }

    focus_.available = state_.available;
    focus_.focused = true;
    focus_.text_entry = state_.text_entry;
    focus_.owner = TextInputFocusOwner::AppSession;
    focus_.owner_session_id = session_id;
    focus_.route_name = "app_foreground";
    return true;
}

bool InMemoryTextInputService::release_focus_for_session(const std::string& session_id) {
    if (focus_.owner != TextInputFocusOwner::AppSession || focus_.owner_session_id != session_id) {
        return false;
    }

    focus_.focused = false;
    focus_.owner = TextInputFocusOwner::None;
    focus_.owner_session_id.clear();
    focus_.route_name = state_.available ? "inactive" : "unavailable";
    return true;
}

void InMemoryTextInputService::assign_shell_focus() {
    focus_.available = state_.available;
    focus_.text_entry = state_.text_entry;
    focus_.owner_session_id.clear();
    if (!state_.available || !state_.text_entry) {
        focus_.focused = false;
        focus_.owner = TextInputFocusOwner::None;
        focus_.route_name = "unavailable";
        return;
    }

    focus_.focused = true;
    focus_.owner = TextInputFocusOwner::Shell;
    focus_.route_name = "shell_foreground";
}

std::string InMemoryTextInputService::describe_backend() const {
    return state_.source_name;
}

}  // namespace aegis::services
