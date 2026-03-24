#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "core/app_registry/app_package_source.hpp"

namespace aegis::ports::zephyr {

class ZephyrAppPackageSource : public core::AppPackageSource {
public:
    explicit ZephyrAppPackageSource(std::vector<std::string> apps_roots = default_roots());

    [[nodiscard]] std::vector<core::RawAppPackage> discover() const override;
    [[nodiscard]] bool exists(const std::string& path) const override;
    [[nodiscard]] const std::vector<std::string>& apps_roots() const noexcept;
    [[nodiscard]] static std::vector<std::string> default_roots();

private:
    [[nodiscard]] static bool path_exists(const std::string& path);
    [[nodiscard]] static bool is_directory(const std::string& path);
    [[nodiscard]] static std::string join_path(const std::string& base, const std::string& name);
    [[nodiscard]] static std::string read_file(const std::string& path);
    [[nodiscard]] static std::vector<core::RawAppPackage> discover_root(std::string_view apps_root);

    std::vector<std::string> apps_roots_;
};

}  // namespace aegis::ports::zephyr
