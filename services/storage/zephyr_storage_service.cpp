#include "services/storage/zephyr_storage_service.hpp"

#include <zephyr/fs/fs.h>

namespace aegis::services {

ZephyrStorageService::ZephyrStorageService(std::string mount_root)
    : mount_root_(std::move(mount_root)) {}

bool ZephyrStorageService::available() const {
    fs_dirent entry {};
    return fs_stat(mount_root_.c_str(), &entry) == 0;
}

std::string ZephyrStorageService::describe_backend() const {
    return available() ? "zephyr-fs:" + mount_root_ : "zephyr-fs-unavailable:" + mount_root_;
}

}  // namespace aegis::services
