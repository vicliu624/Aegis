#pragma once

#include <optional>
#include <unordered_map>

#include "services/common/service_interfaces.hpp"

namespace aegis::services {

class InMemorySettingsService : public ISettingsService {
public:
    void set(std::string key, std::string value) override;
    [[nodiscard]] std::optional<std::string> find(const std::string& key) const override;

private:
    std::unordered_map<std::string, std::string> values_;
};

}  // namespace aegis::services
