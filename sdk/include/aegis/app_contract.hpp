#pragma once

#include <functional>
#include <string>

namespace aegis::runtime {
class HostApi;
}

namespace aegis::sdk {

enum class AppRunResult {
    Completed,
    Failed,
};

using AppEntrypoint = std::function<AppRunResult(runtime::HostApi&)>;

struct AppRuntimeContract {
    std::string entry_symbol;
    AppEntrypoint entrypoint;
};

}  // namespace aegis::sdk
