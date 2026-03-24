#pragma once

#include <string>
#include <vector>

#include "core/app_registry/app_manifest.hpp"

namespace aegis::shell {

enum class LauncherEntryState {
    Visible,
    Hidden,
    Incompatible,
    Invalid,
};

struct LauncherEntry {
    core::AppDescriptor descriptor;
    LauncherEntryState state {LauncherEntryState::Visible};
    std::string reason;
    std::vector<std::string> compatibility_notes;
};

}  // namespace aegis::shell
