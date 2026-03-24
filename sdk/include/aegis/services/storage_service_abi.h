#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AEGIS_STORAGE_TEXT_MAX_V1 96u

typedef enum aegis_storage_service_op_v1 {
    AEGIS_STORAGE_SERVICE_OP_GET_STATUS = 1,
} aegis_storage_service_op_v1_t;

typedef struct aegis_storage_status_v1 {
    uint32_t available;
    char backend_name[AEGIS_STORAGE_TEXT_MAX_V1];
} aegis_storage_status_v1_t;

#ifdef __cplusplus
}
#endif
