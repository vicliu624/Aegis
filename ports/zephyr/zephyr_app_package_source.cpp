#include "ports/zephyr/zephyr_app_package_source.hpp"

#include <stdexcept>
#include <vector>

#include <zephyr/fs/fs.h>
#include <zephyr/sys/printk.h>

#include "ports/zephyr/zephyr_build_config.hpp"

namespace aegis::ports::zephyr {

namespace {

bool fs_path_exists(const std::string& path) {
    fs_dirent entry {};
    return fs_stat(path.c_str(), &entry) == 0;
}

bool fs_path_is_directory(const std::string& path) {
    fs_dirent entry {};
    return fs_stat(path.c_str(), &entry) == 0 && entry.type == FS_DIR_ENTRY_DIR;
}

}  // namespace

ZephyrAppPackageSource::ZephyrAppPackageSource(std::vector<std::string> apps_roots)
    : apps_roots_(std::move(apps_roots)) {}

std::vector<core::RawAppPackage> ZephyrAppPackageSource::discover() const {
    for (const auto& apps_root : apps_roots_) {
        printk("AEGIS TRACE: discover root %s\n", apps_root.c_str());
        if (!is_directory(apps_root)) {
            printk("AEGIS TRACE: root missing %s\n", apps_root.c_str());
            continue;
        }

        auto packages = discover_root(apps_root);
        printk("AEGIS TRACE: root %s packages=%u\n",
               apps_root.c_str(),
               static_cast<unsigned int>(packages.size()));
        if (!packages.empty()) {
            return packages;
        }
    }

    return {};
}

bool ZephyrAppPackageSource::exists(const std::string& path) const {
    return path_exists(path);
}

const std::vector<std::string>& ZephyrAppPackageSource::apps_roots() const noexcept {
    return apps_roots_;
}

std::vector<std::string> ZephyrAppPackageSource::default_roots() {
    return {"/apps", kAppFsAppsRoot};
}

bool ZephyrAppPackageSource::path_exists(const std::string& path) {
    return fs_path_exists(path);
}

bool ZephyrAppPackageSource::is_directory(const std::string& path) {
    return fs_path_is_directory(path);
}

std::vector<core::RawAppPackage> ZephyrAppPackageSource::discover_root(std::string_view apps_root) {
    fs_dir_t dir;
    fs_dirent entry {};
    std::vector<core::RawAppPackage> packages;
    const std::string root(apps_root);

    fs_dir_t_init(&dir);
    if (fs_opendir(&dir, root.c_str()) != 0) {
        return packages;
    }

    while (fs_readdir(&dir, &entry) == 0 && entry.name[0] != '\0') {
        printk("AEGIS TRACE: dir entry root=%s name=%s type=%d size=%zu\n",
               root.c_str(),
               entry.name,
               static_cast<int>(entry.type),
               static_cast<std::size_t>(entry.size));
        if (entry.type != FS_DIR_ENTRY_DIR) {
            continue;
        }

        printk("AEGIS TRACE: app dir %s/%s\n", root.c_str(), entry.name);
        const auto app_dir = join_path(root, entry.name);
        const auto manifest_path = join_path(app_dir, "manifest.json");
        const auto binary_path = join_path(app_dir, "app.llext");
        const auto icon_path = join_path(app_dir, "icon.bin");

        if (!fs_path_exists(manifest_path)) {
            continue;
        }

        try {
            const bool binary_exists = fs_path_exists(binary_path);
            const bool icon_exists = fs_path_exists(icon_path);
            printk("AEGIS TRACE: manifest=%s binary=%d icon=%d\n",
                   manifest_path.c_str(),
                   binary_exists ? 1 : 0,
                   icon_exists ? 1 : 0);
            packages.push_back(core::RawAppPackage {
                .app_dir = app_dir,
                .manifest_path = manifest_path,
                .binary_path = binary_path,
                .icon_path = icon_path,
                .manifest_text = read_file(manifest_path),
                .binary_exists = binary_exists,
                .icon_exists = icon_exists,
            });
        } catch (const std::exception&) {
            continue;
        }
    }

    fs_closedir(&dir);
    return packages;
}

std::string ZephyrAppPackageSource::join_path(const std::string& base, const std::string& name) {
    return base.ends_with('/') ? base + name : base + "/" + name;
}

std::string ZephyrAppPackageSource::read_file(const std::string& path) {
    fs_file_t file;
    fs_file_t_init(&file);

    if (fs_open(&file, path.c_str(), FS_O_READ) != 0) {
        throw std::runtime_error("failed to open app manifest: " + path);
    }

    std::string contents;
    std::vector<char> buffer(256);
    while (true) {
        const auto bytes = fs_read(&file, buffer.data(), buffer.size());
        if (bytes < 0) {
            fs_close(&file);
            throw std::runtime_error("failed to read app manifest: " + path);
        }
        if (bytes == 0) {
            break;
        }
        contents.append(buffer.data(), static_cast<std::size_t>(bytes));
    }

    fs_close(&file);
    return contents;
}

}  // namespace aegis::ports::zephyr
