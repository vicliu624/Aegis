#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AEGIS_RADIO_TEXT_MAX_V1 96u

typedef enum aegis_radio_service_op_v1 {
    AEGIS_RADIO_SERVICE_OP_GET_STATUS = 1,
} aegis_radio_service_op_v1_t;

typedef struct aegis_radio_status_v1 {
    uint32_t available;
    char backend_name[AEGIS_RADIO_TEXT_MAX_V1];
} aegis_radio_status_v1_t;

#ifdef __cplusplus
}
#endif
