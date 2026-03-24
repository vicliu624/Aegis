#include "shell/launcher/launcher_model.hpp"

#include <algorithm>

namespace aegis::shell {

void LauncherModel::set_entries(std::vector<LauncherEntry> entries) {
    std::stable_sort(entries.begin(), entries.end(), [](const LauncherEntry& lhs, const LauncherEntry& rhs) {
        if (lhs.state != rhs.state) {
            return static_cast<int>(lhs.state) < static_cast<int>(rhs.state);
        }
        if (lhs.descriptor.manifest.order_hint != rhs.descriptor.manifest.order_hint) {
            return lhs.descriptor.manifest.order_hint < rhs.descriptor.manifest.order_hint;
        }
        if (lhs.descriptor.manifest.category != rhs.descriptor.manifest.category) {
            return lhs.descriptor.manifest.category < rhs.descriptor.manifest.category;
        }
        return lhs.descriptor.manifest.display_name < rhs.descriptor.manifest.display_name;
    });
    entries_ = std::move(entries);
    focus_index_ = 0;
    for (std::size_t index = 0; index < entries_.size(); ++index) {
        if (entries_[index].state == LauncherEntryState::Visible) {
            focus_index_ = index;
            break;
        }
    }
}

const std::vector<LauncherEntry>& LauncherModel::entries() const {
    return entries_;
}

std::vector<LauncherEntry> LauncherModel::visible_entries() const {
    std::vector<LauncherEntry> visible;
    for (const auto& entry : entries_) {
        if (entry.state == LauncherEntryState::Visible) {
            visible.push_back(entry);
        }
    }
    return visible;
}

const LauncherEntry* LauncherModel::focused_entry() const {
    if (entries_.empty() || focus_index_ >= entries_.size()) {
        return nullptr;
    }
    return &entries_[focus_index_];
}

std::size_t LauncherModel::focus_index() const {
    return focus_index_;
}

bool LauncherModel::focus_next() {
    if (!has_visible_entries()) {
        return false;
    }
    focus_index_ = find_next_visible_index(focus_index_, +1);
    return true;
}

bool LauncherModel::focus_previous() {
    if (!has_visible_entries()) {
        return false;
    }
    focus_index_ = find_next_visible_index(focus_index_, -1);
    return true;
}

bool LauncherModel::has_visible_entries() const {
    return std::any_of(entries_.begin(), entries_.end(), [](const LauncherEntry& entry) {
        return entry.state == LauncherEntryState::Visible;
    });
}

std::size_t LauncherModel::find_next_visible_index(std::size_t start_index, int direction) const {
    if (entries_.empty()) {
        return 0;
    }
    const auto count = static_cast<int>(entries_.size());
    auto index = static_cast<int>(start_index);
    for (int step = 0; step < count; ++step) {
        index = (index + direction + count) % count;
        if (entries_[static_cast<std::size_t>(index)].state == LauncherEntryState::Visible) {
            return static_cast<std::size_t>(index);
        }
    }
    return start_index;
}

}  // namespace aegis::shell
