#include "runtime/ownership/resource_ownership_table.hpp"

#include <algorithm>
#include <utility>

namespace aegis::runtime {

void ResourceOwnershipTable::track(const std::string& session_id,
                                   std::string resource,
                                   std::function<void()> release_hook) {
    auto& resources = owned_resources_[session_id];
    const auto it =
        std::find_if(resources.begin(),
                     resources.end(),
                     [&resource](const TrackedResource& tracked) { return tracked.name == resource; });
    if (it != resources.end()) {
        it->release_hook = std::move(release_hook);
        return;
    }

    resources.push_back(TrackedResource {
        .name = std::move(resource),
        .release_hook = std::move(release_hook),
    });
}

bool ResourceOwnershipTable::untrack(const std::string& session_id, const std::string& resource) {
    const auto it = owned_resources_.find(session_id);
    if (it == owned_resources_.end()) {
        return false;
    }

    auto& resources = it->second;
    const auto resource_it =
        std::find_if(resources.begin(),
                     resources.end(),
                     [&resource](const TrackedResource& tracked) { return tracked.name == resource; });
    if (resource_it == resources.end()) {
        return false;
    }

    resources.erase(resource_it);
    if (resources.empty()) {
        owned_resources_.erase(it);
    }
    return true;
}

std::vector<std::string> ResourceOwnershipTable::release_all(const std::string& session_id) {
    const auto it = owned_resources_.find(session_id);
    if (it == owned_resources_.end()) {
        return {};
    }

    auto owned = std::move(it->second);
    owned_resources_.erase(it);

    std::vector<std::string> released;
    released.reserve(owned.size());
    for (auto& resource : owned) {
        if (resource.release_hook) {
            resource.release_hook();
        }
        released.push_back(std::move(resource.name));
    }
    return released;
}

}  // namespace aegis::runtime
