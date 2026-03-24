#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AEGIS_AUDIO_TEXT_MAX_V1 96u

typedef enum aegis_audio_service_op_v1 {
    AEGIS_AUDIO_SERVICE_OP_GET_STATUS = 1,
} aegis_audio_service_op_v1_t;

typedef struct aegis_audio_status_v1 {
    uint32_t output_available;
    uint32_t input_available;
    char backend_name[AEGIS_AUDIO_TEXT_MAX_V1];
} aegis_audio_status_v1_t;

#ifdef __cplusplus
}
#endif
