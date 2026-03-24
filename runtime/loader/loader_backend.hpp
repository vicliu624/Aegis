#pragma once

#include <memory>
#include <string>

#include "core/app_session/app_session.hpp"
#include "runtime/host_api/host_api.hpp"
#include "runtime/ownership/resource_ownership_table.hpp"

namespace aegis::runtime {

struct LoaderPreparationResult {
    bool ok {false};
    std::string detail;
};

struct LoaderResult {
    bool ok {false};
    std::string detail;
};

class LoaderBackend {
public:
    virtual ~LoaderBackend() = default;

    virtual LoaderPreparationResult prepare(const core::AppSession& session) = 0;
    [[nodiscard]] virtual bool can_resolve_entry_symbol(const std::string& entry_symbol) const = 0;
    virtual LoaderResult load(core::AppSession& session) = 0;
    virtual LoaderResult bringup(core::AppSession& session, HostApi& host_api) = 0;
    virtual LoaderResult teardown(core::AppSession& session,
                                  ResourceOwnershipTable& ownership) = 0;
    virtual LoaderResult unload(core::AppSession& session) = 0;
};

using LoaderBackendPtr = std::unique_ptr<LoaderBackend>;

}  // namespace aegis::runtime
