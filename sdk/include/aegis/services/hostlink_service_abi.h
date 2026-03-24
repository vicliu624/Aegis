#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AEGIS_HOSTLINK_SERVICE_ABI_V1 1u

typedef enum aegis_hostlink_service_op_v1 {
    AEGIS_HOSTLINK_SERVICE_OP_GET_STATUS = 1,
} aegis_hostlink_service_op_v1_t;

typedef struct aegis_hostlink_status_v1 {
    uint32_t available;
    uint32_t connected;
    char transport_name[64];
    char bridge_name[64];
} aegis_hostlink_status_v1_t;

#ifdef __cplusplus
}
#endif
