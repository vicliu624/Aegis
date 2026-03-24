#pragma once

#include <memory>

#include "platform/logging/logger.hpp"
#include "core/app_session/app_session.hpp"
#include "runtime/host_api/host_api.hpp"
#include "runtime/loader/loader_backend.hpp"
#include "sdk/include/aegis/app_contract.hpp"

namespace aegis::runtime {

class RuntimeLoader {
public:
    explicit RuntimeLoader(platform::Logger& logger);
    RuntimeLoader(platform::Logger& logger, LoaderBackendPtr backend);

    LoaderPreparationResult prepare(const core::AppSession& session);
    [[nodiscard]] bool can_resolve_entry_symbol(const std::string& entry_symbol) const;
    LoaderResult load(core::AppSession& session);
    LoaderResult bringup(core::AppSession& session, HostApi& host_api);
    LoaderResult teardown(core::AppSession& session, ResourceOwnershipTable& ownership);
    LoaderResult unload(core::AppSession& session);

private:
    platform::Logger& logger_;
    LoaderBackendPtr backend_;
};

}  // namespace aegis::runtime
