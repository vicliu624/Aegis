#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace aegis::runtime {

class ResourceOwnershipTable {
public:
    void track(const std::string& session_id,
               std::string resource,
               std::function<void()> release_hook = {});
    [[nodiscard]] bool untrack(const std::string& session_id, const std::string& resource);
    [[nodiscard]] std::vector<std::string> release_all(const std::string& session_id);

private:
    struct TrackedResource {
        std::string name;
        std::function<void()> release_hook;
    };

    std::unordered_map<std::string, std::vector<TrackedResource>> owned_resources_;
};

}  // namespace aegis::runtime
