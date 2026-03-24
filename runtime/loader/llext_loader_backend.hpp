#pragma once

#include "platform/logging/logger.hpp"
#include "runtime/loader/llext_adapter.hpp"
#include "runtime/loader/loader_backend.hpp"

namespace aegis::runtime {

class LlextLoaderBackend : public LoaderBackend {
public:
    LlextLoaderBackend(platform::Logger& logger, LlextAdapterPtr adapter);

    LoaderPreparationResult prepare(const core::AppSession& session) override;
    [[nodiscard]] bool can_resolve_entry_symbol(const std::string& entry_symbol) const override;
    LoaderResult load(core::AppSession& session) override;
    LoaderResult bringup(core::AppSession& session, HostApi& host_api) override;
    LoaderResult teardown(core::AppSession& session,
                          ResourceOwnershipTable& ownership) override;
    LoaderResult unload(core::AppSession& session) override;

private:
    [[nodiscard]] sdk::AppRuntimeContract resolve_contract(const std::string& entry_symbol);

    platform::Logger& logger_;
    LlextAdapterPtr adapter_;
    RuntimeImageHandle image_;
};

}  // namespace aegis::runtime
