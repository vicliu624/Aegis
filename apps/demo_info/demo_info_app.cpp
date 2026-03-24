#include "sdk/include/aegis/app_contract.hpp"

#include <sstream>

#include "device/common/capability/capability_set.hpp"
#include "runtime/host_api/host_api.hpp"

namespace aegis::apps {

namespace {

sdk::AppRunResult run_demo_info(runtime::HostApi& host) {
    host.log("demo_info", "enter capability inspection app");
    host.create_ui_root("demo_info.root");

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
    return sdk::AppRunResult::Completed;
}

}  // namespace

sdk::AppRuntimeContract demo_info_contract() {
    return sdk::AppRuntimeContract {
        .entry_symbol = "demo_info_main",
        .entrypoint = run_demo_info,
    };
}

}  // namespace aegis::apps
