#include "services/settings/in_memory_settings_service.hpp"

namespace aegis::services {

void InMemorySettingsService::set(std::string key, std::string value) {
    values_[std::move(key)] = std::move(value);
}

std::string InMemorySettingsService::get(const std::string& key) const {
    const auto it = values_.find(key);
    return it == values_.end() ? std::string {} : it->second;
}

}  // namespace aegis::services
