#pragma once

#include <string>
#include <vector>

#include "shell/settings/settings_entry.hpp"

namespace aegis::shell {

class SettingsModel {
public:
    void set_entries(std::vector<SettingsEntry> entries);
    [[nodiscard]] const std::vector<SettingsEntry>& entries() const;

private:
    std::vector<SettingsEntry> entries_;
};

}  // namespace aegis::shell
