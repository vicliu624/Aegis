#include "sdk/include/aegis/app_contract.hpp"

namespace {

constexpr const char* kTag = "demo_hello";
constexpr const char* kRootName = "demo_hello.root";

}  // namespace

extern "C" aegis_app_run_result_v1_t demo_hello_main(const aegis_host_api_v1_t* host_api) {
    if (host_api == nullptr || host_api->abi_version != AEGIS_HOST_API_ABI_V1) {
        return AEGIS_APP_RUN_RESULT_FAILED;
    }

    if (host_api->log_write != nullptr) {
        host_api->log_write(host_api->user_data, kTag, "enter demo hello app");
    }
    if (host_api->ui_create_root != nullptr) {
        host_api->ui_create_root(host_api->user_data, kRootName);
    }
    if (host_api->log_write != nullptr) {
        host_api->log_write(host_api->user_data, kTag, "returning control to shell");
    }
    if (host_api->ui_destroy_root != nullptr) {
        host_api->ui_destroy_root(host_api->user_data, kRootName);
    }

    return AEGIS_APP_RUN_RESULT_COMPLETED;
}
