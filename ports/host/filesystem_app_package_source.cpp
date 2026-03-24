#include "ports/host/filesystem_app_package_source.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace aegis::ports::host {

namespace {

std::string read_file(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("failed to open manifest: " + path.string());
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

}  // namespace

FilesystemAppPackageSource::FilesystemAppPackageSource(std::filesystem::path apps_root)
    : apps_root_(std::move(apps_root)) {}

std::vector<core::RawAppPackage> FilesystemAppPackageSource::discover() const {
    if (!std::filesystem::exists(apps_root_)) {
        throw std::runtime_error("apps directory does not exist: " + apps_root_.string());
    }

    std::vector<core::RawAppPackage> packages;
    for (const auto& entry : std::filesystem::directory_iterator(apps_root_)) {
        if (!entry.is_directory()) {
            continue;
        }

        const auto app_dir = entry.path();
        const auto manifest_path = app_dir / "manifest.json";
        if (!std::filesystem::exists(manifest_path)) {
            continue;
        }

        const auto binary_path = app_dir / "app.llext";
        const auto icon_path = app_dir / "icon.bin";

        packages.push_back(core::RawAppPackage {
            .app_dir = app_dir.string(),
            .manifest_path = manifest_path.string(),
            .binary_path = binary_path.string(),
            .icon_path = icon_path.string(),
            .manifest_text = read_file(manifest_path),
            .binary_exists = std::filesystem::exists(binary_path),
            .icon_exists = std::filesystem::exists(icon_path),
        });
    }

    return packages;
}

bool FilesystemAppPackageSource::exists(const std::string& path) const {
    return std::filesystem::exists(path);
}

}  // namespace aegis::ports::host
