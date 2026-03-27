#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "runtime/lifecycle/lifecycle.hpp"

namespace aegis::runtime {

enum class AppFaultKind {
    None,
    InvalidMemoryAccess,
    StackOverflow,
    HeapCorruption,
    InvalidSyscallPayload,
    InvalidHandleUse,
    PermissionViolation,
    QuotaExceeded,
    WatchdogTimeout,
    BinaryPolicyViolation,
    LoaderFailure,
    UnhandledException,
};

struct AppFaultRecord {
    AppFaultKind kind {AppFaultKind::None};
    std::string detail;
    AppLifecycleState lifecycle_state {AppLifecycleState::LoadRequested};
    bool fatal {true};
    std::uint32_t strike_cost {1};
};

[[nodiscard]] constexpr std::string_view to_string(AppFaultKind kind) {
    switch (kind) {
        case AppFaultKind::None:
            return "none";
        case AppFaultKind::InvalidMemoryAccess:
            return "invalid memory access";
        case AppFaultKind::StackOverflow:
            return "stack overflow";
        case AppFaultKind::HeapCorruption:
            return "heap corruption";
        case AppFaultKind::InvalidSyscallPayload:
            return "invalid syscall payload";
        case AppFaultKind::InvalidHandleUse:
            return "invalid handle use";
        case AppFaultKind::PermissionViolation:
            return "permission violation";
        case AppFaultKind::QuotaExceeded:
            return "quota exceeded";
        case AppFaultKind::WatchdogTimeout:
            return "watchdog timeout";
        case AppFaultKind::BinaryPolicyViolation:
            return "binary policy violation";
        case AppFaultKind::LoaderFailure:
            return "loader failure";
        case AppFaultKind::UnhandledException:
            return "unhandled exception";
    }

    return "unknown";
}

}  // namespace aegis::runtime
