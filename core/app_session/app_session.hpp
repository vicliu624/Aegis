#pragma once

#include <string>

#include "core/app_registry/app_manifest.hpp"
#include "runtime/lifecycle/lifecycle.hpp"

namespace aegis::core {

class AppSession {
public:
    explicit AppSession(const AppDescriptor& descriptor);

    [[nodiscard]] const std::string& id() const;
    [[nodiscard]] const AppDescriptor& descriptor() const;
    [[nodiscard]] runtime::AppLifecycleState state() const;
    void transition_to(runtime::AppLifecycleState state);

private:
    std::string id_;
    const AppDescriptor& descriptor_;
    runtime::AppLifecycleState state_ {runtime::AppLifecycleState::LoadRequested};
};

}  // namespace aegis::core
