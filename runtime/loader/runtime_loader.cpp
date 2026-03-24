#include "runtime/loader/runtime_loader.hpp"

#include <iostream>
#include <stdexcept>

namespace aegis::apps {
sdk::AppRuntimeContract demo_hello_contract();
sdk::AppRuntimeContract demo_info_contract();
}  // namespace aegis::apps

namespace aegis::runtime {

LoaderResult RuntimeLoader::load(core::AppSession& session) {
    session.transition_to(AppLifecycleState::Loaded);
    return LoaderResult {.ok = true, .detail = "stub loader admitted binary"};
}

LoaderResult RuntimeLoader::bringup(core::AppSession& session, HostApi& host_api) {
    session.transition_to(AppLifecycleState::Started);
    const auto contract = resolve_contract(session.descriptor().manifest.entry_symbol);
    const auto result = contract.entrypoint(host_api);
    session.transition_to(result == sdk::AppRunResult::Completed ? AppLifecycleState::Running
                                                                 : AppLifecycleState::Stopping);
    return LoaderResult {.ok = result == sdk::AppRunResult::Completed,
                         .detail = "stub bringup invoked " + contract.entry_symbol};
}

LoaderResult RuntimeLoader::teardown(core::AppSession& session, ResourceOwnershipTable& ownership) {
    session.transition_to(AppLifecycleState::Stopping);
    for (const auto& resource : ownership.release_all(session.id())) {
        std::cout << "[runtime] reclaim " << resource << '\n';
    }
    session.transition_to(AppLifecycleState::TornDown);
    return LoaderResult {.ok = true, .detail = "session resources reclaimed"};
}

LoaderResult RuntimeLoader::unload(core::AppSession& session) {
    session.transition_to(AppLifecycleState::Unloaded);
    session.transition_to(AppLifecycleState::ReturnedToShell);
    return LoaderResult {.ok = true, .detail = "stub loader unloaded image"};
}

sdk::AppRuntimeContract RuntimeLoader::resolve_contract(const std::string& entry_symbol) const {
    if (entry_symbol == "demo_hello_main") {
        return apps::demo_hello_contract();
    }
    if (entry_symbol == "demo_info_main") {
        return apps::demo_info_contract();
    }

    throw std::runtime_error("unknown app entry symbol: " + entry_symbol);
}

}  // namespace aegis::runtime
