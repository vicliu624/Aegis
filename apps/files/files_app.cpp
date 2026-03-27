#include "sdk/include/aegis/app_module_abi.h"
#include "sdk/include/aegis/host_api_abi.h"
#include "sdk/include/aegis/services/storage_service_abi.h"
#include "sdk/include/aegis/services/ui_service_abi.h"

#include <stddef.h>
#include <stdint.h>

namespace {

constexpr size_t kPageVisibleEntries = 8;
struct FileEntry {
    char name[AEGIS_STORAGE_ENTRY_NAME_MAX_V1] {};
    bool directory {false};
    uint32_t size_bytes {0};
};

enum class BrowserState {
    Root,
    Ready,
    NoCard,
    Unavailable,
};

struct BrowserModel {
    BrowserState state {BrowserState::Unavailable};
    char mount_root[AEGIS_STORAGE_PATH_MAX_V1] {"/lfs"};
    char current_path[AEGIS_STORAGE_PATH_MAX_V1] {"/"};
    char status_text[AEGIS_UI_PAGE_CONTEXT_MAX_V1] {"Storage unavailable"};
    FileEntry entries[kPageVisibleEntries] {};
    size_t entry_count {0};
    size_t focus_index {0};
    uint32_t page_offset {0};
    uint32_t next_offset {0};
    bool has_more {false};
    bool info_dialog_visible {false};
    uint32_t generation {0};
    bool storage_available {false};
    bool sd_card_present {false};
};

struct BrowserScratch {
    aegis_ui_foreground_page_v1_t page {};
    aegis_storage_list_directory_response_v1_t list_response {};
};

BrowserScratch g_browser_scratch {};

BrowserScratch& scratch() {
    return g_browser_scratch;
}

void zero_buffer(char* buffer, size_t capacity) {
    if (buffer == nullptr || capacity == 0) {
        return;
    }
    buffer[0] = '\0';
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

void copy_text(char* destination, size_t capacity, const char* source) {
    if (destination == nullptr || capacity == 0) {
        return;
    }
    size_t used = 0;
    destination[0] = '\0';
    append_text(destination, capacity, used, source);
}

void write_root_name(char* destination, size_t capacity) {
    if (destination == nullptr || capacity < 11) {
        return;
    }
    volatile char* out = destination;
    out[0] = static_cast<char>(0x66);
    out[1] = static_cast<char>(0x69);
    out[2] = static_cast<char>(0x6c);
    out[3] = static_cast<char>(0x65);
    out[4] = static_cast<char>(0x73);
    out[5] = static_cast<char>(0x2e);
    out[6] = static_cast<char>(0x72);
    out[7] = static_cast<char>(0x6f);
    out[8] = static_cast<char>(0x6f);
    out[9] = static_cast<char>(0x74);
    out[10] = '\0';
}

void write_virtual_root(char* destination, size_t capacity) {
    if (destination == nullptr || capacity < 2) {
        return;
    }
    volatile char* out = destination;
    out[0] = static_cast<char>(0x2f);
    out[1] = '\0';
}

size_t bounded_length(const char* text, size_t capacity) {
    if (text == nullptr) {
        return 0;
    }
    size_t length = 0;
    while (length < capacity && text[length] != '\0') {
        ++length;
    }
    return length;
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

bool starts_with(const char* text, const char* prefix) {
    if (text == nullptr || prefix == nullptr) {
        return false;
    }
    while (*prefix != '\0') {
        if (*text == '\0' || *text != *prefix) {
            return false;
        }
        ++text;
        ++prefix;
    }
    return true;
}

uint32_t hash_text(const char* text) {
    uint32_t value = 2166136261u;
    if (text == nullptr) {
        return value;
    }
    while (*text != '\0') {
        value ^= static_cast<unsigned char>(*text++);
        value *= 16777619u;
    }
    return value;
}

void format_size(uint32_t size_bytes, char* buffer, size_t capacity) {
    size_t used = 0;
    zero_buffer(buffer, capacity);
    if (size_bytes < 1024u) {
        append_uint(buffer, capacity, used, size_bytes);
        append_text(buffer, capacity, used, " B");
        return;
    }
    if (size_bytes < (1024u * 1024u)) {
        append_uint(buffer, capacity, used, size_bytes / 1024u);
        append_text(buffer, capacity, used, " KB");
        return;
    }
    append_uint(buffer, capacity, used, size_bytes / (1024u * 1024u));
    append_text(buffer, capacity, used, " MB");
}

void spin_delay(uint32_t rounds) {
    volatile uint32_t sink = 0;
    for (uint32_t index = 0; index < rounds; ++index) {
        sink += index;
    }
    (void)sink;
}

bool service_call(const aegis_host_api_v1_t* host_api,
                  uint32_t domain,
                  uint32_t op,
                  const void* input,
                  size_t input_size,
                  void* output,
                  size_t* output_size) {
    if (host_api == nullptr || host_api->service_call == nullptr) {
        return false;
    }
    return host_api->service_call(
               host_api->user_data, domain, op, input, input_size, output, output_size) ==
           AEGIS_HOST_STATUS_OK;
}

void bump_generation(BrowserModel& model) {
    ++model.generation;
}

bool is_virtual_root(const BrowserModel& model) {
    return model.current_path[0] == '/' && model.current_path[1] == '\0';
}

const FileEntry* focused_entry(const BrowserModel& model) {
    if (model.entry_count == 0 || model.focus_index >= model.entry_count) {
        return nullptr;
    }
    return &model.entries[model.focus_index];
}

bool refresh_storage_status(const aegis_host_api_v1_t* host_api, BrowserModel& model) {
    aegis_storage_status_v1_t status {};
    size_t output_size = sizeof(status);
    if (!service_call(host_api,
                      AEGIS_SERVICE_DOMAIN_STORAGE,
                      AEGIS_STORAGE_SERVICE_OP_GET_STATUS,
                      nullptr,
                      0,
                      &status,
                      &output_size)) {
        model.storage_available = false;
        model.sd_card_present = false;
        model.state = BrowserState::Unavailable;
        copy_text(model.status_text, sizeof(model.status_text), "Storage unavailable");
        return false;
    }

    model.storage_available = status.available != 0;
    model.sd_card_present = status.sd_card_present != 0;
    if (status.mount_root[0] == '\0') {
        copy_text(model.mount_root, sizeof(model.mount_root), "/lfs");
    } else {
        copy_text(model.mount_root, sizeof(model.mount_root), status.mount_root);
    }
    return true;
}

void load_virtual_root(BrowserModel& model) {
    write_virtual_root(model.current_path, sizeof(model.current_path));
    write_virtual_root(model.status_text, sizeof(model.status_text));
    model.entry_count = 0;
    model.focus_index = 0;
    model.page_offset = 0;
    model.next_offset = 0;
    model.has_more = false;
    model.info_dialog_visible = false;
    model.state = BrowserState::Root;

    copy_text(model.entries[model.entry_count].name, sizeof(model.entries[model.entry_count].name), "lfs");
    model.entries[model.entry_count].directory = true;
    model.entries[model.entry_count].size_bytes = 0;
    ++model.entry_count;

    if (model.sd_card_present && model.entry_count < kPageVisibleEntries) {
        copy_text(
            model.entries[model.entry_count].name, sizeof(model.entries[model.entry_count].name), "sdcard");
        model.entries[model.entry_count].directory = true;
        model.entries[model.entry_count].size_bytes = 0;
        ++model.entry_count;
    }

    bump_generation(model);
}

void load_unavailable(BrowserModel& model, BrowserState state, const char* status_text) {
    model.entry_count = 0;
    model.focus_index = 0;
    model.page_offset = 0;
    model.next_offset = 0;
    model.has_more = false;
    model.info_dialog_visible = false;
    model.state = state;
    copy_text(model.status_text, sizeof(model.status_text), status_text);
    bump_generation(model);
}

void join_path(const char* base, const char* name, char* output, size_t capacity) {
    size_t used = 0;
    zero_buffer(output, capacity);
    if (text_equals(base, "/")) {
        append_char(output, capacity, used, '/');
        append_text(output, capacity, used, name);
        return;
    }
    append_text(output, capacity, used, base);
    append_char(output, capacity, used, '/');
    append_text(output, capacity, used, name);
}

void parent_path_for(const BrowserModel& model, char* output, size_t capacity) {
    if (text_equals(model.current_path, model.mount_root) || text_equals(model.current_path, "/sdcard")) {
        write_virtual_root(output, capacity);
        return;
    }

    copy_text(output, capacity, model.current_path);
    const size_t length = bounded_length(output, capacity);
    for (size_t index = length; index > 0; --index) {
        if (output[index - 1] == '/') {
            if (index - 1 == 0) {
                write_virtual_root(output, capacity);
            } else {
                output[index - 1] = '\0';
            }
            return;
        }
    }

    write_virtual_root(output, capacity);
}

void load_path(const aegis_host_api_v1_t* host_api,
               BrowserModel& model,
               const char* path,
               uint32_t offset,
               size_t focus_index) {
    refresh_storage_status(host_api, model);
    copy_text(model.current_path, sizeof(model.current_path), path);
    model.entry_count = 0;
    model.focus_index = 0;
    model.page_offset = offset;
    model.next_offset = offset;
    model.has_more = false;
    model.info_dialog_visible = false;

    const bool sd_path =
        text_equals(model.current_path, "/sdcard") || starts_with(model.current_path, "/sdcard/");
    if (sd_path && !model.sd_card_present) {
        load_unavailable(model, BrowserState::NoCard, "No SD card inserted");
        return;
    }
    if (text_equals(model.current_path, model.mount_root) && !model.storage_available) {
        load_unavailable(model, BrowserState::Unavailable, "LittleFS unavailable");
        return;
    }

    aegis_storage_list_directory_request_v1_t list_request {};
    list_request.offset = offset;
    list_request.limit = static_cast<uint32_t>(kPageVisibleEntries);
    copy_text(list_request.path, sizeof(list_request.path), model.current_path);

    auto& list_response = scratch().list_response;
    list_response = {};
    size_t output_size = sizeof(list_response);
    if (!service_call(host_api,
                      AEGIS_SERVICE_DOMAIN_STORAGE,
                      AEGIS_STORAGE_SERVICE_OP_LIST_DIRECTORY,
                      &list_request,
                      sizeof(list_request),
                      &list_response,
                      &output_size)) {
        load_unavailable(model, BrowserState::Unavailable, "Directory query failed");
        return;
    }

    if (list_response.status == AEGIS_STORAGE_LIST_STATUS_NO_CARD) {
        load_unavailable(model, BrowserState::NoCard, "No SD card inserted");
        return;
    }
    if (list_response.status == AEGIS_STORAGE_LIST_STATUS_UNAVAILABLE) {
        load_unavailable(model, BrowserState::Unavailable, "Storage unavailable");
        return;
    }
    if (list_response.status == AEGIS_STORAGE_LIST_STATUS_NOT_FOUND) {
        load_unavailable(model, BrowserState::Unavailable, "Folder unavailable");
        return;
    }

    for (uint32_t index = 0;
         index < list_response.entry_count && model.entry_count < kPageVisibleEntries;
         ++index) {
        auto& target = model.entries[model.entry_count++];
        copy_text(target.name,
                  sizeof(target.name),
                  list_response.entries[index].name);
        target.directory = list_response.entries[index].directory != 0;
        target.size_bytes = list_response.entries[index].size_bytes;
    }

    model.state = BrowserState::Ready;
    model.has_more = list_response.has_more != 0;
    model.next_offset = list_response.next_offset;
    if (model.entry_count > 0) {
        model.focus_index = focus_index < model.entry_count ? focus_index : (model.entry_count - 1);
    }
    if (model.entry_count == 0) {
        copy_text(model.status_text, sizeof(model.status_text), "Folder is empty");
    } else {
        copy_text(model.status_text, sizeof(model.status_text), model.current_path);
    }
    bump_generation(model);
}

void configure_softkey(aegis_ui_softkey_v1_t& softkey,
                       uint32_t role,
                       const char* label,
                       const char* command_id,
                       bool visible,
                       bool enabled) {
    softkey = {};
    softkey.visible = visible ? 1u : 0u;
    softkey.enabled = enabled ? 1u : 0u;
    softkey.role = role;
    copy_text(softkey.label, sizeof(softkey.label), label);
    copy_text(softkey.command_id, sizeof(softkey.command_id), command_id);
}

void set_page_item(aegis_ui_page_item_v1_t& item,
                   const char* item_id,
                   const char* label,
                   const char* detail,
                   bool focused,
                   bool emphasized,
                   bool disabled,
                   uint32_t accessory,
                   const char* icon_key) {
    item = {};
    item.focused = focused ? 1u : 0u;
    item.emphasized = emphasized ? 1u : 0u;
    item.disabled = disabled ? 1u : 0u;
    item.accessory = accessory;
    item.icon_source = icon_key != nullptr && icon_key[0] != '\0' ? AEGIS_UI_ITEM_ICON_SOURCE_APP_ASSET
                                                                  : AEGIS_UI_ITEM_ICON_SOURCE_NONE;
    copy_text(item.item_id, sizeof(item.item_id), item_id);
    copy_text(item.label, sizeof(item.label), label);
    copy_text(item.detail, sizeof(item.detail), detail);
    copy_text(item.icon_key, sizeof(item.icon_key), icon_key != nullptr ? icon_key : "");
}

void declare_page(const aegis_host_api_v1_t* host_api, const BrowserModel& model) {
    if (host_api == nullptr || host_api->service_call == nullptr) {
        return;
    }

    auto& page = scratch().page;
    page = {};
    copy_text(page.page_id, sizeof(page.page_id), "files.main");
    copy_text(page.title, sizeof(page.title), "Files");
    copy_text(page.context, sizeof(page.context), model.status_text);

    char token[AEGIS_UI_PAGE_STATE_TOKEN_MAX_V1] {};
    size_t used = 0;
    append_text(token, sizeof(token), used, "f");
    append_uint(token, sizeof(token), used, hash_text(model.current_path));
    append_char(token, sizeof(token), used, ':');
    append_uint(token, sizeof(token), used, model.page_offset);
    append_char(token, sizeof(token), used, ':');
    append_uint(token, sizeof(token), used, static_cast<uint32_t>(model.focus_index));
    append_char(token, sizeof(token), used, ':');
    append_uint(token, sizeof(token), used, model.info_dialog_visible ? 1u : 0u);
    append_char(token, sizeof(token), used, ':');
    append_uint(token, sizeof(token), used, model.generation % 1000u);
    copy_text(page.page_state_token, sizeof(page.page_state_token), token);

    page.layout_template = AEGIS_UI_PAGE_TEMPLATE_FILE_LIST;

    const FileEntry* entry = focused_entry(model);
    const bool can_open = entry != nullptr && entry->directory;
    const bool can_info = entry != nullptr && !entry->directory;
    configure_softkey(page.softkeys[0],
                      AEGIS_UI_SOFTKEY_ROLE_PRIMARY,
                      "Open",
                      "open",
                      true,
                      can_open);
    configure_softkey(page.softkeys[1],
                      AEGIS_UI_SOFTKEY_ROLE_INFO,
                      "Info",
                      "info",
                      true,
                      can_info);
    configure_softkey(
        page.softkeys[2], AEGIS_UI_SOFTKEY_ROLE_BACK, "", "", false, false);

    if (model.state == BrowserState::NoCard) {
        page.item_count = 1;
        set_page_item(page.items[0],
                      "no_card",
                      "No SD card inserted",
                      "Insert a card to browse",
                      true,
                      true,
                      true,
                      AEGIS_UI_ITEM_ACCESSORY_NONE,
                      "");
    } else if (model.state == BrowserState::Unavailable) {
        page.item_count = 1;
        set_page_item(page.items[0],
                      "unavailable",
                      model.status_text,
                      "Storage root unavailable",
                      true,
                      true,
                      true,
                      AEGIS_UI_ITEM_ACCESSORY_NONE,
                      "");
    } else if (model.entry_count == 0) {
        page.item_count = 1;
        set_page_item(page.items[0],
                      "empty",
                      "Folder is empty",
                      "",
                      true,
                      true,
                      true,
                      AEGIS_UI_ITEM_ACCESSORY_NONE,
                      "");
    } else {
        page.item_count = static_cast<uint32_t>(model.entry_count);

        for (size_t index = 0; index < model.entry_count; ++index) {
            const auto& source = model.entries[index];
            const bool focused = index == model.focus_index;
            char detail[AEGIS_UI_PAGE_ITEM_DETAIL_MAX_V1] {};
            uint32_t accessory = AEGIS_UI_ITEM_ACCESSORY_NONE;
            if (source.directory) {
                copy_text(detail, sizeof(detail), "Directory");
                accessory = AEGIS_UI_ITEM_ACCESSORY_FOLDER;
            } else {
                format_size(source.size_bytes, detail, sizeof(detail));
                accessory = AEGIS_UI_ITEM_ACCESSORY_FILE;
            }
            // Use shell-side fallback symbols for file rows until package-owned
            // row icons are stable on the LVGL foreground path.
            set_page_item(page.items[index],
                          source.name,
                          source.name,
                          detail,
                          focused,
                          focused,
                          false,
                          accessory,
                          "");
        }
    }

    if (model.info_dialog_visible && entry != nullptr && !entry->directory) {
        page.dialog.visible = 1u;
        copy_text(page.dialog.title, sizeof(page.dialog.title), "File Info");
        char size_text[24] {};
        char body[AEGIS_UI_DIALOG_BODY_MAX_V1] {};
        size_t body_used = 0;
        format_size(entry->size_bytes, size_text, sizeof(size_text));
        append_text(body, sizeof(body), body_used, "Name: ");
        append_text(body, sizeof(body), body_used, entry->name);
        append_text(body, sizeof(body), body_used, "\nSize: ");
        append_text(body, sizeof(body), body_used, size_text);
        copy_text(page.dialog.body, sizeof(page.dialog.body), body);
    }

    host_api->service_call(host_api->user_data,
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

void focus_next(BrowserModel& model) {
    if (model.entry_count == 0) {
        return;
    }
    model.info_dialog_visible = false;
    if (model.focus_index + 1 < model.entry_count) {
        ++model.focus_index;
    }
    bump_generation(model);
}

void move_next(const aegis_host_api_v1_t* host_api, BrowserModel& model) {
    if (model.entry_count == 0) {
        return;
    }
    if (model.focus_index + 1 < model.entry_count) {
        focus_next(model);
        return;
    }
    if (model.state == BrowserState::Ready && model.has_more && !is_virtual_root(model)) {
        load_path(host_api, model, model.current_path, model.next_offset, 0);
        return;
    }
    bump_generation(model);
}

void focus_previous(BrowserModel& model) {
    if (model.entry_count == 0) {
        return;
    }
    model.info_dialog_visible = false;
    if (model.focus_index > 0) {
        --model.focus_index;
    }
    bump_generation(model);
}

void move_previous(const aegis_host_api_v1_t* host_api, BrowserModel& model) {
    if (model.entry_count == 0) {
        return;
    }
    if (model.focus_index > 0) {
        focus_previous(model);
        return;
    }
    if (model.state == BrowserState::Ready && model.page_offset > 0 && !is_virtual_root(model)) {
        const auto previous_offset = model.page_offset > kPageVisibleEntries
                                         ? model.page_offset - static_cast<uint32_t>(kPageVisibleEntries)
                                         : 0u;
        load_path(host_api,
                  model,
                  model.current_path,
                  previous_offset,
                  kPageVisibleEntries - 1);
        return;
    }
    bump_generation(model);
}

void open_focused(const aegis_host_api_v1_t* host_api, BrowserModel& model) {
    const FileEntry* entry = focused_entry(model);
    if (entry == nullptr || !entry->directory) {
        return;
    }

    model.info_dialog_visible = false;
    if (is_virtual_root(model)) {
        if (text_equals(entry->name, "lfs")) {
            load_path(host_api, model, model.mount_root, 0, 0);
        } else if (text_equals(entry->name, "sdcard")) {
            load_path(host_api, model, "/sdcard", 0, 0);
        }
        return;
    }

    char next_path[AEGIS_STORAGE_PATH_MAX_V1] {};
    join_path(model.current_path, entry->name, next_path, sizeof(next_path));
    load_path(host_api, model, next_path, 0, 0);
}

bool go_back(const aegis_host_api_v1_t* host_api, BrowserModel& model) {
    if (model.info_dialog_visible) {
        model.info_dialog_visible = false;
        bump_generation(model);
        return false;
    }
    if (is_virtual_root(model)) {
        return true;
    }

    char parent_path[AEGIS_STORAGE_PATH_MAX_V1] {};
    parent_path_for(model, parent_path, sizeof(parent_path));
    if (parent_path[0] == '/' && parent_path[1] == '\0') {
        refresh_storage_status(host_api, model);
        load_virtual_root(model);
    } else {
        load_path(host_api, model, parent_path, 0, 0);
    }
    return false;
}

void toggle_info(BrowserModel& model) {
    const FileEntry* entry = focused_entry(model);
    if (entry == nullptr || entry->directory) {
        return;
    }
    model.info_dialog_visible = !model.info_dialog_visible;
    bump_generation(model);
}

}  // namespace

extern "C" aegis_app_run_result_v1_t files_main(const aegis_host_api_v1_t* host_api) {
    if (host_api == nullptr || host_api->abi_version != AEGIS_HOST_API_ABI_V1) {
        return AEGIS_APP_RUN_RESULT_FAILED;
    }

    if (host_api->ui_create_root != nullptr) {
        char root_name[16] {};
        write_root_name(root_name, sizeof(root_name));
        host_api->ui_create_root(host_api->user_data, root_name);
    }

    BrowserModel model {};

    if (!refresh_storage_status(host_api, model)) {
        load_unavailable(model, BrowserState::Unavailable, "Storage unavailable");
    } else {
        load_virtual_root(model);
    }

    declare_page(host_api, model);

    bool exit_requested = false;
    while (!exit_requested) {
        aegis_ui_routed_event_v1_t event {};
        const int status = poll_event(host_api, event);
        if (status != AEGIS_HOST_STATUS_OK) {
            if (host_api->ui_destroy_root != nullptr) {
                char root_name[16] {};
                write_root_name(root_name, sizeof(root_name));
                host_api->ui_destroy_root(host_api->user_data, root_name);
            }
            return AEGIS_APP_RUN_RESULT_FAILED;
        }

        if (event.event_type == AEGIS_UI_EVENT_TYPE_NONE) {
            spin_delay(1500000u);
            continue;
        }

        if (event.event_type == AEGIS_UI_EVENT_TYPE_PAGE_COMMAND) {
            if (text_equals(event.command_id, "open")) {
                open_focused(host_api, model);
                declare_page(host_api, model);
                continue;
            }
            if (text_equals(event.command_id, "info")) {
                toggle_info(model);
                declare_page(host_api, model);
                continue;
            }
        }

        if (event.event_type == AEGIS_UI_EVENT_TYPE_ROUTED_ACTION) {
            switch (event.routed_action) {
                case AEGIS_UI_ROUTED_ACTION_MOVE_NEXT:
                    move_next(host_api, model);
                    declare_page(host_api, model);
                    continue;
                case AEGIS_UI_ROUTED_ACTION_MOVE_PREVIOUS:
                    move_previous(host_api, model);
                    declare_page(host_api, model);
                    continue;
                case AEGIS_UI_ROUTED_ACTION_PRIMARY:
                case AEGIS_UI_ROUTED_ACTION_SELECT:
                    open_focused(host_api, model);
                    declare_page(host_api, model);
                    continue;
                case AEGIS_UI_ROUTED_ACTION_BACK:
                    exit_requested = go_back(host_api, model);
                    if (!exit_requested) {
                        declare_page(host_api, model);
                    }
                    continue;
                default:
                    continue;
            }
        }
    }

    if (host_api->ui_destroy_root != nullptr) {
        char root_name[16] {};
        write_root_name(root_name, sizeof(root_name));
        host_api->ui_destroy_root(host_api->user_data, root_name);
    }
    return AEGIS_APP_RUN_RESULT_COMPLETED;
}

LL_EXTENSION_SYMBOL(files_main);
