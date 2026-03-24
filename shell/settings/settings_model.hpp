#pragma once

#include <string>
#include <vector>

namespace aegis::shell {

class SettingsModel {
public:
    void set_visible_pages(std::vector<std::string> pages);
    [[nodiscard]] const std::vector<std::string>& visible_pages() const;

private:
    std::vector<std::string> visible_pages_;
};

}  // namespace aegis::shell
