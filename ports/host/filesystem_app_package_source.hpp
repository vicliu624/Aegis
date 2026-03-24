#pragma once

#include <filesystem>

#include "core/app_registry/app_package_source.hpp"

namespace aegis::ports::host {

class FilesystemAppPackageSource : public core::AppPackageSource {
public:
    explicit FilesystemAppPackageSource(std::filesystem::path apps_root);

    [[nodiscard]] std::vector<core::RawAppPackage> discover() const override;
    [[nodiscard]] bool exists(const std::string& path) const override;

private:
    std::filesystem::path apps_root_;
};

}  // namespace aegis::ports::host
