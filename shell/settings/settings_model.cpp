#include "shell/settings/settings_model.hpp"

namespace aegis::shell {

void SettingsModel::set_entries(std::vector<SettingsEntry> entries) {
    entries_ = std::move(entries);
}

const std::vector<SettingsEntry>& SettingsModel::entries() const {
    return entries_;
}

}  // namespace aegis::shell
