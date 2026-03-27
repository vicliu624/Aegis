#include "runtime/handles/app_handle_table.hpp"

#include <utility>

namespace aegis::runtime {

std::uint32_t AppHandleTable::register_handle(AppHandleKind kind,
                                              std::string label,
                                              std::function<void()> release_hook) {
    const auto handle_id = next_handle_id_++;
    handles_.emplace(handle_id,
                     AppHandleRecord {
                         .handle_id = handle_id,
                         .kind = kind,
                         .label = std::move(label),
                         .release_hook = std::move(release_hook),
                     });
    return handle_id;
}

std::optional<AppHandleRecord> AppHandleTable::find(std::uint32_t handle_id) const {
    const auto it = handles_.find(handle_id);
    if (it == handles_.end()) {
        return std::nullopt;
    }
    return it->second;
}

bool AppHandleTable::release(std::uint32_t handle_id) {
    const auto it = handles_.find(handle_id);
    if (it == handles_.end()) {
        return false;
    }

    auto record = std::move(it->second);
    handles_.erase(it);
    if (record.release_hook) {
        record.release_hook();
    }
    return true;
}

bool AppHandleTable::release_by_label(AppHandleKind kind, const std::string& label) {
    for (const auto& [handle_id, record] : handles_) {
        if (record.kind == kind && record.label == label) {
            return release(handle_id);
        }
    }
    return false;
}

std::vector<AppHandleRecord> AppHandleTable::snapshot() const {
    std::vector<AppHandleRecord> result;
    result.reserve(handles_.size());
    for (const auto& [_, record] : handles_) {
        result.push_back(record);
    }
    return result;
}

}  // namespace aegis::runtime
