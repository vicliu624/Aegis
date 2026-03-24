#include "core/app_session/app_session.hpp"

#include <stdexcept>

namespace aegis::core {

namespace {

bool is_legal_transition(runtime::AppLifecycleState from, runtime::AppLifecycleState to) {
    using State = runtime::AppLifecycleState;
    switch (from) {
        case State::Discovered:
            return to == State::Validated || to == State::ReturnedToShell;
        case State::Validated:
            return to == State::LoadRequested || to == State::ReturnedToShell;
        case State::LoadRequested:
            return to == State::Loaded || to == State::ReturnedToShell;
        case State::Loaded:
            return to == State::Started || to == State::Stopping;
        case State::Started:
            return to == State::Running || to == State::Stopping;
        case State::Running:
            return to == State::Stopping;
        case State::Stopping:
            return to == State::TornDown;
        case State::TornDown:
            return to == State::Unloaded;
        case State::Unloaded:
            return to == State::ReturnedToShell;
        case State::ReturnedToShell:
            return false;
    }

    return false;
}

}  // namespace

AppSession::AppSession(const AppDescriptor& descriptor)
    : id_(descriptor.manifest.app_id + ":session"),
      descriptor_(descriptor),
      state_(descriptor.validated ? runtime::AppLifecycleState::Validated
                                  : runtime::AppLifecycleState::Discovered),
      history_ {state_} {}

const std::string& AppSession::id() const {
    return id_;
}

const AppDescriptor& AppSession::descriptor() const {
    return descriptor_;
}

runtime::AppLifecycleState AppSession::state() const {
    return state_;
}

const std::vector<runtime::AppLifecycleState>& AppSession::history() const {
    return history_;
}

void AppSession::transition_to(runtime::AppLifecycleState state) {
    if (state == state_) {
        return;
    }
    if (!is_legal_transition(state_, state)) {
        throw std::runtime_error("illegal lifecycle transition from " +
                                 std::string(runtime::to_string(state_)) + " to " +
                                 std::string(runtime::to_string(state)));
    }
    state_ = state;
    history_.push_back(state_);
}

void AppSession::recover_to_shell() {
    using State = runtime::AppLifecycleState;
    switch (state_) {
        case State::Discovered:
        case State::Validated:
        case State::LoadRequested:
            transition_to(State::ReturnedToShell);
            return;
        case State::Loaded:
        case State::Started:
        case State::Running:
            transition_to(State::Stopping);
            [[fallthrough]];
        case State::Stopping:
            transition_to(State::TornDown);
            [[fallthrough]];
        case State::TornDown:
            transition_to(State::Unloaded);
            [[fallthrough]];
        case State::Unloaded:
            transition_to(State::ReturnedToShell);
            return;
        case State::ReturnedToShell:
            return;
    }
}

}  // namespace aegis::core
