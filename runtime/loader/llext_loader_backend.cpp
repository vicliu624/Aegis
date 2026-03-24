#include "runtime/loader/llext_loader_backend.hpp"

#include <stdexcept>
#include <utility>

#include "runtime/loader/compiled_app_contract_registry.hpp"

namespace aegis::runtime {

LlextLoaderBackend::LlextLoaderBackend(platform::Logger& logger, LlextAdapterPtr adapter)
    : logger_(logger), adapter_(std::move(adapter)) {}

LoaderPreparationResult LlextLoaderBackend::prepare(const core::AppSession& session) {
    if (!session.descriptor().validated) {
        return LoaderPreparationResult {.ok = false, .detail = "descriptor not validated"};
    }
    if (session.descriptor().binary_path.empty()) {
        return LoaderPreparationResult {.ok = false, .detail = "binary path missing"};
    }
    if (!session.descriptor().binary_exists) {
        return LoaderPreparationResult {.ok = false, .detail = "binary missing"};
    }
    if (!can_resolve_entry_symbol(session.descriptor().manifest.entry_symbol)) {
        return LoaderPreparationResult {.ok = false, .detail = "entry symbol unavailable"};
    }
    const auto adapter_result = adapter_->prepare_image(session.descriptor().binary_path);
    return LoaderPreparationResult {.ok = adapter_result.ok, .detail = adapter_result.detail};
}

bool LlextLoaderBackend::can_resolve_entry_symbol(const std::string& entry_symbol) const {
    return !entry_symbol.empty();
}

LoaderResult LlextLoaderBackend::load(core::AppSession& session) {
    const auto adapter_result = adapter_->load_image(session.descriptor().binary_path, image_);
    if (!adapter_result.ok) {
        return LoaderResult {.ok = false, .detail = adapter_result.detail};
    }
    session.transition_to(AppLifecycleState::Loaded);
    return LoaderResult {.ok = true, .detail = adapter_result.detail};
}

LoaderResult LlextLoaderBackend::bringup(core::AppSession& session, HostApi& host_api) {
    session.transition_to(AppLifecycleState::Started);
    const auto contract = resolve_contract(session.descriptor().manifest.entry_symbol);
    const auto result =
        contract.abi_entrypoint != nullptr
            ? sdk::from_abi_result(contract.abi_entrypoint(host_api.abi()))
            : contract.entrypoint(host_api);
    session.transition_to(result == sdk::AppRunResult::Completed ? AppLifecycleState::Running
                                                                 : AppLifecycleState::Stopping);
    return LoaderResult {.ok = result == sdk::AppRunResult::Completed,
                         .detail = "llext bringup invoked " + contract.entry_symbol};
}

LoaderResult LlextLoaderBackend::teardown(core::AppSession& session,
                                          ResourceOwnershipTable& ownership) {
    session.transition_to(AppLifecycleState::Stopping);
    for (const auto& resource : ownership.release_all(session.id())) {
        logger_.info("runtime", "reclaim " + resource);
    }
    session.transition_to(AppLifecycleState::TornDown);
    return LoaderResult {.ok = true, .detail = "session resources reclaimed"};
}

LoaderResult LlextLoaderBackend::unload(core::AppSession& session) {
    const auto adapter_result = adapter_->unload_image(image_);
    if (!adapter_result.ok) {
        return LoaderResult {.ok = false, .detail = adapter_result.detail};
    }
    session.transition_to(AppLifecycleState::Unloaded);
    session.transition_to(AppLifecycleState::ReturnedToShell);
    return LoaderResult {.ok = true, .detail = adapter_result.detail};
}

sdk::AppRuntimeContract LlextLoaderBackend::resolve_contract(const std::string& entry_symbol) {
    sdk::AppRuntimeContract contract;
    const auto adapter_result = adapter_->resolve_entry(image_, entry_symbol, contract);
    if (!adapter_result.ok) {
        throw std::runtime_error(adapter_result.detail);
    }
    return contract;
}

}  // namespace aegis::runtime
