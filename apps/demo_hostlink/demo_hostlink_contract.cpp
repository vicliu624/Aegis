#include "sdk/include/aegis/app_contract.hpp"

extern "C" aegis_app_run_result_v1_t demo_hostlink_main(const aegis_host_api_v1_t* host_api);

namespace aegis::apps {

sdk::AppRuntimeContract demo_hostlink_contract() {
    return sdk::AppRuntimeContract {
        .entry_symbol = "demo_hostlink_main",
        .entrypoint = nullptr,
        .abi_entrypoint = demo_hostlink_main,
    };
}

}  // namespace aegis::apps
