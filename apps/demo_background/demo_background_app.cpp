#include "sdk/include/aegis/app_contract.hpp"

#include "sdk/include/aegis/host_api_client.hpp"

namespace {

aegis::sdk::AppRunResult run_demo_background_abi(const aegis_host_api_v1_t* host) {
    const aegis::sdk::HostApiClient api(host);
    if (!api.valid()) {
        return aegis::sdk::AppRunResult::Failed;
    }

    api.log_write("demo_background", "background-capable app entered foreground runtime");
    return aegis::sdk::AppRunResult::Completed;
}

}  // namespace

extern "C" aegis_app_run_result_v1_t demo_background_main(const aegis_host_api_v1_t* host_api) {
    return aegis::sdk::to_abi_result(run_demo_background_abi(host_api));
}
