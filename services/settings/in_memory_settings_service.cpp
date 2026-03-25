#include "services/settings/in_memory_settings_service.hpp"

namespace aegis::services {

void InMemorySettingsService::set(std::string key, std::string value) {
    values_[std::move(key)] = std::move(value);
}

std::optional<std::string> InMemorySettingsService::find(const std::string& key) const {
    const auto it = values_.find(key);
    if (it == values_.end()) {
        return std::nullopt;
    }
    return it->second;
}

}  // namespace aegis::services
