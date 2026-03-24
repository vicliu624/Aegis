#include "runtime/loader/runtime_loader.hpp"

#include <utility>

#include "runtime/loader/stub_loader_backend.hpp"

namespace aegis::runtime {

RuntimeLoader::RuntimeLoader(platform::Logger& logger)
    : RuntimeLoader(logger, std::make_unique<StubLoaderBackend>(logger)) {}

RuntimeLoader::RuntimeLoader(platform::Logger& logger, LoaderBackendPtr backend)
    : logger_(logger), backend_(std::move(backend)) {}

LoaderPreparationResult RuntimeLoader::prepare(const core::AppSession& session) {
    logger_.info("runtime", "prepare requested for " + session.descriptor().manifest.app_id);
    return backend_->prepare(session);
}

bool RuntimeLoader::can_resolve_entry_symbol(const std::string& entry_symbol) const {
    return backend_->can_resolve_entry_symbol(entry_symbol);
}

LoaderResult RuntimeLoader::load(core::AppSession& session) {
    logger_.info("runtime", "load requested for " + session.descriptor().manifest.app_id);
    return backend_->load(session);
}

LoaderResult RuntimeLoader::bringup(core::AppSession& session, HostApi& host_api) {
    logger_.info("runtime", "bringup requested for " + session.descriptor().manifest.app_id);
    return backend_->bringup(session, host_api);
}

LoaderResult RuntimeLoader::teardown(core::AppSession& session, ResourceOwnershipTable& ownership) {
    logger_.info("runtime", "teardown requested for " + session.descriptor().manifest.app_id);
    return backend_->teardown(session, ownership);
}

LoaderResult RuntimeLoader::unload(core::AppSession& session) {
    logger_.info("runtime", "unload requested for " + session.descriptor().manifest.app_id);
    return backend_->unload(session);
}

}  // namespace aegis::runtime
