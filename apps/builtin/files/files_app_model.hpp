#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "services/common/service_interfaces.hpp"

namespace aegis::shell {

struct FilesAppEntry {
    std::string name;
    bool directory {false};
    std::size_t size_bytes {0};
};

class FilesAppModel {
public:
    enum class State {
        Root,
        NoCard,
        Unavailable,
        Ready,
    };

    void load_root(const std::shared_ptr<services::IStorageService>& storage_service);
    [[nodiscard]] bool focus_next(bool wrap_navigation);
    [[nodiscard]] bool focus_previous(bool wrap_navigation);
    [[nodiscard]] bool open_focused(const std::shared_ptr<services::IStorageService>& storage_service);
    [[nodiscard]] bool go_parent(const std::shared_ptr<services::IStorageService>& storage_service);
    [[nodiscard]] State state() const;
    [[nodiscard]] const std::string& current_path() const;
    [[nodiscard]] const std::vector<FilesAppEntry>& entries() const;
    [[nodiscard]] std::size_t focus_index() const;
    [[nodiscard]] const FilesAppEntry* focused_entry() const;
    [[nodiscard]] std::string status_text() const;
    [[nodiscard]] static std::string format_size(std::size_t size_bytes);

private:
    void load_path(const std::shared_ptr<services::IStorageService>& storage_service,
                   std::string path);
    void load_virtual_root(const std::shared_ptr<services::IStorageService>& storage_service);
    [[nodiscard]] static std::string parent_path_for(const std::string& path);
    [[nodiscard]] static std::string join_path(const std::string& base, const std::string& name);
    [[nodiscard]] static bool is_virtual_root(const std::string& path);
    State state_ {State::Unavailable};
    std::string mount_root_ {"/"};
    std::string current_path_ {"/"};
    std::string status_text_override_;
    std::vector<FilesAppEntry> entries_;
    std::size_t focus_index_ {0};
};

}  // namespace aegis::shell
