#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AEGIS_UI_TEXT_MAX_V1 96u
#define AEGIS_UI_LAYOUT_CLASS_MAX_V1 32u
#define AEGIS_UI_PRIMARY_INPUT_MAX_V1 32u
#define AEGIS_UI_PAGE_ID_MAX_V1 32u
#define AEGIS_UI_PAGE_STATE_TOKEN_MAX_V1 32u
#define AEGIS_UI_PAGE_TITLE_MAX_V1 32u
#define AEGIS_UI_PAGE_CONTEXT_MAX_V1 48u
#define AEGIS_UI_SOFTKEY_LABEL_MAX_V1 16u
#define AEGIS_UI_COMMAND_ID_MAX_V1 24u
#define AEGIS_UI_PAGE_ITEM_MAX_V1 8u
#define AEGIS_UI_PAGE_ITEM_LABEL_MAX_V1 32u
#define AEGIS_UI_PAGE_ITEM_DETAIL_MAX_V1 48u
#define AEGIS_UI_PAGE_ITEM_ICON_KEY_MAX_V1 48u
#define AEGIS_UI_DIALOG_BODY_MAX_V1 128u

typedef enum aegis_ui_service_op_v1 {
    AEGIS_UI_SERVICE_OP_GET_DISPLAY_INFO = 1,
    AEGIS_UI_SERVICE_OP_GET_INPUT_INFO = 2,
    AEGIS_UI_SERVICE_OP_SET_FOREGROUND_PAGE = 3,
    AEGIS_UI_SERVICE_OP_POLL_EVENT = 4,
} aegis_ui_service_op_v1_t;

typedef enum aegis_ui_softkey_role_v1 {
    AEGIS_UI_SOFTKEY_ROLE_PRIMARY = 1,
    AEGIS_UI_SOFTKEY_ROLE_SECONDARY = 2,
    AEGIS_UI_SOFTKEY_ROLE_BACK = 3,
    AEGIS_UI_SOFTKEY_ROLE_MENU = 4,
    AEGIS_UI_SOFTKEY_ROLE_CONFIRM = 5,
    AEGIS_UI_SOFTKEY_ROLE_INFO = 6,
    AEGIS_UI_SOFTKEY_ROLE_CUSTOM = 7,
} aegis_ui_softkey_role_v1_t;

typedef enum aegis_ui_event_type_v1 {
    AEGIS_UI_EVENT_TYPE_NONE = 0,
    AEGIS_UI_EVENT_TYPE_PAGE_COMMAND = 1,
    AEGIS_UI_EVENT_TYPE_ROUTED_ACTION = 2,
} aegis_ui_event_type_v1_t;

typedef enum aegis_ui_event_source_v1 {
    AEGIS_UI_EVENT_SOURCE_UNKNOWN = 0,
    AEGIS_UI_EVENT_SOURCE_SOFTKEY_LEFT = 1,
    AEGIS_UI_EVENT_SOURCE_SOFTKEY_CENTER = 2,
    AEGIS_UI_EVENT_SOURCE_SOFTKEY_RIGHT = 3,
    AEGIS_UI_EVENT_SOURCE_PHYSICAL = 4,
} aegis_ui_event_source_v1_t;

typedef enum aegis_ui_routed_action_v1 {
    AEGIS_UI_ROUTED_ACTION_NONE = 0,
    AEGIS_UI_ROUTED_ACTION_MOVE_NEXT = 1,
    AEGIS_UI_ROUTED_ACTION_MOVE_PREVIOUS = 2,
    AEGIS_UI_ROUTED_ACTION_PRIMARY = 3,
    AEGIS_UI_ROUTED_ACTION_SELECT = 4,
    AEGIS_UI_ROUTED_ACTION_BACK = 5,
    AEGIS_UI_ROUTED_ACTION_OPEN_MENU = 6,
    AEGIS_UI_ROUTED_ACTION_OPEN_FILES = 7,
    AEGIS_UI_ROUTED_ACTION_OPEN_SETTINGS = 8,
    AEGIS_UI_ROUTED_ACTION_OPEN_NOTIFICATIONS = 9,
} aegis_ui_routed_action_v1_t;

