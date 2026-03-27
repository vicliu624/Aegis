#include "apps/builtin/settings/settings_app_model.hpp"

#include <utility>

namespace aegis::shell {

void SettingsAppModel::set_entries(std::vector<SettingsAppEntry> entries) {
    entries_ = std::move(entries);
    focus_index_ = 0;
    for (std::size_t index = 0; index < entries_.size(); ++index) {
        auto& entry = entries_[index];
        entry.committed_index = normalize_index(entry, entry.committed_index);
        entry.draft_index = normalize_index(entry, entry.draft_index);
        if (entry.visible) {
            focus_index_ = index;
            break;
        }
    }
}

void SettingsAppModel::load(const std::shared_ptr<services::ISettingsService>& settings_service) {
    for (auto& entry : entries_) {
        entry.committed_index = normalize_index(entry, entry.committed_index);
        entry.draft_index = entry.committed_index;
        if (settings_service == nullptr) {
            continue;
        }
        if (const auto stored = settings_service->find(settings_key_for(entry.id)); stored.has_value()) {
            for (std::size_t index = 0; index < entry.options.size(); ++index) {
                if (entry.options[index] == *stored) {
                    entry.committed_index = index;
                    entry.draft_index = index;
                    break;
                }
            }
        }
    }
}

bool SettingsAppModel::focus_next(bool wrap_navigation) {
    if (entries_.empty()) {
        return false;
    }

    for (std::size_t step = 1; step <= entries_.size(); ++step) {
        std::size_t next = focus_index_ + step;
        if (wrap_navigation) {
            next %= entries_.size();
        } else if (next >= entries_.size()) {
            next = entries_.size() - 1;
        }
        if (entries_[next].visible) {
            const bool changed = next != focus_index_;
            focus_index_ = next;
            return changed;
        }
        if (!wrap_navigation && next == entries_.size() - 1) {
            break;
        }
    }
    return false;
}

bool SettingsAppModel::focus_previous(bool wrap_navigation) {
    if (entries_.empty()) {
        return false;
    }

    for (std::size_t step = 1; step <= entries_.size(); ++step) {
        std::size_t next = 0;
        if (wrap_navigation) {
            next = (focus_index_ + entries_.size() - (step % entries_.size())) % entries_.size();
        } else {
            next = (step > focus_index_) ? 0 : (focus_index_ - step);
        }
        if (entries_[next].visible) {
            const bool changed = next != focus_index_;
            focus_index_ = next;
            return changed;
        }
        if (!wrap_navigation && next == 0) {
            break;
        }
    }
    return false;
}

bool SettingsAppModel::cycle_focused_option() {
    if (entries_.empty() || focus_index_ >= entries_.size()) {
        return false;
    }

    auto& entry = entries_[focus_index_];
    if (entry.options.empty()) {
        return false;
    }
    entry.draft_index = (entry.draft_index + 1) % entry.options.size();
    return true;
}

bool SettingsAppModel::apply(const std::shared_ptr<services::ISettingsService>& settings_service) {
    bool changed = false;
    for (auto& entry : entries_) {
        if (entry.draft_index == entry.committed_index) {
            continue;
        }
        entry.committed_index = normalize_index(entry, entry.draft_index);
        if (settings_service != nullptr && !entry.options.empty()) {
            settings_service->set(settings_key_for(entry.id), entry.options[entry.committed_index]);
        }
        changed = true;
    }
    return changed;
}

std::size_t SettingsAppModel::dirty_count() const {
    std::size_t count = 0;
    for (const auto& entry : entries_) {
        if (entry.draft_index != entry.committed_index) {
            ++count;
        }
    }
    return count;
}

bool SettingsAppModel::has_dirty_entries() const {
    return dirty_count() != 0;
}

const std::vector<SettingsAppEntry>& SettingsAppModel::entries() const {
    return entries_;
}

std::size_t SettingsAppModel::focus_index() const {
    return focus_index_;
}

const SettingsAppEntry* SettingsAppModel::focused_entry() const {
    if (entries_.empty() || focus_index_ >= entries_.size()) {
        return nullptr;
    }
    return &entries_[focus_index_];
}

const SettingsAppEntry* SettingsAppModel::find_entry(const std::string& id) const {
    for (const auto& entry : entries_) {
        if (entry.id == id) {
            return &entry;
        }
    }
    return nullptr;
}

std::string SettingsAppModel::value_for(const SettingsAppEntry& entry) const {
    if (entry.options.empty()) {
        return {};
    }
    const auto index = normalize_index(entry, entry.draft_index);
    return entry.options[index];
}

runtime::ForegroundPagePresentation SettingsAppModel::page_presentation() const {
    runtime::ForegroundPagePresentation page;
    page.items.reserve(std::min<std::size_t>(entries_.size(), 8U));
    page.page_id = "builtin.settings";
    page.page_state_token =
        "settings:" + std::to_string(focus_index_) + ":" + std::to_string(dirty_count());
    page.title = "Settings";
    page.context = {};
    page.layout_template = runtime::ForegroundPageTemplate::ValueList;
    page.softkeys[0] = {
        .label = "Apply",
        .command_id = "apply",
        .role = AEGIS_UI_SOFTKEY_ROLE_PRIMARY,
        .visible = true,
        .enabled = has_dirty_entries(),
    };
    page.softkeys[1] = {
        .label = "Select",
        .command_id = "select",
        .role = AEGIS_UI_SOFTKEY_ROLE_CONFIRM,
        .visible = true,
        .enabled = focused_entry() != nullptr,
    };

    for (std::size_t index = 0; index < entries_.size() && page.items.size() < 8; ++index) {
        const auto& entry = entries_[index];
        if (!entry.visible) {
            continue;
        }
        page.items.push_back({
            .item_id = entry.id,
            .label = entry.label,
            .detail = value_for(entry),
            .focused = index == focus_index_,
            .emphasized = index == focus_index_,
            .warning = entry.draft_index != entry.committed_index,
            .disabled = false,
            .accessory = runtime::ForegroundPageItemAccessory::None,
        });
    }

    return page;
}

bool SettingsAppModel::handle_routed_action(
    ShellNavigationAction action,
    const std::shared_ptr<services::ISettingsService>& settings_service) {
    switch (action) {
        case ShellNavigationAction::MoveNext:
            return focus_next(true);
        case ShellNavigationAction::MovePrevious:
            return focus_previous(true);
        case ShellNavigationAction::PrimaryAction:
            return apply(settings_service);
        case ShellNavigationAction::Select:
            return cycle_focused_option();
        case ShellNavigationAction::Back:
        case ShellNavigationAction::OpenMenu:
        case ShellNavigationAction::OpenSettings:
        case ShellNavigationAction::OpenNotifications:
            return false;
    }

    return false;
}

bool SettingsAppModel::handle_page_command(
    std::string_view command_id,
    const std::shared_ptr<services::ISettingsService>& settings_service) {
    if (command_id == "apply") {
        return apply(settings_service);
    }
    if (command_id == "select") {
        return cycle_focused_option();
    }
    return false;
}

std::string SettingsAppModel::settings_key_for(const std::string& id) {
    return "builtin.settings." + id;
}

std::size_t SettingsAppModel::normalize_index(const SettingsAppEntry& entry, std::size_t index) const {
    if (entry.options.empty()) {
        return 0;
    }
    if (index >= entry.options.size()) {
        return 0;
    }
    return index;
}

}  // namespace aegis::shell
