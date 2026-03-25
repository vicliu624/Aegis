#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "services/common/service_interfaces.hpp"

namespace aegis::shell {

struct SettingsAppEntry {
    std::string id;
    std::string label;
    std::vector<std::string> options;
    std::size_t committed_index {0};
    std::size_t draft_index {0};
    bool visible {true};
};

class SettingsAppModel {
public:
    void set_entries(std::vector<SettingsAppEntry> entries);
    void load(const std::shared_ptr<services::ISettingsService>& settings_service);
    [[nodiscard]] bool focus_next(bool wrap_navigation);
    [[nodiscard]] bool focus_previous(bool wrap_navigation);
    [[nodiscard]] bool cycle_focused_option();
    [[nodiscard]] bool apply(const std::shared_ptr<services::ISettingsService>& settings_service);
    [[nodiscard]] std::size_t dirty_count() const;
    [[nodiscard]] bool has_dirty_entries() const;
    [[nodiscard]] const std::vector<SettingsAppEntry>& entries() const;
    [[nodiscard]] std::size_t focus_index() const;
    [[nodiscard]] const SettingsAppEntry* focused_entry() const;
    [[nodiscard]] const SettingsAppEntry* find_entry(const std::string& id) const;
    [[nodiscard]] std::string value_for(const SettingsAppEntry& entry) const;

private:
    [[nodiscard]] static std::string settings_key_for(const std::string& id);
    [[nodiscard]] std::size_t normalize_index(const SettingsAppEntry& entry, std::size_t index) const;

    std::vector<SettingsAppEntry> entries_;
    std::size_t focus_index_ {0};
};

}  // namespace aegis::shell
