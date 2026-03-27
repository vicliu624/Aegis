#include "sdk/include/aegis/app_module_abi.h"
#include "sdk/include/aegis/host_api_abi.h"
#include "sdk/include/aegis/services/ui_service_abi.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

namespace {

constexpr const char* kTag = "demo_hello";
constexpr const char* kRootName = "demo_hello.root";

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

void copy_text(char* destination, size_t capacity, const char* source) {
    size_t used = 0;
    if (capacity == 0) {
        return;
    }
    destination[0] = '\0';
    append_text(destination, capacity, used, source);
}

int log_line(const aegis_host_api_v1_t* host_api, const char* message) {
    if (host_api == nullptr || host_api->log_write == nullptr) {
        return 0;
    }
    return host_api->log_write(host_api->user_data, kTag, message);
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

int declare_page(const aegis_host_api_v1_t* host_api, uint32_t ping_count) {
    if (host_api == nullptr || host_api->service_call == nullptr) {
        return AEGIS_HOST_STATUS_INVALID_ARGUMENT;
    }

    aegis_ui_foreground_page_v1_t page {};
    char token[AEGIS_UI_PAGE_STATE_TOKEN_MAX_V1];
    char context[AEGIS_UI_PAGE_CONTEXT_MAX_V1];
    size_t used = 0;

    copy_text(page.page_id, sizeof(page.page_id), "demo_hello.main");
    copy_text(page.title, sizeof(page.title), "Demo Hello");

    token[0] = '\0';
    append_text(token, sizeof(token), used, "hello-");
    append_uint(token, sizeof(token), used, ping_count);
    copy_text(page.page_state_token, sizeof(page.page_state_token), token);

    used = 0;
    context[0] = '\0';
    append_text(context, sizeof(context), used, "Ready ");
    append_uint(context, sizeof(context), used, ping_count);
    copy_text(page.context, sizeof(page.context), context);

    configure_softkey(page.softkeys[0],
                      AEGIS_UI_SOFTKEY_ROLE_PRIMARY,
                      "Ping",
                      "ping",
                      true,
                      true);
    configure_softkey(page.softkeys[1],
                      AEGIS_UI_SOFTKEY_ROLE_CONFIRM,
                      "Exit",
                      "exit",
                      true,
                      true);
    configure_softkey(page.softkeys[2],
                      AEGIS_UI_SOFTKEY_ROLE_BACK,
                      "",
                      "",
                      false,
                      false);

    return host_api->service_call(host_api->user_data,
                                  AEGIS_SERVICE_DOMAIN_UI,
                                  AEGIS_UI_SERVICE_OP_SET_FOREGROUND_PAGE,
                                  &page,
                                  sizeof(page),
                                  nullptr,
                                  nullptr);
}

int poll_event(const aegis_host_api_v1_t* host_api, aegis_ui_routed_event_v1_t& event) {
    if (host_api == nullptr || host_api->service_call == nullptr) {
        return AEGIS_HOST_STATUS_INVALID_ARGUMENT;
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

void log_event(const aegis_host_api_v1_t* host_api, const aegis_ui_routed_event_v1_t& event) {
    char message[192];
    size_t used = 0;
    message[0] = '\0';
    append_text(message, sizeof(message), used, "event=");
    append_uint(message, sizeof(message), used, event.event_type);
    append_text(message, sizeof(message), used, ",src=");
    append_uint(message, sizeof(message), used, event.event_source);
    append_text(message, sizeof(message), used, ",action=");
    append_uint(message, sizeof(message), used, event.routed_action);
    append_text(message, sizeof(message), used, ",cmd=");
    append_text(message, sizeof(message), used, event.command_id);
    append_text(message, sizeof(message), used, ",token=");
    append_text(message, sizeof(message), used, event.page_state_token);
    log_line(host_api, message);
}

}  // namespace

extern "C" aegis_app_run_result_v1_t demo_hello_main(const aegis_host_api_v1_t* host_api) {
    if (host_api == nullptr || host_api->abi_version != AEGIS_HOST_API_ABI_V1) {
        return AEGIS_APP_RUN_RESULT_FAILED;
    }

    log_line(host_api, "enter demo hello routed-event app");
    if (host_api->ui_create_root != nullptr) {
        host_api->ui_create_root(host_api->user_data, kRootName);
    }

    if (declare_page(host_api, 0) != AEGIS_HOST_STATUS_OK) {
        log_line(host_api, "foreground page declaration failed");
    } else {
        log_line(host_api, "foreground page declared");
    }

    uint32_t ping_count = 0;
    uint32_t idle_slices = 0;
    bool exit_requested = false;
    while (!exit_requested && idle_slices < 600u) {
        aegis_ui_routed_event_v1_t event {};
        const int status = poll_event(host_api, event);
        if (status != AEGIS_HOST_STATUS_OK) {
            log_line(host_api, "poll_event failed");
            break;
        }

        if (event.event_type == AEGIS_UI_EVENT_TYPE_NONE) {
            ++idle_slices;
            spin_delay(1500000u);
            continue;
        }

        idle_slices = 0;
        log_event(host_api, event);

        if (event.event_type == AEGIS_UI_EVENT_TYPE_PAGE_COMMAND) {
            if (strcmp(event.command_id, "ping") == 0) {
                ++ping_count;
                declare_page(host_api, ping_count);
                continue;
            }
            if (strcmp(event.command_id, "exit") == 0) {
                exit_requested = true;
                continue;
            }
        }

        if (event.event_type == AEGIS_UI_EVENT_TYPE_ROUTED_ACTION) {
            if (event.routed_action == AEGIS_UI_ROUTED_ACTION_PRIMARY ||
                event.routed_action == AEGIS_UI_ROUTED_ACTION_SELECT) {
                ++ping_count;
                declare_page(host_api, ping_count);
                continue;
            }
            if (event.routed_action == AEGIS_UI_ROUTED_ACTION_BACK) {
                exit_requested = true;
                continue;
            }
        }
    }

    log_line(host_api, exit_requested ? "exit requested" : "idle timeout");
    if (host_api->ui_destroy_root != nullptr) {
        host_api->ui_destroy_root(host_api->user_data, kRootName);
    }
    log_line(host_api, "returning control to shell");
    return AEGIS_APP_RUN_RESULT_COMPLETED;
}
