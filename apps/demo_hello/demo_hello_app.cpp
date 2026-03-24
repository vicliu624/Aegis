#include "sdk/include/aegis/app_contract.hpp"

#include <sstream>

#include "runtime/host_api/host_api.hpp"

namespace aegis::apps {

namespace {

sdk::AppRunResult run_demo_hello(runtime::HostApi& host) {
    host.log("demo_hello", "enter demo hello app");
    host.create_ui_root("demo_hello.root");
    host.notify("Demo Hello", "Hello from a governed native app");

    std::ostringstream summary;
    summary << "display=" << host.describe_display() << ", input=" << host.describe_input();
    host.log("demo_hello", summary.str());
    host.log("demo_hello", "returning control to shell");
    return sdk::AppRunResult::Completed;
}

}  // namespace

sdk::AppRuntimeContract demo_hello_contract() {
    return sdk::AppRuntimeContract {
        .entry_symbol = "demo_hello_main",
        .entrypoint = run_demo_hello,
    };
}

}  // namespace aegis::apps
