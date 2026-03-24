#pragma once

#include <stdint.h>

#include "sdk/include/aegis/host_api_abi.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AEGIS_APP_MODULE_ABI_V1 1u

typedef enum aegis_app_run_result_v1 {
    AEGIS_APP_RUN_RESULT_COMPLETED = 0,
    AEGIS_APP_RUN_RESULT_FAILED = 1,
} aegis_app_run_result_v1_t;

typedef aegis_app_run_result_v1_t (*aegis_app_main_v1_fn)(const aegis_host_api_v1_t* host_api);

typedef struct aegis_app_module_v1 {
    uint32_t abi_version;
    uint32_t flags;
    const char* entry_symbol;
    aegis_app_main_v1_fn main_fn;
} aegis_app_module_v1_t;

#ifdef __cplusplus
}
#endif
