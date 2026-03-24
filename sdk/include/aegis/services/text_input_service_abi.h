#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AEGIS_TEXT_INPUT_TEXT_MAX_V1 16u
#define AEGIS_TEXT_INPUT_SOURCE_MAX_V1 48u
#define AEGIS_TEXT_INPUT_SESSION_MAX_V1 64u
#define AEGIS_TEXT_INPUT_ROUTE_MAX_V1 32u

typedef enum aegis_text_input_service_op_v1 {
    AEGIS_TEXT_INPUT_SERVICE_OP_GET_STATE = 1,
    AEGIS_TEXT_INPUT_SERVICE_OP_GET_FOCUS_STATE = 2,
    AEGIS_TEXT_INPUT_SERVICE_OP_REQUEST_FOCUS = 3,
    AEGIS_TEXT_INPUT_SERVICE_OP_RELEASE_FOCUS = 4,
} aegis_text_input_service_op_v1_t;

typedef enum aegis_text_input_modifier_v1 {
    AEGIS_TEXT_INPUT_MODIFIER_SHIFT = 1u << 0,
    AEGIS_TEXT_INPUT_MODIFIER_CAPSLOCK = 1u << 1,
    AEGIS_TEXT_INPUT_MODIFIER_SYMBOL = 1u << 2,
    AEGIS_TEXT_INPUT_MODIFIER_ALT = 1u << 3,
} aegis_text_input_modifier_v1_t;

typedef enum aegis_text_input_focus_owner_v1 {
    AEGIS_TEXT_INPUT_FOCUS_OWNER_NONE = 0,
    AEGIS_TEXT_INPUT_FOCUS_OWNER_SHELL = 1,
    AEGIS_TEXT_INPUT_FOCUS_OWNER_APP_SESSION = 2,
} aegis_text_input_focus_owner_v1_t;

typedef struct aegis_text_input_state_v1 {
    uint32_t available;
    uint32_t text_entry;
    uint32_t last_key_code;
    uint32_t modifier_mask;
    char last_text[AEGIS_TEXT_INPUT_TEXT_MAX_V1];
    char source_name[AEGIS_TEXT_INPUT_SOURCE_MAX_V1];
} aegis_text_input_state_v1_t;

typedef struct aegis_text_input_focus_state_v1 {
    uint32_t available;
    uint32_t focused;
    uint32_t text_entry;
    uint32_t owner;
    char owner_session_id[AEGIS_TEXT_INPUT_SESSION_MAX_V1];
    char route_name[AEGIS_TEXT_INPUT_ROUTE_MAX_V1];
} aegis_text_input_focus_state_v1_t;

#ifdef __cplusplus
}
#endif
