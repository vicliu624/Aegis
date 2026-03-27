#include "sdk/include/aegis/app_module_abi.h"
#include "sdk/include/aegis/host_api_abi.h"
#include "sdk/include/aegis/services/settings_service_abi.h"
#include "sdk/include/aegis/services/ui_service_abi.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

namespace {

constexpr const char* kRootName = "settings.root";
constexpr size_t kEntryCount = 7;

struct SettingsEntry {
    size_t committed_index;
    size_t draft_index;
};

struct SettingsModel {
    SettingsEntry entries[kEntryCount];
    size_t focus_index;
    uint32_t generation;
};

struct SettingsScratch {
    aegis_ui_foreground_page_v1_t page {};
    char token[AEGIS_UI_PAGE_STATE_TOKEN_MAX_V1] {};
};

SettingsScratch g_settings_scratch {};

SettingsScratch& scratch() {
    return g_settings_scratch;
}

void append_char(char* buffer, size_t capacity, size_t& used, char value) {
    if (buffer == nullptr || capacity == 0 || used + 1 >= capacity) {
        return;
    }
    buffer[used++] = value;
    buffer[used] = '\0';
}

void append_text(char* buffer, size_t capacity, size_t& used, const char* text) {
    if (buffer == nullptr || text == nullptr || capacity == 0 || used >= capacity) {
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
    if (destination == nullptr || capacity == 0) {
        return;
    }
    size_t used = 0;
    destination[0] = '\0';
    append_text(destination, capacity, used, source);
}

bool text_equals(const char* left, const char* right) {
    if (left == nullptr || right == nullptr) {
        return false;
    }
    while (*left != '\0' && *right != '\0') {
        if (*left != *right) {
            return false;
        }
        ++left;
        ++right;
    }
    return *left == '\0' && *right == '\0';
}

void write_root_name(char* destination, size_t capacity) {
    if (destination == nullptr || capacity < 14) {
        return;
    }
    volatile char* out = destination;
    out[0] = 's';
    out[1] = 'e';
    out[2] = 't';
    out[3] = 't';
    out[4] = 'i';
    out[5] = 'n';
    out[6] = 'g';
    out[7] = 's';
    out[8] = '.';
    out[9] = 'r';
    out[10] = 'o';
    out[11] = 'o';
    out[12] = 't';
    out[13] = '\0';
}

size_t option_count_for_entry(size_t entry_index) {
    if (entry_index == 0) {
        return 5;
    }
    if (entry_index == 1) {
        return 2;
    }
    if (entry_index == 2) {
        return 6;
    }
    if (entry_index == 3) {
        return 2;
    }
    if (entry_index == 4) {
        return 2;
    }
    if (entry_index == 5) {
        return 6;
    }
    if (entry_index == 6) {
        return 2;
    }
    return 0;
}

void write_entry_id(size_t entry_index, char* destination, size_t capacity) {
    if (destination == nullptr || capacity == 0) {
        return;
    }
    if (entry_index == 0) {
        copy_text(destination, capacity, "screen_brightness");
        return;
    }
    if (entry_index == 1) {
        copy_text(destination, capacity, "keyboard_backlight");
        return;
    }
    if (entry_index == 2) {
        copy_text(destination, capacity, "screen_timeout");
        return;
    }
    if (entry_index == 3) {
        copy_text(destination, capacity, "bluetooth");
        return;
    }
    if (entry_index == 4) {
        copy_text(destination, capacity, "gps");
        return;
    }
    if (entry_index == 5) {
        copy_text(destination, capacity, "timezone");
        return;
    }
    if (entry_index == 6) {
        copy_text(destination, capacity, "time_format");
        return;
    }
    destination[0] = '\0';
}

void write_entry_label(size_t entry_index, char* destination, size_t capacity) {
    if (destination == nullptr || capacity == 0) {
        return;
    }
    if (entry_index == 0) {
        copy_text(destination, capacity, "Screen Brightness");
        return;
    }
    if (entry_index == 1) {
        copy_text(destination, capacity, "Keyboard Backlight");
        return;
    }
    if (entry_index == 2) {
        copy_text(destination, capacity, "Screen Timeout");
        return;
    }
    if (entry_index == 3) {
        copy_text(destination, capacity, "Bluetooth");
        return;
    }
    if (entry_index == 4) {
        copy_text(destination, capacity, "GPS");
        return;
    }
    if (entry_index == 5) {
        copy_text(destination, capacity, "Time Zone");
        return;
    }
    if (entry_index == 6) {
        copy_text(destination, capacity, "Time Format");
        return;
    }
    destination[0] = '\0';
}

void write_option_text(size_t entry_index,
                       size_t option_index,
                       char* destination,
                       size_t capacity) {
    if (destination == nullptr || capacity == 0) {
        return;
    }
    destination[0] = '\0';
    if (entry_index == 0) {
        if (option_index == 0) {
            copy_text(destination, capacity, "20%");
            return;
        }
        if (option_index == 1) {
            copy_text(destination, capacity, "40%");
            return;
        }
        if (option_index == 2) {
            copy_text(destination, capacity, "60%");
            return;
        }
        if (option_index == 3) {
            copy_text(destination, capacity, "80%");
            return;
        }
        if (option_index == 4) {
            copy_text(destination, capacity, "100%");
            return;
        }
        return;
    }

    if (entry_index == 1 || entry_index == 3 || entry_index == 4) {
        if (option_index == 0) {
            copy_text(destination, capacity, "Off");
            return;
        }
        if (option_index == 1) {
            copy_text(destination, capacity, "On");
            return;
        }
        return;
    }

    if (entry_index == 2) {
        if (option_index == 0) {
            copy_text(destination, capacity, "15 sec");
            return;
        }
        if (option_index == 1) {
            copy_text(destination, capacity, "30 sec");
            return;
        }
        if (option_index == 2) {
            copy_text(destination, capacity, "1 min");
            return;
        }
        if (option_index == 3) {
            copy_text(destination, capacity, "2 min");
            return;
        }
        if (option_index == 4) {
            copy_text(destination, capacity, "5 min");
            return;
        }
        if (option_index == 5) {
            copy_text(destination, capacity, "Never");
            return;
        }
        return;
    }

    if (entry_index == 5) {
        if (option_index == 0) {
            copy_text(destination, capacity, "UTC-08:00");
            return;
        }
        if (option_index == 1) {
            copy_text(destination, capacity, "UTC");
            return;
        }
        if (option_index == 2) {
            copy_text(destination, capacity, "Asia/Shanghai");
            return;
        }
        if (option_index == 3) {
            copy_text(destination, capacity, "Asia/Tokyo");
            return;
        }
        if (option_index == 4) {
            copy_text(destination, capacity, "Europe/Berlin");
            return;
        }
        if (option_index == 5) {
            copy_text(destination, capacity, "America/New_York");
            return;
        }
        return;
    }

    if (entry_index == 6) {
        if (option_index == 0) {
            copy_text(destination, capacity, "12-hour");
            return;
        }
        if (option_index == 1) {
            copy_text(destination, capacity, "24-hour");
            return;
        }
    }
}

bool option_matches(size_t entry_index, size_t option_index, const char* value) {
    char option[AEGIS_SETTINGS_VALUE_MAX_V1] {};
    write_option_text(entry_index, option_index, option, sizeof(option));
    return text_equals(value, option);
}

void make_settings_key(size_t entry_index, char* key, size_t capacity) {
    size_t used = 0;
    if (key == nullptr || capacity == 0) {
        return;
    }
    key[0] = '\0';
    append_text(key, capacity, used, "builtin.settings.");
    char id[AEGIS_UI_COMMAND_ID_MAX_V1] {};
    write_entry_id(entry_index, id, sizeof(id));
    append_text(key, capacity, used, id);
}

bool settings_get(const aegis_host_api_v1_t* host_api,
                  const char* key,
                  char* value,
                  size_t value_capacity) {
    if (host_api == nullptr || host_api->service_call == nullptr || value == nullptr ||
        value_capacity == 0) {
        return false;
    }

    aegis_settings_get_request_v1_t request {};
    aegis_settings_get_response_v1_t response {};
    size_t output_size = sizeof(response);
    copy_text(request.key, sizeof(request.key), key);
    const int rc = host_api->service_call(host_api->user_data,
                                          AEGIS_SERVICE_DOMAIN_SETTINGS,
                                          AEGIS_SETTINGS_SERVICE_OP_GET,
                                          &request,
                                          sizeof(request),
                                          &response,
                                          &output_size);
    if (rc != AEGIS_HOST_STATUS_OK || response.found == 0u) {
        return false;
    }
    copy_text(value, value_capacity, response.value);
    return true;
}

bool settings_set(const aegis_host_api_v1_t* host_api, const char* key, const char* value) {
    if (host_api == nullptr || host_api->service_call == nullptr) {
        return false;
    }

    aegis_settings_set_request_v1_t request {};
    copy_text(request.key, sizeof(request.key), key);
    copy_text(request.value, sizeof(request.value), value);
    return host_api->service_call(host_api->user_data,
                                  AEGIS_SERVICE_DOMAIN_SETTINGS,
                                  AEGIS_SETTINGS_SERVICE_OP_SET,
                                  &request,
                                  sizeof(request),
                                  nullptr,
                                  nullptr) == AEGIS_HOST_STATUS_OK;
}

void initialize_model(SettingsModel& model) {
    model.entries[0] = {3, 3};
    model.entries[1] = {1, 1};
    model.entries[2] = {2, 2};
    model.entries[3] = {0, 0};
    model.entries[4] = {1, 1};
    model.entries[5] = {2, 2};
    model.entries[6] = {1, 1};
    model.focus_index = 0;
    model.generation = 1;
}

void load_settings(const aegis_host_api_v1_t* host_api, SettingsModel& model) {
    char key[AEGIS_SETTINGS_KEY_MAX_V1] {};
    char value[AEGIS_SETTINGS_VALUE_MAX_V1] {};
    for (size_t entry_index = 0; entry_index < kEntryCount; ++entry_index) {
        auto& entry = model.entries[entry_index];
        make_settings_key(entry_index, key, sizeof(key));
        if (!settings_get(host_api, key, value, sizeof(value))) {
            continue;
        }
        for (size_t option_index = 0; option_index < option_count_for_entry(entry_index); ++option_index) {
            if (option_matches(entry_index, option_index, value)) {
                entry.committed_index = option_index;
                entry.draft_index = option_index;
                break;
            }
        }
    }
}

size_t dirty_count(const SettingsModel& model) {
    size_t count = 0;
    for (size_t index = 0; index < kEntryCount; ++index) {
        if (model.entries[index].draft_index != model.entries[index].committed_index) {
            ++count;
        }
    }
    return count;
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

void declare_page(const aegis_host_api_v1_t* host_api, const SettingsModel& model) {
    if (host_api == nullptr || host_api->service_call == nullptr) {
        return;
    }

    auto& page = scratch().page;
    memset(&page, 0, sizeof(page));
    copy_text(page.page_id, sizeof(page.page_id), "settings.main");
    copy_text(page.title, sizeof(page.title), "Settings");

    auto& token = scratch().token;
    size_t used = 0;
    append_text(token, sizeof(token), used, "s:");
    append_uint(token, sizeof(token), used, static_cast<uint32_t>(model.focus_index));
    append_char(token, sizeof(token), used, ':');
    append_uint(token, sizeof(token), used, static_cast<uint32_t>(dirty_count(model)));
    append_char(token, sizeof(token), used, ':');
    append_uint(token, sizeof(token), used, model.generation % 1000u);
    copy_text(page.page_state_token, sizeof(page.page_state_token), token);

    page.layout_template = AEGIS_UI_PAGE_TEMPLATE_VALUE_LIST;
    page.item_count = static_cast<uint32_t>(kEntryCount);

    configure_softkey(page.softkeys[0],
                      AEGIS_UI_SOFTKEY_ROLE_PRIMARY,
                      "Apply",
                      "apply",
                      true,
                      dirty_count(model) != 0);
    configure_softkey(page.softkeys[1],
                      AEGIS_UI_SOFTKEY_ROLE_CONFIRM,
                      "Select",
                      "select",
                      true,
                      true);
    configure_softkey(page.softkeys[2],
                      AEGIS_UI_SOFTKEY_ROLE_BACK,
                      "",
                      "",
                      false,
                      false);

    for (size_t index = 0; index < kEntryCount; ++index) {
        auto& item = page.items[index];
        const auto& entry = model.entries[index];
        char item_id[AEGIS_UI_COMMAND_ID_MAX_V1] {};
        char label[AEGIS_UI_PAGE_ITEM_LABEL_MAX_V1] {};
        char detail[AEGIS_UI_PAGE_ITEM_DETAIL_MAX_V1] {};
        memset(&item, 0, sizeof(item));
        item.focused = index == model.focus_index ? 1u : 0u;
        item.emphasized = item.focused;
        item.warning = entry.draft_index != entry.committed_index ? 1u : 0u;
        write_entry_id(index, item_id, sizeof(item_id));
        write_entry_label(index, label, sizeof(label));
        write_option_text(index, entry.draft_index, detail, sizeof(detail));
        copy_text(item.item_id, sizeof(item.item_id), item_id);
        copy_text(item.label, sizeof(item.label), label);
        copy_text(item.detail, sizeof(item.detail), detail);
    }

    host_api->service_call(host_api->user_data,
                           AEGIS_SERVICE_DOMAIN_UI,
                           AEGIS_UI_SERVICE_OP_SET_FOREGROUND_PAGE,
                           &page,
                           sizeof(page),
                           nullptr,
                           nullptr);
}

void focus_next(SettingsModel& model) {
    model.focus_index = (model.focus_index + 1u) % kEntryCount;
    ++model.generation;
}

void focus_previous(SettingsModel& model) {
    model.focus_index = (model.focus_index + kEntryCount - 1u) % kEntryCount;
    ++model.generation;
}

void cycle_focused(SettingsModel& model) {
    auto& entry = model.entries[model.focus_index];
    const size_t option_count = option_count_for_entry(model.focus_index);
    if (option_count == 0) {
        return;
    }
    entry.draft_index = (entry.draft_index + 1u) % option_count;
    ++model.generation;
}

void apply_changes(const aegis_host_api_v1_t* host_api, SettingsModel& model) {
    char key[AEGIS_SETTINGS_KEY_MAX_V1] {};
    bool changed = false;
    for (size_t index = 0; index < kEntryCount; ++index) {
        auto& entry = model.entries[index];
        if (entry.draft_index == entry.committed_index) {
            continue;
        }
        char value[AEGIS_SETTINGS_VALUE_MAX_V1] {};
        make_settings_key(index, key, sizeof(key));
        write_option_text(index, entry.draft_index, value, sizeof(value));
        if (settings_set(host_api, key, value)) {
            entry.committed_index = entry.draft_index;
            changed = true;
        }
    }
    if (changed) {
        ++model.generation;
    }
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

}  // namespace

extern "C" aegis_app_run_result_v1_t settings_main(const aegis_host_api_v1_t* host_api) {
    if (host_api == nullptr || host_api->abi_version != AEGIS_HOST_API_ABI_V1) {
        return AEGIS_APP_RUN_RESULT_FAILED;
    }

    char root_name[16] {};
    write_root_name(root_name, sizeof(root_name));

    if (host_api->ui_create_root != nullptr) {
        host_api->ui_create_root(host_api->user_data, root_name);
    }

    SettingsModel model {};
    initialize_model(model);
    load_settings(host_api, model);
    declare_page(host_api, model);

    bool exit_requested = false;
    while (!exit_requested) {
        aegis_ui_routed_event_v1_t event {};
        const int status = poll_event(host_api, event);
        if (status != AEGIS_HOST_STATUS_OK) {
            if (host_api->ui_destroy_root != nullptr) {
                host_api->ui_destroy_root(host_api->user_data, root_name);
            }
            return AEGIS_APP_RUN_RESULT_FAILED;
        }

        if (event.event_type == AEGIS_UI_EVENT_TYPE_NONE) {
            spin_delay(1500000u);
            continue;
        }

        if (event.event_type == AEGIS_UI_EVENT_TYPE_PAGE_COMMAND) {
            if (text_equals(event.command_id, "apply")) {
                apply_changes(host_api, model);
                declare_page(host_api, model);
                continue;
            }
            if (text_equals(event.command_id, "select")) {
                cycle_focused(model);
                declare_page(host_api, model);
                continue;
            }
        }

        if (event.event_type == AEGIS_UI_EVENT_TYPE_ROUTED_ACTION) {
            if (event.routed_action == AEGIS_UI_ROUTED_ACTION_MOVE_NEXT) {
                focus_next(model);
                declare_page(host_api, model);
                continue;
            }
            if (event.routed_action == AEGIS_UI_ROUTED_ACTION_MOVE_PREVIOUS) {
                focus_previous(model);
                declare_page(host_api, model);
                continue;
            }
            if (event.routed_action == AEGIS_UI_ROUTED_ACTION_SELECT) {
                cycle_focused(model);
                declare_page(host_api, model);
                continue;
            }
            if (event.routed_action == AEGIS_UI_ROUTED_ACTION_PRIMARY) {
                apply_changes(host_api, model);
                declare_page(host_api, model);
                continue;
            }
            if (event.routed_action == AEGIS_UI_ROUTED_ACTION_BACK) {
                exit_requested = true;
                continue;
            }
            continue;
        }
    }

    if (host_api->ui_destroy_root != nullptr) {
        host_api->ui_destroy_root(host_api->user_data, root_name);
    }
    return AEGIS_APP_RUN_RESULT_COMPLETED;
}

LL_EXTENSION_SYMBOL(settings_main);
