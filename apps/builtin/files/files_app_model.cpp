#include "apps/builtin/files/files_app_model.hpp"

#include <algorithm>

namespace aegis::shell {

void FilesAppModel::load_root(const std::shared_ptr<services::IStorageService>& storage_service) {
    entries_.clear();
    focus_index_ = 0;
    status_text_override_.clear();
    if (storage_service == nullptr) {
        state_ = State::Unavailable;
        mount_root_ = "/lfs";
        current_path_ = "/";
        status_text_override_ = "Storage unavailable";
        return;
    }
    mount_root_ = storage_service->mount_root();
    load_virtual_root(storage_service);
}

bool FilesAppModel::focus_next(bool wrap_navigation) {
    if (entries_.empty()) {
        return false;
    }
    if (wrap_navigation) {
        focus_index_ = (focus_index_ + 1) % entries_.size();
        return true;
    }
    if ((focus_index_ + 1) >= entries_.size()) {
        return false;
    }
    ++focus_index_;
    return true;
}

bool FilesAppModel::focus_previous(bool wrap_navigation) {
    if (entries_.empty()) {
        return false;
    }
    if (wrap_navigation) {
        focus_index_ = (focus_index_ + entries_.size() - 1) % entries_.size();
        return true;
    }
    if (focus_index_ == 0) {
        return false;
    }
    --focus_index_;
    return true;
}

bool FilesAppModel::open_focused(const std::shared_ptr<services::IStorageService>& storage_service) {
    if ((state_ != State::Ready && state_ != State::Root) || entries_.empty() ||
        focus_index_ >= entries_.size()) {
        return false;
    }
    const auto& entry = entries_[focus_index_];
    if (!entry.directory) {
        return false;
    }
    if (is_virtual_root(current_path_)) {
        if (entry.name == "lfs") {
            load_path(storage_service, mount_root_);
            return true;
        }
        if (entry.name == "sdcard") {
            load_path(storage_service, "/sdcard");
            return true;
        }
        return false;
    }
    load_path(storage_service, join_path(current_path_, entry.name));
    return true;
}

bool FilesAppModel::go_parent(const std::shared_ptr<services::IStorageService>& storage_service) {
    if (is_virtual_root(current_path_) || current_path_.empty()) {
        return false;
    }
    if (current_path_ == mount_root_ || current_path_ == "/sdcard") {
        load_virtual_root(storage_service);
        return true;
    }
    load_path(storage_service, parent_path_for(current_path_));
    return true;
}

FilesAppModel::State FilesAppModel::state() const {
    return state_;
}

const std::string& FilesAppModel::current_path() const {
    return current_path_;
}

const std::vector<FilesAppEntry>& FilesAppModel::entries() const {
    return entries_;
}

std::size_t FilesAppModel::focus_index() const {
    return focus_index_;
}

const FilesAppEntry* FilesAppModel::focused_entry() const {
    if (entries_.empty() || focus_index_ >= entries_.size()) {
        return nullptr;
    }
    return &entries_[focus_index_];
}

std::string FilesAppModel::status_text() const {
    if (!status_text_override_.empty()) {
        return status_text_override_;
    }
    switch (state_) {
        case State::Root:
            return "/";
        case State::NoCard:
            return "No SD card inserted";
        case State::Unavailable:
            return "Storage unavailable";
        case State::Ready:
            if (entries_.empty()) {
                return "Folder is empty";
            }
            return current_path_;
    }
    return {};
}

void FilesAppModel::load_path(const std::shared_ptr<services::IStorageService>& storage_service,
                              std::string path) {
    current_path_ = std::move(path);
    entries_.clear();
    focus_index_ = 0;
    status_text_override_.clear();
    if (storage_service == nullptr) {
        state_ = State::Unavailable;
        status_text_override_ = "Storage unavailable";
        return;
    }

    if (current_path_ == mount_root_) {
        if (!storage_service->available()) {
            state_ = State::Unavailable;
            status_text_override_ = "LittleFS unavailable";
            return;
        }
    } else if (current_path_ == "/sdcard") {
        if (!storage_service->sd_card_present()) {
            state_ = State::NoCard;
            status_text_override_ = "No SD card inserted";
            return;
        }
        if (!storage_service->directory_exists(current_path_)) {
            state_ = State::Unavailable;
            status_text_override_ = "SD card filesystem unavailable";
            return;
        }
    } else if (!storage_service->directory_exists(current_path_)) {
        state_ = State::Unavailable;
        status_text_override_ = "Folder unavailable";
        return;
    }

    if (current_path_.rfind("/sdcard", 0) == 0 && !storage_service->sd_card_present()) {
        state_ = State::NoCard;
        status_text_override_ = "No SD card inserted";
        return;
    }

    const auto listed = storage_service->list_directory(current_path_);
    entries_.reserve(listed.size());
    for (const auto& entry : listed) {
        entries_.push_back(FilesAppEntry {
            .name = entry.name,
            .directory = entry.directory,
            .size_bytes = entry.size_bytes,
        });
    }
    state_ = State::Ready;
}

void FilesAppModel::load_virtual_root(const std::shared_ptr<services::IStorageService>& storage_service) {
    current_path_ = "/";
    entries_.clear();
    focus_index_ = 0;
    status_text_override_.clear();
    state_ = State::Root;

    entries_.push_back(FilesAppEntry {
        .name = "lfs",
        .directory = true,
        .size_bytes = 0,
    });

    if (storage_service != nullptr && storage_service->sd_card_present()) {
        entries_.push_back(FilesAppEntry {
            .name = "sdcard",
            .directory = true,
            .size_bytes = 0,
        });
    }
}

std::string FilesAppModel::parent_path_for(const std::string& path) {
    if (path.empty() || path == "/") {
        return path;
    }
    const auto separator = path.find_last_of('/');
    if (separator == std::string::npos || separator == 0) {
        return "/";
    }
    return path.substr(0, separator);
}

std::string FilesAppModel::join_path(const std::string& base, const std::string& name) {
    if (base.empty() || base == "/") {
        return "/" + name;
    }
    return base + "/" + name;
}

bool FilesAppModel::is_virtual_root(const std::string& path) {
    return path == "/";
}

std::string FilesAppModel::format_size(std::size_t size_bytes) {
    if (size_bytes >= (1024U * 1024U)) {
        return std::to_string(size_bytes / (1024U * 1024U)) + " MB";
    }
    if (size_bytes >= 1024U) {
        return std::to_string(size_bytes / 1024U) + " KB";
    }
    return std::to_string(size_bytes) + " B";
}

}  // namespace aegis::shell
