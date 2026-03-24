#include "sdk/include/aegis/app_contract.hpp"

extern "C" aegis_app_run_result_v1_t demo_background_main(const aegis_host_api_v1_t* host_api);

namespace aegis::apps {

sdk::AppRuntimeContract demo_background_contract() {
    return sdk::AppRuntimeContract {
        .entry_symbol = "demo_background_main",
        .entrypoint = nullptr,
        .abi_entrypoint = demo_background_main,
    };
}

}  // namespace aegis::apps
