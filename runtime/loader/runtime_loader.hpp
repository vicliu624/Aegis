#pragma once

#include "core/app_session/app_session.hpp"
#include "runtime/host_api/host_api.hpp"
#include "sdk/include/aegis/app_contract.hpp"

namespace aegis::runtime {

struct LoaderResult {
    bool ok {false};
    std::string detail;
};

class RuntimeLoader {
public:
    LoaderResult load(core::AppSession& session);
    LoaderResult bringup(core::AppSession& session, HostApi& host_api);
    LoaderResult teardown(core::AppSession& session, ResourceOwnershipTable& ownership);
    LoaderResult unload(core::AppSession& session);

private:
    [[nodiscard]] sdk::AppRuntimeContract resolve_contract(const std::string& entry_symbol) const;
};

}  // namespace aegis::runtime
