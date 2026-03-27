#include "sdk/include/aegis/app_module_abi.h"
#include "sdk/include/aegis/host_api_abi.h"
#include "sdk/include/aegis/services/audio_service_abi.h"
#include "sdk/include/aegis/services/gps_service_abi.h"
#include "sdk/include/aegis/services/hostlink_service_abi.h"
#include "sdk/include/aegis/services/notification_service_abi.h"
#include "sdk/include/aegis/services/power_service_abi.h"
#include "sdk/include/aegis/services/radio_service_abi.h"
#include "sdk/include/aegis/services/settings_service_abi.h"
#include "sdk/include/aegis/services/storage_service_abi.h"
#include "sdk/include/aegis/services/text_input_service_abi.h"
#include "sdk/include/aegis/services/time_service_abi.h"
#include "sdk/include/aegis/services/ui_service_abi.h"

#include <cstddef>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

namespace {

constexpr const char* kTag = "demo_info_abi";
constexpr const char* kRootName = "demo_info_abi.root";

void append_char(char* buffer, size_t capacity, size_t& used, char value) {
    if (used + 1 >= capacity) {
        return;
    }
    buffer[used++] = value;
    buffer[used] = '\0';
}

void append_text(char* buffer, size_t capacity, size_t& used, const char* text) {
    if (text == nullptr || capacity == 0 || used >= capacity) {
        return;
    }

    while (*text != '\0' && used + 1 < capacity) {
        buffer[used++] = *text++;
    }
    buffer[used] = '\0';
}

void append_uint(char* buffer, size_t capacity, size_t& used, uint32_t value) {
    char digits[11];
    size_t count = 0;

    do {
        digits[count++] = static_cast<char>('0' + (value % 10u));
        value /= 10u;
    } while (value != 0 && count < sizeof(digits));

    while (count > 0) {
        append_char(buffer, capacity, used, digits[--count]);
    }
}

void append_int(char* buffer, size_t capacity, size_t& used, int value) {
    if (value < 0) {
        append_char(buffer, capacity, used, '-');
        append_uint(buffer, capacity, used, static_cast<uint32_t>(-(value + 1)) + 1u);
        return;
    }
    append_uint(buffer, capacity, used, static_cast<uint32_t>(value));
}

void copy_text(char* destination, size_t capacity, const char* source) {
    size_t used = 0;
    if (capacity == 0) {
        return;
    }
    destination[0] = '\0';
    append_text(destination, capacity, used, source);
}

const char* capability_name(uint32_t id) {
    if (id == 0) {
        return "display";
    }
    if (id == 1) {
        return "keyboard_input";
    }
    if (id == 2) {
        return "pointer_input";
    }
    if (id == 3) {
        return "joystick_input";
    }
    if (id == 4) {
        return "radio_messaging";
    }
    if (id == 5) {
        return "gps_fix";
    }
    if (id == 6) {
        return "audio_output";
    }
    if (id == 7) {
        return "microphone_input";
    }
    if (id == 8) {
        return "battery_status";
    }
    if (id == 9) {
        return "vibration";
    }
    if (id == 10) {
        return "removable_storage";
    }
    if (id == 11) {
        return "usb_hostlink";
    }
    return "unknown";
}

const char* capability_level_name(int level) {
    if (level == AEGIS_CAPABILITY_LEVEL_ABSENT) {
        return "absent";
    }
    if (level == AEGIS_CAPABILITY_LEVEL_DEGRADED) {
        return "degraded";
    }
    if (level == AEGIS_CAPABILITY_LEVEL_FULL) {
        return "full";
    }
    return "unknown";
}

const char* focus_owner_name(uint32_t owner) {
    if (owner == AEGIS_TEXT_INPUT_FOCUS_OWNER_NONE) {
        return "none";
    }
    if (owner == AEGIS_TEXT_INPUT_FOCUS_OWNER_SHELL) {
        return "shell";
    }
    if (owner == AEGIS_TEXT_INPUT_FOCUS_OWNER_APP_SESSION) {
        return "app_session";
    }
    return "unknown";
}

int log_line(const aegis_host_api_v1_t* host_api, const char* message) {
    if (host_api->log_write == nullptr) {
        return 0;
    }
    return host_api->log_write(host_api->user_data, kTag, message);
}

int service_status_only(const aegis_host_api_v1_t* host_api,
                        uint32_t domain,
                        uint32_t op,
                        void* output,
                        size_t output_size) {
    if (host_api->service_call == nullptr) {
        return -1;
    }
    size_t actual_output_size = output_size;
    return host_api->service_call(
        host_api->user_data, domain, op, nullptr, 0, output, &actual_output_size);
}

int service_no_output(const aegis_host_api_v1_t* host_api, uint32_t domain, uint32_t op) {
    if (host_api->service_call == nullptr) {
        return -1;
    }
    return host_api->service_call(host_api->user_data, domain, op, nullptr, 0, nullptr, nullptr);
}

void configure_softkey(aegis_ui_softkey_v1_t& softkey,
                       uint32_t role,
                       const char* label,
                       const char* command_id,
                       bool visible,
                       bool enabled) {
    memset(&softkey, 0, sizeof(softkey));
    softkey.visible = visible ? 1u : 0u;
    softkey.enabled = enabled ? 1u : 0u;
    softkey.role = role;
    copy_text(softkey.label, sizeof(softkey.label), label);
    copy_text(softkey.command_id, sizeof(softkey.command_id), command_id);
}

void declare_foreground_page(const aegis_host_api_v1_t* host_api, int ping_count) {
    aegis_ui_foreground_page_v1_t page {};
    char token[AEGIS_UI_PAGE_STATE_TOKEN_MAX_V1];
    char context[AEGIS_UI_PAGE_CONTEXT_MAX_V1];

    copy_text(page.page_id, sizeof(page.page_id), "demo_info.main");
    copy_text(page.title, sizeof(page.title), "Demo Info");
    {
        size_t used = 0;
        token[0] = '\0';
        append_text(token, sizeof(token), used, "route-");
        append_uint(token, sizeof(token), used, static_cast<uint32_t>(ping_count));
    }
    {
        size_t used = 0;
        context[0] = '\0';
        append_text(context, sizeof(context), used, "Ping count ");
        append_uint(context, sizeof(context), used, static_cast<uint32_t>(ping_count));
    }
    copy_text(page.page_state_token, sizeof(page.page_state_token), token);
    copy_text(page.context, sizeof(page.context), context);

    configure_softkey(page.softkeys[0],
                      AEGIS_UI_SOFTKEY_ROLE_PRIMARY,
                      "Ping",
                      "ping",
                      true,
                      true);
    configure_softkey(page.softkeys[1],
                      AEGIS_UI_SOFTKEY_ROLE_CONFIRM,
                      "Done",
                      "done",
                      true,
                      true);
    configure_softkey(page.softkeys[2],
                      AEGIS_UI_SOFTKEY_ROLE_BACK,
                      "",
                      "",
                      false,
                      false);

    host_api->service_call(host_api->user_data,
                           AEGIS_SERVICE_DOMAIN_UI,
                           AEGIS_UI_SERVICE_OP_SET_FOREGROUND_PAGE,
                           &page,
                           sizeof(page),
                           nullptr,
                           nullptr);
}

const char* routed_action_name(uint32_t action) {
    switch (action) {
        case AEGIS_UI_ROUTED_ACTION_MOVE_NEXT:
            return "move_next";
        case AEGIS_UI_ROUTED_ACTION_MOVE_PREVIOUS:
            return "move_previous";
        case AEGIS_UI_ROUTED_ACTION_PRIMARY:
            return "primary";
        case AEGIS_UI_ROUTED_ACTION_SELECT:
            return "select";
        case AEGIS_UI_ROUTED_ACTION_BACK:
            return "back";
        case AEGIS_UI_ROUTED_ACTION_OPEN_MENU:
            return "open_menu";
        case AEGIS_UI_ROUTED_ACTION_OPEN_FILES:
            return "open_files";
        case AEGIS_UI_ROUTED_ACTION_OPEN_SETTINGS:
            return "open_settings";
        case AEGIS_UI_ROUTED_ACTION_OPEN_NOTIFICATIONS:
            return "open_notifications";
        default:
            return "none";
    }
}

const char* event_source_name(uint32_t source) {
    switch (source) {
        case AEGIS_UI_EVENT_SOURCE_SOFTKEY_LEFT:
            return "softkey_left";
        case AEGIS_UI_EVENT_SOURCE_SOFTKEY_CENTER:
            return "softkey_center";
        case AEGIS_UI_EVENT_SOURCE_SOFTKEY_RIGHT:
            return "softkey_right";
        case AEGIS_UI_EVENT_SOURCE_PHYSICAL:
            return "physical";
        default:
            return "unknown";
    }
}

void log_routed_event(const aegis_host_api_v1_t* host_api, const aegis_ui_routed_event_v1_t& event) {
    char message[256];
    size_t used = 0;
    message[0] = '\0';

    append_text(message, sizeof(message), used, "routed_event:type=");
    append_uint(message, sizeof(message), used, event.event_type);
    append_text(message, sizeof(message), used, ", source=");
    append_text(message, sizeof(message), used, event_source_name(event.event_source));
    append_text(message, sizeof(message), used, ", action=");
    append_text(message, sizeof(message), used, routed_action_name(event.routed_action));
    append_text(message, sizeof(message), used, ", page=");
    append_text(message, sizeof(message), used, event.page_id);
    append_text(message, sizeof(message), used, ", token=");
    append_text(message, sizeof(message), used, event.page_state_token);
    append_text(message, sizeof(message), used, ", command=");
    append_text(message, sizeof(message), used, event.command_id);
    log_line(host_api, message);
}

int poll_routed_event(const aegis_host_api_v1_t* host_api, aegis_ui_routed_event_v1_t& event) {
    if (host_api->service_call == nullptr) {
        return -1;
    }

    size_t output_size = sizeof(event);
    return host_api->service_call(host_api->user_data,
                                  AEGIS_SERVICE_DOMAIN_UI,
                                  AEGIS_UI_SERVICE_OP_POLL_EVENT,
                                  nullptr,
                                  0,
                                  &event,
                                  &output_size);
}

void spin_delay(uint32_t rounds) {
    volatile uint32_t sink = 0;
    for (uint32_t index = 0; index < rounds; ++index) {
        sink += index;
    }
    (void)sink;
}

void log_capabilities(const aegis_host_api_v1_t* host_api) {
    char message[320];
    size_t used = 0;
    message[0] = '\0';

    for (uint32_t id = 0; id <= 11; ++id) {
        const int level = host_api->get_capability_level != nullptr
                              ? host_api->get_capability_level(host_api->user_data, id)
                              : -1;
        if (level < 0) {
            continue;
        }

        append_text(message, sizeof(message), used, capability_name(id));
        append_char(message, sizeof(message), used, '=');
        append_text(message, sizeof(message), used, capability_level_name(level));
        append_text(message, sizeof(message), used, "; ");
    }

    log_line(host_api, message);
}

void log_status_with_backend(const aegis_host_api_v1_t* host_api,
                             const char* label,
                             int status,
                             const char* backend_name) {
    char message[192];
    size_t used = 0;
    message[0] = '\0';

    append_text(message, sizeof(message), used, label);
    append_text(message, sizeof(message), used, ":status=");
    append_int(message, sizeof(message), used, status);
    append_text(message, sizeof(message), used, ", backend=");
    append_text(message, sizeof(message), used, backend_name);

    log_line(host_api, message);
}

}  // namespace

