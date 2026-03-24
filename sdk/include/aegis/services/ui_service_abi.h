#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AEGIS_UI_TEXT_MAX_V1 96u
#define AEGIS_UI_LAYOUT_CLASS_MAX_V1 32u
#define AEGIS_UI_PRIMARY_INPUT_MAX_V1 32u

typedef enum aegis_ui_service_op_v1 {
    AEGIS_UI_SERVICE_OP_GET_DISPLAY_INFO = 1,
    AEGIS_UI_SERVICE_OP_GET_INPUT_INFO = 2,
} aegis_ui_service_op_v1_t;

typedef struct aegis_ui_display_info_v1 {
    uint32_t width;
    uint32_t height;
    uint32_t touch;
    char layout_class[AEGIS_UI_LAYOUT_CLASS_MAX_V1];
    char surface_description[AEGIS_UI_TEXT_MAX_V1];
} aegis_ui_display_info_v1_t;

typedef struct aegis_ui_input_info_v1 {
    uint32_t keyboard;
    uint32_t pointer;
    uint32_t joystick;
    char primary_input[AEGIS_UI_PRIMARY_INPUT_MAX_V1];
    char input_mode[AEGIS_UI_TEXT_MAX_V1];
} aegis_ui_input_info_v1_t;

#ifdef __cplusplus
}
#endif
