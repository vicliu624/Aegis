#include "sdk/include/aegis/app_contract.hpp"

#include <sstream>

#include "runtime/host_api/host_api.hpp"

extern "C" aegis_app_run_result_v1_t demo_info_main(const aegis_host_api_v1_t* host_api);

namespace aegis::apps {

namespace {

sdk::AppRunResult run_demo_info(runtime::HostApi& host) {
    host.log("demo_info", "enter capability inspection app");
    host.create_ui_root("demo_info.root");
    void* scratch = host.allocate(64, alignof(std::max_align_t));

    std::ostringstream capabilities;
    for (const auto& descriptor : host.capabilities().list()) {
        capabilities << device::to_string(descriptor.id) << '='
                     << device::to_string(descriptor.level) << "; ";
    }

    host.log("demo_info", capabilities.str());
    host.log("demo_info", "radio backend=" + host.describe_radio());
    host.log("demo_info", "gps backend=" + host.describe_gps());
    host.set_setting("demo_info.last_seen", "ok");
    host.notify("Demo Info", "Capabilities queried without board branching");
    if (scratch != nullptr) {
        host.free(scratch);
    }
    host.destroy_ui_root("demo_info.root");
    return sdk::AppRunResult::Completed;
}

}  // namespace

sdk::AppRuntimeContract demo_info_contract() {
    return sdk::AppRuntimeContract {
        .entry_symbol = "demo_info_main",
        .entrypoint = run_demo_info,
        .abi_entrypoint = demo_info_main,
    };
}

}  // namespace aegis::apps