extern "C" aegis_app_run_result_v1_t demo_info_main(const aegis_host_api_v1_t* host_api) {
    if (host_api == nullptr || host_api->abi_version != AEGIS_HOST_API_ABI_V1) {
        return AEGIS_APP_RUN_RESULT_FAILED;
    }

    log_line(host_api, "enter ABI capability inspection app");

    if (host_api->ui_create_root != nullptr) {
        host_api->ui_create_root(host_api->user_data, kRootName);
    }

    void* scratch = nullptr;
    if (host_api->mem_alloc != nullptr) {
        scratch = host_api->mem_alloc(host_api->user_data, 96u, alignof(max_align_t));
    }

    int timer_id = 0;
    if (host_api->timer_create != nullptr) {
        host_api->timer_create(host_api->user_data, 1000u, 0, &timer_id);
    }

    log_capabilities(host_api);

    aegis_radio_status_v1_t radio {};
    const int radio_status = service_status_only(host_api,
                                                 AEGIS_SERVICE_DOMAIN_RADIO,
                                                 AEGIS_RADIO_SERVICE_OP_GET_STATUS,
                                                 &radio,
                                                 sizeof(radio));
    log_status_with_backend(host_api, "radio", radio_status, radio.backend_name);

    aegis_gps_status_v1_t gps {};
    const int gps_status = service_status_only(host_api,
                                               AEGIS_SERVICE_DOMAIN_GPS,
                                               AEGIS_GPS_SERVICE_OP_GET_STATUS,
                                               &gps,
                                               sizeof(gps));
    log_status_with_backend(host_api, "gps", gps_status, gps.backend_name);

    aegis_audio_status_v1_t audio {};
    const int audio_status = service_status_only(host_api,
                                                 AEGIS_SERVICE_DOMAIN_AUDIO,
                                                 AEGIS_AUDIO_SERVICE_OP_GET_STATUS,
                                                 &audio,
                                                 sizeof(audio));
    log_status_with_backend(host_api, "audio", audio_status, audio.backend_name);

    aegis_ui_display_info_v1_t display {};
    service_status_only(host_api,
                        AEGIS_SERVICE_DOMAIN_UI,
                        AEGIS_UI_SERVICE_OP_GET_DISPLAY_INFO,
                        &display,
                        sizeof(display));
    log_line(host_api, display.surface_description);

    aegis_storage_status_v1_t storage {};
    service_status_only(host_api,
                        AEGIS_SERVICE_DOMAIN_STORAGE,
                        AEGIS_STORAGE_SERVICE_OP_GET_STATUS,
                        &storage,
                        sizeof(storage));
    log_line(host_api, storage.backend_name);

    aegis_power_status_v1_t power {};
    service_status_only(host_api,
                        AEGIS_SERVICE_DOMAIN_POWER,
                        AEGIS_POWER_SERVICE_OP_GET_STATUS,
                        &power,
                        sizeof(power));
    log_line(host_api, power.status_text);

    aegis_time_status_v1_t time {};
    service_status_only(host_api,
                        AEGIS_SERVICE_DOMAIN_TIME,
                        AEGIS_TIME_SERVICE_OP_GET_STATUS,
                        &time,
                        sizeof(time));
    log_line(host_api, time.source_name);

    aegis_hostlink_status_v1_t hostlink {};
    const int hostlink_status = service_status_only(host_api,
                                                    AEGIS_SERVICE_DOMAIN_HOSTLINK,
                                                    AEGIS_HOSTLINK_SERVICE_OP_GET_STATUS,
                                                    &hostlink,
                                                    sizeof(hostlink));
    {
        char message[224];
        size_t used = 0;
        message[0] = '\0';
        append_text(message, sizeof(message), used, "hostlink:status=");
        append_int(message, sizeof(message), used, hostlink_status);
        append_text(message, sizeof(message), used, ", transport=");
        append_text(message, sizeof(message), used, hostlink.transport_name);
        append_text(message, sizeof(message), used, ", bridge=");
        append_text(message, sizeof(message), used, hostlink.bridge_name);
        log_line(host_api, message);
    }

    aegis_text_input_state_v1_t text_input {};
    service_status_only(host_api,
                        AEGIS_SERVICE_DOMAIN_TEXT_INPUT,
                        AEGIS_TEXT_INPUT_SERVICE_OP_GET_STATE,
                        &text_input,
                        sizeof(text_input));
    {
        char message[224];
        size_t used = 0;
        message[0] = '\0';
        append_text(message, sizeof(message), used, "text_input:src=");
        append_text(message, sizeof(message), used, text_input.source_name);
        append_text(message, sizeof(message), used, ", last=");
        append_text(message, sizeof(message), used, text_input.last_text);
        append_text(message, sizeof(message), used, ", modifiers=");
        append_uint(message, sizeof(message), used, text_input.modifier_mask);
        log_line(host_api, message);
    }

    aegis_text_input_focus_state_v1_t focus {};
    service_status_only(host_api,
                        AEGIS_SERVICE_DOMAIN_TEXT_INPUT,
                        AEGIS_TEXT_INPUT_SERVICE_OP_GET_FOCUS_STATE,
                        &focus,
                        sizeof(focus));
    {
        char message[256];
        size_t used = 0;
        message[0] = '\0';
        append_text(message, sizeof(message), used, "text_focus:owner=");
        append_text(message, sizeof(message), used, focus_owner_name(focus.owner));
        append_text(message, sizeof(message), used, ", route=");
        append_text(message, sizeof(message), used, focus.route_name);
        append_text(message, sizeof(message), used, ", session=");
        append_text(message, sizeof(message), used, focus.owner_session_id);
        log_line(host_api, message);
    }

    const int release_status = service_no_output(host_api,
                                                 AEGIS_SERVICE_DOMAIN_TEXT_INPUT,
                                                 AEGIS_TEXT_INPUT_SERVICE_OP_RELEASE_FOCUS);
    service_status_only(host_api,
                        AEGIS_SERVICE_DOMAIN_TEXT_INPUT,
                        AEGIS_TEXT_INPUT_SERVICE_OP_GET_FOCUS_STATE,
                        &focus,
                        sizeof(focus));
    {
        char message[224];
        size_t used = 0;
        message[0] = '\0';
        append_text(message, sizeof(message), used, "text_focus_after_release:status=");
        append_int(message, sizeof(message), used, release_status);
        append_text(message, sizeof(message), used, ", owner=");
        append_text(message, sizeof(message), used, focus_owner_name(focus.owner));
        append_text(message, sizeof(message), used, ", route=");
        append_text(message, sizeof(message), used, focus.route_name);
        log_line(host_api, message);
    }

    const int request_status = service_no_output(host_api,
                                                 AEGIS_SERVICE_DOMAIN_TEXT_INPUT,
                                                 AEGIS_TEXT_INPUT_SERVICE_OP_REQUEST_FOCUS);
    service_status_only(host_api,
                        AEGIS_SERVICE_DOMAIN_TEXT_INPUT,
                        AEGIS_TEXT_INPUT_SERVICE_OP_GET_FOCUS_STATE,
                        &focus,
                        sizeof(focus));
    {
        char message[256];
        size_t used = 0;
        message[0] = '\0';
        append_text(message, sizeof(message), used, "text_focus_after_request:status=");
        append_int(message, sizeof(message), used, request_status);
        append_text(message, sizeof(message), used, ", owner=");
        append_text(message, sizeof(message), used, focus_owner_name(focus.owner));
        append_text(message, sizeof(message), used, ", route=");
        append_text(message, sizeof(message), used, focus.route_name);
        append_text(message, sizeof(message), used, ", session=");
        append_text(message, sizeof(message), used, focus.owner_session_id);
        log_line(host_api, message);
    }

    if (host_api->service_call != nullptr) {
        aegis_settings_set_request_v1_t settings_request {};
        copy_text(settings_request.key, sizeof(settings_request.key), "demo_info_abi.last_seen");
        copy_text(settings_request.value, sizeof(settings_request.value), "ok");
        host_api->service_call(host_api->user_data,
                               AEGIS_SERVICE_DOMAIN_SETTINGS,
                               AEGIS_SETTINGS_SERVICE_OP_SET,
                               &settings_request,
                               sizeof(settings_request),
                               nullptr,
                               nullptr);

        aegis_notification_publish_request_v1_t notification_request {};
        copy_text(notification_request.title,
                  sizeof(notification_request.title),
                  "Demo Info ABI");
        copy_text(notification_request.body,
                  sizeof(notification_request.body),
                  "Host API ABI path exercised");
        host_api->service_call(host_api->user_data,
                               AEGIS_SERVICE_DOMAIN_NOTIFICATION,
                               AEGIS_NOTIFICATION_SERVICE_OP_PUBLISH,
                               &notification_request,
                               sizeof(notification_request),
                               nullptr,
                               nullptr);
    }

    declare_foreground_page(host_api, 0);
    log_line(host_api, "foreground page declared for routed event test");

    int ping_count = 0;
    bool exit_requested = false;
    uint32_t idle_slices = 0;
    while (!exit_requested && idle_slices < 600u) {
        aegis_ui_routed_event_v1_t event {};
        const int poll_status = poll_routed_event(host_api, event);
        if (poll_status != AEGIS_HOST_STATUS_OK) {
            char message[128];
            size_t used = 0;
            message[0] = '\0';
            append_text(message, sizeof(message), used, "ui.poll_event status=");
            append_int(message, sizeof(message), used, poll_status);
            log_line(host_api, message);
            break;
        }

        if (event.event_type == AEGIS_UI_EVENT_TYPE_NONE) {
            ++idle_slices;
            spin_delay(1500000u);
            continue;
        }

        idle_slices = 0;
        log_routed_event(host_api, event);
        if (event.event_type == AEGIS_UI_EVENT_TYPE_PAGE_COMMAND) {
            if (strcmp(event.command_id, "ping") == 0) {
                ++ping_count;
                declare_foreground_page(host_api, ping_count);
                continue;
            }
            if (strcmp(event.command_id, "done") == 0) {
                exit_requested = true;
                continue;
            }
        }

        if (event.event_type == AEGIS_UI_EVENT_TYPE_ROUTED_ACTION) {
            if (event.routed_action == AEGIS_UI_ROUTED_ACTION_PRIMARY ||
                event.routed_action == AEGIS_UI_ROUTED_ACTION_SELECT) {
                ++ping_count;
                declare_foreground_page(host_api, ping_count);
                continue;
            }
            if (event.routed_action == AEGIS_UI_ROUTED_ACTION_BACK) {
                exit_requested = true;
                continue;
            }
        }
        spin_delay(1500000u);
    }

    if (host_api->timer_cancel != nullptr) {
        host_api->timer_cancel(host_api->user_data, timer_id);
    }
    if (host_api->ui_destroy_root != nullptr) {
        host_api->ui_destroy_root(host_api->user_data, kRootName);
    }
    if (scratch != nullptr && host_api->mem_free != nullptr) {
        host_api->mem_free(host_api->user_data, scratch);
    }

    return AEGIS_APP_RUN_RESULT_COMPLETED;
}
