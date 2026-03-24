#pragma once

#include <string>

namespace aegis::shell {

struct SettingsEntry {
    std::string id;
    std::string label;
    bool visible {true};
};

}  // namespace aegis::shell
