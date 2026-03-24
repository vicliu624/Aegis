#pragma once

#include "platform/logging/logger.hpp"
#include "runtime/loader/llext_adapter.hpp"
#include "runtime/loader/loader_backend.hpp"
#include "sdk/include/aegis/app_contract.hpp"

namespace aegis::runtime {

class StubLoaderBackend : public LoaderBackend {
public:
    explicit StubLoaderBackend(platform::Logger& logger);
    StubLoaderBackend(platform::Logger& logger, LlextAdapterPtr adapter);

    LoaderPreparationResult prepare(const core::AppSession& session) override;
    [[nodiscard]] bool can_resolve_entry_symbol(const std::string& entry_symbol) const override;
    LoaderResult load(core::AppSession& session) override;
    LoaderResult bringup(core::AppSession& session, HostApi& host_api) override;
    LoaderResult teardown(core::AppSession& session,
                          ResourceOwnershipTable& ownership) override;
    LoaderResult unload(core::AppSession& session) override;

private:
    platform::Logger& logger_;
    LlextAdapterPtr adapter_;
    RuntimeImageHandle image_;
    [[nodiscard]] sdk::AppRuntimeContract resolve_contract(const std::string& entry_symbol);
};

}  // namespace aegis::runtime
