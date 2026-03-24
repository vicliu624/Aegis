#pragma once

#include <string_view>

namespace aegis::runtime {

enum class AppLifecycleState {
    Discovered,
    Validated,
    LoadRequested,
    Loaded,
    Started,
    Running,
    Stopping,
    TornDown,
    Unloaded,
    ReturnedToShell,
};

[[nodiscard]] constexpr std::string_view to_string(AppLifecycleState state) {
    switch (state) {
        case AppLifecycleState::Discovered:
            return "discovered";
        case AppLifecycleState::Validated:
            return "validated";
        case AppLifecycleState::LoadRequested:
            return "load requested";
        case AppLifecycleState::Loaded:
            return "loaded";
        case AppLifecycleState::Started:
            return "started";
        case AppLifecycleState::Running:
            return "running";
        case AppLifecycleState::Stopping:
            return "stopping";
        case AppLifecycleState::TornDown:
            return "torn down";
        case AppLifecycleState::Unloaded:
            return "unloaded";
        case AppLifecycleState::ReturnedToShell:
            return "returned to shell";
    }

    return "unknown";
}

}  // namespace aegis::runtime
