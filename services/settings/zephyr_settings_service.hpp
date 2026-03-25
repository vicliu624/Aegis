#pragma once

#include <optional>
#include <unordered_map>
#include <string>

#include "services/common/service_interfaces.hpp"

namespace aegis::services {

class ZephyrSettingsService : public ISettingsService {
public:
    explicit ZephyrSettingsService(std::string namespace_root = "aegis");

    void set(std::string key, std::string value) override;
    [[nodiscard]] std::optional<std::string> find(const std::string& key) const override;

private:
    [[nodiscard]] std::string full_key(const std::string& key) const;

    std::string namespace_root_;
    mutable std::unordered_map<std::string, std::string> cache_;
};

}  // namespace aegis::services