typedef enum aegis_ui_page_template_v1 {
    AEGIS_UI_PAGE_TEMPLATE_TEXT_LOG = 0,
    AEGIS_UI_PAGE_TEMPLATE_SIMPLE_LIST = 1,
    AEGIS_UI_PAGE_TEMPLATE_VALUE_LIST = 2,
    AEGIS_UI_PAGE_TEMPLATE_FILE_LIST = 3,
} aegis_ui_page_template_v1_t;

typedef enum aegis_ui_item_accessory_v1 {
    AEGIS_UI_ITEM_ACCESSORY_NONE = 0,
    AEGIS_UI_ITEM_ACCESSORY_FILE = 1,
    AEGIS_UI_ITEM_ACCESSORY_FOLDER = 2,
} aegis_ui_item_accessory_v1_t;

typedef enum aegis_ui_item_icon_source_v1 {
    AEGIS_UI_ITEM_ICON_SOURCE_NONE = 0,
    AEGIS_UI_ITEM_ICON_SOURCE_APP_ASSET = 1,
} aegis_ui_item_icon_source_v1_t;

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

typedef struct aegis_ui_softkey_v1 {
    uint32_t visible;
    uint32_t enabled;
    uint32_t role;
    char label[AEGIS_UI_SOFTKEY_LABEL_MAX_V1];
    char command_id[AEGIS_UI_COMMAND_ID_MAX_V1];
} aegis_ui_softkey_v1_t;

typedef struct aegis_ui_page_item_v1 {
    uint32_t focused;
    uint32_t emphasized;
    uint32_t warning;
    uint32_t disabled;
    uint32_t accessory;
    uint32_t icon_source;
    char item_id[AEGIS_UI_COMMAND_ID_MAX_V1];
    char label[AEGIS_UI_PAGE_ITEM_LABEL_MAX_V1];
    char detail[AEGIS_UI_PAGE_ITEM_DETAIL_MAX_V1];
    char icon_key[AEGIS_UI_PAGE_ITEM_ICON_KEY_MAX_V1];
} aegis_ui_page_item_v1_t;

typedef struct aegis_ui_dialog_v1 {
    uint32_t visible;
    char title[AEGIS_UI_PAGE_TITLE_MAX_V1];
    char body[AEGIS_UI_DIALOG_BODY_MAX_V1];
} aegis_ui_dialog_v1_t;

typedef struct aegis_ui_foreground_page_v1 {
    char page_id[AEGIS_UI_PAGE_ID_MAX_V1];
    char page_state_token[AEGIS_UI_PAGE_STATE_TOKEN_MAX_V1];
    char title[AEGIS_UI_PAGE_TITLE_MAX_V1];
    char context[AEGIS_UI_PAGE_CONTEXT_MAX_V1];
    uint32_t layout_template;
    uint32_t item_count;
    aegis_ui_softkey_v1_t softkeys[3];
    aegis_ui_page_item_v1_t items[AEGIS_UI_PAGE_ITEM_MAX_V1];
    aegis_ui_dialog_v1_t dialog;
} aegis_ui_foreground_page_v1_t;

typedef struct aegis_ui_routed_event_v1 {
    uint32_t event_type;
    uint32_t event_source;
    uint32_t routed_action;
    uint32_t reserved;
    char page_id[AEGIS_UI_PAGE_ID_MAX_V1];
    char page_state_token[AEGIS_UI_PAGE_STATE_TOKEN_MAX_V1];
    char command_id[AEGIS_UI_COMMAND_ID_MAX_V1];
} aegis_ui_routed_event_v1_t;

#ifdef __cplusplus
}
#endif
