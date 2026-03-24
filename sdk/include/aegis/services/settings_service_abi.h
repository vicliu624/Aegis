#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AEGIS_SETTINGS_KEY_MAX_V1 64u
#define AEGIS_SETTINGS_VALUE_MAX_V1 128u

typedef enum aegis_settings_service_op_v1 {
    AEGIS_SETTINGS_SERVICE_OP_SET = 1,
    AEGIS_SETTINGS_SERVICE_OP_GET = 2,
} aegis_settings_service_op_v1_t;

typedef struct aegis_settings_set_request_v1 {
    char key[AEGIS_SETTINGS_KEY_MAX_V1];
    char value[AEGIS_SETTINGS_VALUE_MAX_V1];
} aegis_settings_set_request_v1_t;

typedef struct aegis_settings_get_request_v1 {
    char key[AEGIS_SETTINGS_KEY_MAX_V1];
} aegis_settings_get_request_v1_t;

typedef struct aegis_settings_get_response_v1 {
    uint32_t found;
    char value[AEGIS_SETTINGS_VALUE_MAX_V1];
} aegis_settings_get_response_v1_t;

#ifdef __cplusplus
}
#endif
