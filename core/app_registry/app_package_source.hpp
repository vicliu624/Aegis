#pragma once

#include <string>
#include <vector>

namespace aegis::core {

struct RawAppPackage {
    std::string app_dir;
    std::string manifest_path;
    std::string binary_path;
    std::string icon_path;
    std::string manifest_text;
    bool binary_exists {false};
    bool icon_exists {false};
};

class AppPackageSource {
public:
    virtual ~AppPackageSource() = default;
    [[nodiscard]] virtual std::vector<RawAppPackage> discover() const = 0;
    [[nodiscard]] virtual bool exists(const std::string& path) const = 0;
};

}  // namespace aegis::core
