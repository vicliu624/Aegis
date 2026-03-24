#pragma once

#include <vector>

#include "shell/control/shell_input_model.hpp"
#include "shell/settings/settings_entry.hpp"
#include "shell/status/notification_entry.hpp"
#include "shell/status/status_item.hpp"

namespace aegis::device {

struct ShellSurfaceProfile {
    std::vector<shell::SettingsEntry> settings_entries;
    std::vector<shell::StatusItem> status_items;
    std::vector<shell::NotificationEntry> notification_entries;
    std::vector<shell::ShellNavigationAction> navigation_actions;
};

}  // namespace aegis::device
