#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AEGIS_STORAGE_TEXT_MAX_V1 96u
#define AEGIS_STORAGE_PATH_MAX_V1 96u
#define AEGIS_STORAGE_ENTRY_NAME_MAX_V1 48u
#define AEGIS_STORAGE_DIRECTORY_ENTRY_MAX_V1 16u

typedef enum aegis_storage_service_op_v1 {
    AEGIS_STORAGE_SERVICE_OP_GET_STATUS = 1,
    AEGIS_STORAGE_SERVICE_OP_LIST_DIRECTORY = 2,
} aegis_storage_service_op_v1_t;

typedef enum aegis_storage_list_status_v1 {
    AEGIS_STORAGE_LIST_STATUS_OK = 0,
    AEGIS_STORAGE_LIST_STATUS_UNAVAILABLE = 1,
    AEGIS_STORAGE_LIST_STATUS_NOT_FOUND = 2,
    AEGIS_STORAGE_LIST_STATUS_NO_CARD = 3,
} aegis_storage_list_status_v1_t;

typedef struct aegis_storage_status_v1 {
    uint32_t available;
    uint32_t sd_card_present;
    char backend_name[AEGIS_STORAGE_TEXT_MAX_V1];
    char mount_root[AEGIS_STORAGE_PATH_MAX_V1];
} aegis_storage_status_v1_t;

typedef struct aegis_storage_directory_entry_v1 {
    uint32_t directory;
    uint32_t size_bytes;
    char name[AEGIS_STORAGE_ENTRY_NAME_MAX_V1];
} aegis_storage_directory_entry_v1_t;

typedef struct aegis_storage_list_directory_request_v1 {
    uint32_t offset;
    uint32_t limit;
    char path[AEGIS_STORAGE_PATH_MAX_V1];
} aegis_storage_list_directory_request_v1_t;

typedef struct aegis_storage_list_directory_response_v1 {
    uint32_t status;
    uint32_t entry_count;
    uint32_t has_more;
    uint32_t next_offset;
    aegis_storage_directory_entry_v1_t entries[AEGIS_STORAGE_DIRECTORY_ENTRY_MAX_V1];
} aegis_storage_list_directory_response_v1_t;

#ifdef __cplusplus
}
#endif
