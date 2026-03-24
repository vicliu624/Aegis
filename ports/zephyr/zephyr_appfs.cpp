#include "ports/zephyr/zephyr_appfs.hpp"

#include <cerrno>
#include <cstdint>
#include <string>

#include <zephyr/devicetree.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/storage/flash_map.h>

#include "ports/zephyr/zephyr_build_config.hpp"

namespace aegis::ports::zephyr {

namespace {

#define AEGIS_APPFS_PARTITION_NODE DT_NODELABEL(storage_partition)

#if !DT_NODE_EXISTS(AEGIS_APPFS_PARTITION_NODE)
#error "Aegis Zephyr appfs requires a storage_partition devicetree node"
#endif

FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(aegis_appfs);

fs_mount_t& appfs_mount() {
    static fs_mount_t mount {
        .type = FS_LITTLEFS,
        .mnt_point = kAppFsMountPoint,
        .fs_data = &aegis_appfs,
        .storage_dev = reinterpret_cast<void*>(FIXED_PARTITION_ID(storage_partition)),
        .flags = 0,
    };
    return mount;
}

std::string rc_text(int rc) {
    return std::to_string(rc);
}

}  // namespace

ZephyrAppFs::ZephyrAppFs(platform::Logger& logger) : logger_(logger) {}

ZephyrAppFs::~ZephyrAppFs() {
    if (!mounted_) {
        return;
    }

    auto& mount = appfs_mount();
    const int rc = fs_unmount(&mount);
    if (rc == 0) {
        logger_.info("appfs", "unmounted app filesystem at " + std::string(kAppFsMountPoint));
    } else {
        logger_.info("appfs", "app filesystem unmount skipped rc=" + rc_text(rc));
    }
}

bool ZephyrAppFs::mount() {
    auto& mount = appfs_mount();
    int rc = fs_mount(&mount);
    if (rc != 0) {
        logger_.info("appfs", "initial mount failed rc=" + rc_text(rc) + ", attempting mkfs");
        rc = fs_mkfs(FS_LITTLEFS, reinterpret_cast<std::uintptr_t>(mount.storage_dev), mount.fs_data, 0);
        if (rc != 0) {
            logger_.info("appfs", "mkfs failed rc=" + rc_text(rc));
            return false;
        }

        rc = fs_mount(&mount);
        if (rc != 0) {
            logger_.info("appfs", "mount after mkfs failed rc=" + rc_text(rc));
            return false;
        }
    }

    mounted_ = true;
    logger_.info("appfs", "mounted LittleFS app filesystem at " + std::string(kAppFsMountPoint));

    if (!ensure_directory(kAppFsAppsRoot)) {
        logger_.info("appfs", "failed to ensure apps root " + std::string(kAppFsAppsRoot));
        return false;
    }

    logger_.info("appfs", "apps root ready at " + std::string(kAppFsAppsRoot));
    return true;
}

bool ZephyrAppFs::available() const noexcept {
    return mounted_;
}

std::string ZephyrAppFs::mount_point() const {
    return kAppFsMountPoint;
}

std::string ZephyrAppFs::apps_root() const {
    return kAppFsAppsRoot;
}

std::size_t ZephyrAppFs::app_directory_count() const {
    if (!mounted_) {
        return 0;
    }

    return count_directories(kAppFsAppsRoot);
}

std::string ZephyrAppFs::diagnostics_summary() const {
    if (!mounted_) {
        return "appfs unavailable";
    }

    return "appfs mounted apps=" + std::to_string(app_directory_count()) + " root=" +
           std::string(kAppFsAppsRoot);
}

bool ZephyrAppFs::ensure_directory(const std::string& path) const {
    fs_dirent entry {};
    const int stat_rc = fs_stat(path.c_str(), &entry);
    if (stat_rc == 0) {
        return entry.type == FS_DIR_ENTRY_DIR;
    }

    const int mkdir_rc = fs_mkdir(path.c_str());
    if (mkdir_rc == 0 || mkdir_rc == -EEXIST) {
        return true;
    }

    return false;
}

std::size_t ZephyrAppFs::count_directories(const std::string& path) const {
    fs_dir_t dir;
    fs_dirent entry {};
    std::size_t count = 0;

    fs_dir_t_init(&dir);
    if (fs_opendir(&dir, path.c_str()) != 0) {
        return 0;
    }

    while (fs_readdir(&dir, &entry) == 0 && entry.name[0] != '\0') {
        if (entry.type == FS_DIR_ENTRY_DIR) {
            ++count;
        }
    }

    fs_closedir(&dir);
    return count;
}

}  // namespace aegis::ports::zephyr
