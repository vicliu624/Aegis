#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AEGIS_GPS_TEXT_MAX_V1 96u

typedef enum aegis_gps_service_op_v1 {
    AEGIS_GPS_SERVICE_OP_GET_STATUS = 1,
} aegis_gps_service_op_v1_t;

typedef struct aegis_gps_status_v1 {
    uint32_t available;
    char backend_name[AEGIS_GPS_TEXT_MAX_V1];
} aegis_gps_status_v1_t;

#ifdef __cplusplus
}
#endif
