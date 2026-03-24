#include "runtime/ownership/resource_ownership_table.hpp"

namespace aegis::runtime {

void ResourceOwnershipTable::track(const std::string& session_id, std::string resource) {
    owned_resources_[session_id].push_back(std::move(resource));
}

std::vector<std::string> ResourceOwnershipTable::release_all(const std::string& session_id) {
    const auto it = owned_resources_.find(session_id);
    if (it == owned_resources_.end()) {
        return {};
    }

    auto released = std::move(it->second);
    owned_resources_.erase(it);
    return released;
}

}  // namespace aegis::runtime
