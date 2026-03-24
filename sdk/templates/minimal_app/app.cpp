#include "sdk/include/aegis/app_contract.hpp"
#include "sdk/include/aegis/host_api_client.hpp"
#include "sdk/include/aegis/services/service_clients.hpp"

namespace aegis::template_app {

sdk::AppRunResult run_minimal_app_abi(const aegis_host_api_v1_t* host) {
    sdk::HostApiClient api(host);
    if (!api.valid()) {
        return sdk::AppRunResult::Failed;
    }

    api.log_write("minimal_app", "enter template app");
    api.create_ui_root("minimal_app.root");

    aegis_ui_display_info_v1_t display {};
    api.ui().get_display_info(display);
    api.log_write("minimal_app", display.surface_description);

    api.notification().publish("Minimal App", "Template app executed");
    return sdk::AppRunResult::Completed;
}

}  // namespace aegis::template_app

extern "C" aegis_app_run_result_v1_t minimal_app_main(const aegis_host_api_v1_t* host) {
    return aegis::sdk::to_abi_result(aegis::template_app::run_minimal_app_abi(host));
}

namespace aegis::template_app {

sdk::AppRuntimeContract minimal_app_contract() {
    return sdk::AppRuntimeContract {
        .entry_symbol = "minimal_app_main",
        .entrypoint = nullptr,
        .abi_entrypoint = minimal_app_main,
    };
}

}  // namespace aegis::template_app
