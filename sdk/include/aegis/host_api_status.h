#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef enum aegis_host_status_v1 {
    AEGIS_HOST_STATUS_OK = 0,
    AEGIS_HOST_STATUS_INVALID_ARGUMENT = -1,
    AEGIS_HOST_STATUS_BUFFER_TOO_SMALL = -2,
    AEGIS_HOST_STATUS_UNSUPPORTED = -3,
} aegis_host_status_v1_t;

#ifdef __cplusplus
}
#endif
