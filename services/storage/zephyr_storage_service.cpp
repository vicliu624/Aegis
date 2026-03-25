#include "services/storage/zephyr_storage_service.hpp"

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

std::string ZephyrStorageService::describe_backend() const {
    if (const auto* runtime = ports::zephyr::try_active_zephyr_board_runtime(); runtime != nullptr &&
        runtime->config().backend_id == config_.backend_id) {
        return std::string("zephyr-board-storage:mount=") + mount_root_ +
               ",power=" + (runtime->storage_ready() ? "ready" : "gated") +
               ",shared-spi=" + (runtime->shared_spi_ready() ? "ready" : "missing") +
               ",owner=" + runtime->shared_spi_owner_name() +
               ",sd-present=" + (runtime->sd_card_present() ? "1" : "0");
    }
    return available() ? "zephyr-fs:" + mount_root_ : "zephyr-fs-unavailable:" + mount_root_;
}

}  // namespace aegis::services
