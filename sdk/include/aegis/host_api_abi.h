#pragma once

#include <stddef.h>
#include <stdint.h>

#include "sdk/include/aegis/host_api_status.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AEGIS_HOST_API_ABI_V1 1u

typedef enum aegis_capability_level_v1 {
    AEGIS_CAPABILITY_LEVEL_ABSENT = 0,
    AEGIS_CAPABILITY_LEVEL_DEGRADED = 1,
    AEGIS_CAPABILITY_LEVEL_FULL = 2,
} aegis_capability_level_v1_t;

typedef enum aegis_service_domain_v1 {
    AEGIS_SERVICE_DOMAIN_UI = 1,
    AEGIS_SERVICE_DOMAIN_INPUT = 2,
    AEGIS_SERVICE_DOMAIN_RADIO = 3,
    AEGIS_SERVICE_DOMAIN_GPS = 4,
    AEGIS_SERVICE_DOMAIN_AUDIO = 5,
    AEGIS_SERVICE_DOMAIN_SETTINGS = 6,
    AEGIS_SERVICE_DOMAIN_NOTIFICATION = 7,
    AEGIS_SERVICE_DOMAIN_STORAGE = 8,
    AEGIS_SERVICE_DOMAIN_POWER = 9,
    AEGIS_SERVICE_DOMAIN_TIME = 10,
    AEGIS_SERVICE_DOMAIN_HOSTLINK = 11,
    AEGIS_SERVICE_DOMAIN_TEXT_INPUT = 12,
} aegis_service_domain_v1_t;

typedef struct aegis_host_api_v1 {
    uint32_t abi_version;
    void* user_data;

    int (*log_write)(void* user_data, const char* tag, const char* message);
    int (*get_capability_level)(void* user_data, uint32_t capability_id);
    void* (*mem_alloc)(void* user_data, size_t size, size_t alignment);
    int (*mem_free)(void* user_data, void* ptr);
    int (*ui_create_root)(void* user_data, const char* root_name);
    int (*ui_destroy_root)(void* user_data, const char* root_name);
    int (*timer_create)(void* user_data, uint32_t timeout_ms, int repeat, int* timer_id);
    int (*timer_cancel)(void* user_data, int timer_id);
    int (*service_call)(void* user_data,
                        uint32_t domain,
                        uint32_t op,
                        const void* input,
                        size_t input_size,
                        void* output,
                        size_t* output_size);
} aegis_host_api_v1_t;

#ifdef __cplusplus
}
#endif
