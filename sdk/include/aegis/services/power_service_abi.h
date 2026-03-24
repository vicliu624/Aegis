#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AEGIS_POWER_TEXT_MAX_V1 96u

typedef enum aegis_power_service_op_v1 {
    AEGIS_POWER_SERVICE_OP_GET_STATUS = 1,
} aegis_power_service_op_v1_t;

typedef struct aegis_power_status_v1 {
    uint32_t battery_present;
    uint32_t low_power_mode_supported;
    char status_text[AEGIS_POWER_TEXT_MAX_V1];
} aegis_power_status_v1_t;

#ifdef __cplusplus
}
#endif
