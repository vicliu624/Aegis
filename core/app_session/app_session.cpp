#include "core/app_session/app_session.hpp"

namespace aegis::core {

AppSession::AppSession(const AppDescriptor& descriptor)
    : instance_(descriptor) {}

const std::string& AppSession::id() const {
    return instance_.instance_id();
}

const AppDescriptor& AppSession::descriptor() const {
    return instance_.descriptor();
}

runtime::AppLifecycleState AppSession::state() const {
    return instance_.state();
}

const std::vector<runtime::AppLifecycleState>& AppSession::history() const {
    return instance_.history();
}

runtime::AppInstance& AppSession::instance() {
    return instance_;
}

const runtime::AppInstance& AppSession::instance() const {
    return instance_;
}

void AppSession::transition_to(runtime::AppLifecycleState state) {
    instance_.transition_to(state);
}

void AppSession::recover_to_shell() {
    using State = runtime::AppLifecycleState;
    switch (state()) {
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
