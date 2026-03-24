#pragma once

#include <functional>
#include <string>

#include "sdk/include/aegis/app_module_abi.h"
#include "sdk/include/aegis/host_api_abi.h"

namespace aegis::runtime {
class HostApi;
}

namespace aegis::sdk {

enum class AppRunResult {
    Completed,
    Failed,
};

[[nodiscard]] inline AppRunResult from_abi_result(aegis_app_run_result_v1_t result) {
    return result == AEGIS_APP_RUN_RESULT_COMPLETED ? AppRunResult::Completed : AppRunResult::Failed;
}

[[nodiscard]] inline aegis_app_run_result_v1_t to_abi_result(AppRunResult result) {
    return result == AppRunResult::Completed ? AEGIS_APP_RUN_RESULT_COMPLETED
                                             : AEGIS_APP_RUN_RESULT_FAILED;
}

using AppEntrypoint = std::function<AppRunResult(runtime::HostApi&)>;
using AppAbiEntrypoint = aegis_app_run_result_v1_t (*)(const aegis_host_api_v1_t*);

struct AppRuntimeContract {
    std::string entry_symbol;
    AppEntrypoint entrypoint;
    AppAbiEntrypoint abi_entrypoint {nullptr};
};

}  // namespace aegis::sdk
