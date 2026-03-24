#include "core/app_session/app_session.hpp"

namespace aegis::core {

AppSession::AppSession(const AppDescriptor& descriptor)
    : id_(descriptor.manifest.app_id + ":session"), descriptor_(descriptor) {}

const std::string& AppSession::id() const {
    return id_;
}

const AppDescriptor& AppSession::descriptor() const {
    return descriptor_;
}

runtime::AppLifecycleState AppSession::state() const {
    return state_;
}

void AppSession::transition_to(runtime::AppLifecycleState state) {
    state_ = state;
}

}  // namespace aegis::core
