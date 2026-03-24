#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace aegis::runtime {

class ResourceOwnershipTable {
public:
    void track(const std::string& session_id, std::string resource);
    [[nodiscard]] std::vector<std::string> release_all(const std::string& session_id);

private:
    std::unordered_map<std::string, std::vector<std::string>> owned_resources_;
};

}  // namespace aegis::runtime
