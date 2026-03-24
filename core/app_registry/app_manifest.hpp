#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "device/common/capability/capability_set.hpp"

namespace aegis::core {

struct AppManifest {
    std::string app_id;
    std::string display_name;
    std::string version;
    std::string abi_version;
    std::string entry_symbol;
    std::string description;
    std::vector<device::CapabilityId> required_capabilities;
    std::vector<device::CapabilityId> optional_capabilities;
};

struct AppDescriptor {
    AppManifest manifest;
    std::filesystem::path app_dir;
    std::filesystem::path manifest_path;
    std::filesystem::path binary_path;
    bool validated {false};
};

class ManifestParser {
public:
    [[nodiscard]] AppManifest parse_file(const std::filesystem::path& manifest_path) const;
};

}  // namespace aegis::core
