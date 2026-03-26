#include "services/storage/zephyr_storage_service.hpp"

#include <algorithm>

#include <zephyr/fs/fs.h>

#include "ports/zephyr/zephyr_board_runtime.hpp"

namespace aegis::services {

ZephyrStorageService::ZephyrStorageService(std::string mount_root)
    : mount_root_(std::move(mount_root)) {}

ZephyrStorageService::ZephyrStorageService(std::string mount_root,
                                           ports::zephyr::ZephyrBoardBackendConfig config)
    : mount_root_(std::move(mount_root)),
      config_(std::move(config)) {}

bool ZephyrStorageService::available() const {
    if (const auto* runtime = ports::zephyr::try_active_zephyr_board_runtime(); runtime != nullptr &&
        runtime->config().backend_id == config_.backend_id && !runtime->storage_ready()) {
        return false;
    }
    fs_dirent entry {};
    return fs_stat(mount_root_.c_str(), &entry) == 0;
}

bool ZephyrStorageService::sd_card_present() const {
    if (const auto* runtime = ports::zephyr::try_active_zephyr_board_runtime(); runtime != nullptr &&
        runtime->config().backend_id == config_.backend_id) {
        return runtime->sd_card_present();
    }
    return available();
}

std::string ZephyrStorageService::mount_root() const {
    return mount_root_;
}

bool ZephyrStorageService::directory_exists(const std::string& path) const {
    fs_dirent entry {};
    return fs_stat(path.c_str(), &entry) == 0 && entry.type == FS_DIR_ENTRY_DIR;
}

std::vector<StorageDirectoryEntry> ZephyrStorageService::list_directory(const std::string& path) const {
    std::vector<StorageDirectoryEntry> entries;
    if (!available()) {
        return entries;
    }

    fs_dir_t dir;
    fs_dir_t_init(&dir);
    if (fs_opendir(&dir, path.c_str()) != 0) {
        return entries;
    }

    while (true) {
        fs_dirent entry {};
        const int rc = fs_readdir(&dir, &entry);
        if (rc != 0 || entry.name[0] == '\0') {
            break;
        }
        if (std::string_view(entry.name) == "." || std::string_view(entry.name) == "..") {
            continue;
        }
        entries.push_back(StorageDirectoryEntry {
            .name = entry.name,
            .directory = entry.type == FS_DIR_ENTRY_DIR,
            .size_bytes = static_cast<std::size_t>(entry.size),
        });
    }

    (void)fs_closedir(&dir);
    std::sort(entries.begin(), entries.end(), [](const StorageDirectoryEntry& lhs,
                                                 const StorageDirectoryEntry& rhs) {
        if (lhs.directory != rhs.directory) {
            return lhs.directory > rhs.directory;
        }
        return lhs.name < rhs.name;
    });
    return entries;
}

std::string ZephyrStorageService::describe_backend() const {
    if (const auto* runtime = ports::zephyr::try_active_zephyr_board_runtime(); runtime != nullptr &&
        runtime->config().backend_id == config_.backend_id) {
        return std::string("zephyr-board-storage:mount=") + mount_root_ +
               ",power=" + (runtime->storage_ready() ? "ready" : "gated") +
               ",coordination-domain=" +
                   (runtime->coordination_domain_ready(ports::zephyr::ZephyrBoardCoordinationDomain::StorageSession)
                        ? "ready"
                        : "missing") +
               ",coordinator=" +
                   runtime->coordination_domain_coordinator_name(
                       ports::zephyr::ZephyrBoardCoordinationDomain::StorageSession) +
               ",owner=" +
                   runtime->coordination_domain_owner_name(
                       ports::zephyr::ZephyrBoardCoordinationDomain::StorageSession) +
               ",sd-present=" + (runtime->sd_card_present() ? "1" : "0");
    }
    return available() ? "zephyr-fs:" + mount_root_ : "zephyr-fs-unavailable:" + mount_root_;
}

}  // namespace aegis::services
