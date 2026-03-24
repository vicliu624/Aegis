#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AEGIS_TIME_TEXT_MAX_V1 96u

typedef enum aegis_time_service_op_v1 {
    AEGIS_TIME_SERVICE_OP_GET_STATUS = 1,
} aegis_time_service_op_v1_t;

typedef struct aegis_time_status_v1 {
    uint32_t available;
    char source_name[AEGIS_TIME_TEXT_MAX_V1];
} aegis_time_status_v1_t;

#ifdef __cplusplus
}
#endif
