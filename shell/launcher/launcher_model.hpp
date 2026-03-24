#pragma once

#include <string>
#include <vector>

#include "shell/launcher/launcher_entry.hpp"

namespace aegis::shell {

class LauncherModel {
public:
    void set_entries(std::vector<LauncherEntry> entries);
    [[nodiscard]] const std::vector<LauncherEntry>& entries() const;
    [[nodiscard]] std::vector<LauncherEntry> visible_entries() const;
    [[nodiscard]] const LauncherEntry* focused_entry() const;
    [[nodiscard]] std::size_t focus_index() const;
    bool focus_next();
    bool focus_previous();
    [[nodiscard]] bool has_visible_entries() const;

private:
    [[nodiscard]] std::size_t find_next_visible_index(std::size_t start_index, int direction) const;

    std::vector<LauncherEntry> entries_;
    std::size_t focus_index_ {0};
};

}  // namespace aegis::shell
