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

[[nodiscard]] constexpr bool is_terminal_state(AppLifecycleState state) {
    return state == AppLifecycleState::ReturnedToShell;
}

[[nodiscard]] constexpr bool is_legal_transition(AppLifecycleState from, AppLifecycleState to) {
    switch (from) {
        case AppLifecycleState::Discovered:
            return to == AppLifecycleState::Validated || to == AppLifecycleState::ReturnedToShell;
        case AppLifecycleState::Validated:
            return to == AppLifecycleState::LoadRequested || to == AppLifecycleState::ReturnedToShell;
        case AppLifecycleState::LoadRequested:
            return to == AppLifecycleState::Loaded || to == AppLifecycleState::ReturnedToShell;
        case AppLifecycleState::Loaded:
            return to == AppLifecycleState::Started || to == AppLifecycleState::Stopping;
        case AppLifecycleState::Started:
            return to == AppLifecycleState::Running || to == AppLifecycleState::Stopping;
        case AppLifecycleState::Running:
            return to == AppLifecycleState::Stopping;
        case AppLifecycleState::Stopping:
            return to == AppLifecycleState::TornDown;
        case AppLifecycleState::TornDown:
            return to == AppLifecycleState::Unloaded;
        case AppLifecycleState::Unloaded:
            return to == AppLifecycleState::ReturnedToShell;
        case AppLifecycleState::ReturnedToShell:
            return false;
    }

    return false;
}

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
