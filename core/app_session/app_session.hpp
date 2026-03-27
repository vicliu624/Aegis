#pragma once

#include <string>
#include <vector>

#include "core/app_registry/app_manifest.hpp"
#include "runtime/lifecycle/lifecycle.hpp"
#include "runtime/supervisor/app_instance.hpp"

namespace aegis::core {

class AppSession {
public:
    explicit AppSession(const AppDescriptor& descriptor);

    [[nodiscard]] const std::string& id() const;
    [[nodiscard]] const AppDescriptor& descriptor() const;
    [[nodiscard]] runtime::AppLifecycleState state() const;
    [[nodiscard]] const std::vector<runtime::AppLifecycleState>& history() const;
    [[nodiscard]] runtime::AppInstance& instance();
    [[nodiscard]] const runtime::AppInstance& instance() const;
    void transition_to(runtime::AppLifecycleState state);
    void recover_to_shell();

private:
    runtime::AppInstance instance_;
};

}  // namespace aegis::core
