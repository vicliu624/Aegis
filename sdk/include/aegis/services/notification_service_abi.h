#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define AEGIS_NOTIFICATION_TITLE_MAX_V1 64u
#define AEGIS_NOTIFICATION_BODY_MAX_V1 160u

typedef enum aegis_notification_service_op_v1 {
    AEGIS_NOTIFICATION_SERVICE_OP_PUBLISH = 1,
} aegis_notification_service_op_v1_t;

typedef struct aegis_notification_publish_request_v1 {
    char title[AEGIS_NOTIFICATION_TITLE_MAX_V1];
    char body[AEGIS_NOTIFICATION_BODY_MAX_V1];
} aegis_notification_publish_request_v1_t;

#ifdef __cplusplus
}
#endif
