#include "shell/settings/settings_model.hpp"

namespace aegis::shell {

void SettingsModel::set_visible_pages(std::vector<std::string> pages) {
    visible_pages_ = std::move(pages);
}

const std::vector<std::string>& SettingsModel::visible_pages() const {
    return visible_pages_;
}

}  // namespace aegis::shell
