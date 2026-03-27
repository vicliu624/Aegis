#include "runtime/host_api/host_api.hpp"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <utility>

#include "sdk/include/aegis/services/audio_service_abi.h"
#include "sdk/include/aegis/services/gps_service_abi.h"
#include "sdk/include/aegis/services/hostlink_service_abi.h"
#include "sdk/include/aegis/services/notification_service_abi.h"
#include "sdk/include/aegis/services/radio_service_abi.h"
#include "sdk/include/aegis/services/settings_service_abi.h"
#include "sdk/include/aegis/services/storage_service_abi.h"
#include "sdk/include/aegis/services/text_input_service_abi.h"

namespace aegis::runtime {

namespace {

template <std::size_t N>
std::string read_c_field(const char (&value)[N]) {
    return std::string(value, ::strnlen(value, N));
}

template <std::size_t N>
void write_c_field(char (&destination)[N], const std::string& value) {
    const auto count = std::min<std::size_t>(N - 1, value.size());
    std::memcpy(destination, value.data(), count);
    destination[count] = '\0';
}

std::uint32_t to_abi_routed_action(shell::ShellNavigationAction action) {
    switch (action) {
        case shell::ShellNavigationAction::MoveNext:
            return AEGIS_UI_ROUTED_ACTION_MOVE_NEXT;
        case shell::ShellNavigationAction::MovePrevious:
            return AEGIS_UI_ROUTED_ACTION_MOVE_PREVIOUS;
        case shell::ShellNavigationAction::PrimaryAction:
            return AEGIS_UI_ROUTED_ACTION_PRIMARY;
        case shell::ShellNavigationAction::Select:
            return AEGIS_UI_ROUTED_ACTION_SELECT;
        case shell::ShellNavigationAction::Back:
            return AEGIS_UI_ROUTED_ACTION_BACK;
        case shell::ShellNavigationAction::OpenMenu:
            return AEGIS_UI_ROUTED_ACTION_OPEN_MENU;
        case shell::ShellNavigationAction::OpenSettings:
            return AEGIS_UI_ROUTED_ACTION_OPEN_SETTINGS;
        case shell::ShellNavigationAction::OpenNotifications:
            return AEGIS_UI_ROUTED_ACTION_OPEN_NOTIFICATIONS;
    }

    return AEGIS_UI_ROUTED_ACTION_NONE;
}

ForegroundPageTemplate from_abi_page_template(std::uint32_t value) {
    switch (value) {
        case AEGIS_UI_PAGE_TEMPLATE_SIMPLE_LIST:
            return ForegroundPageTemplate::SimpleList;
        case AEGIS_UI_PAGE_TEMPLATE_VALUE_LIST:
            return ForegroundPageTemplate::ValueList;
        case AEGIS_UI_PAGE_TEMPLATE_FILE_LIST:
            return ForegroundPageTemplate::FileList;
        case AEGIS_UI_PAGE_TEMPLATE_TEXT_LOG:
        default:
            return ForegroundPageTemplate::TextLog;
    }
}

ForegroundPageItemAccessory from_abi_page_item_accessory(std::uint32_t value) {
    switch (value) {
        case AEGIS_UI_ITEM_ACCESSORY_FILE:
            return ForegroundPageItemAccessory::File;
        case AEGIS_UI_ITEM_ACCESSORY_FOLDER:
            return ForegroundPageItemAccessory::Folder;
        case AEGIS_UI_ITEM_ACCESSORY_NONE:
        default:
            return ForegroundPageItemAccessory::None;
    }
}

ForegroundPageItemIconSource from_abi_page_item_icon_source(std::uint32_t value) {
    switch (value) {
        case AEGIS_UI_ITEM_ICON_SOURCE_APP_ASSET:
            return ForegroundPageItemIconSource::AppAsset;
        case AEGIS_UI_ITEM_ICON_SOURCE_NONE:
        default:
            return ForegroundPageItemIconSource::None;
    }
}

bool is_relative_asset_path(std::string_view path) {
    return !path.empty() && path.front() != '/' && path.find("..") == std::string_view::npos;
}

std::string join_app_path(std::string_view app_dir, std::string_view relative_path) {
    if (app_dir.empty()) {
        return std::string(relative_path);
    }
    return std::string(app_dir) + "/" + std::string(relative_path);
}

}  // namespace

HostApi::HostApi(const device::DeviceProfile& profile,
                 device::ServiceBindingRegistry& services,
                 ResourceOwnershipTable& ownership,
                 std::string app_dir,
                 std::string session_id,
                 std::vector<core::AppPermissionId> granted_permissions,
                 AppQuotaLedger* quota_ledger,
                 std::function<void(const std::string&)> ui_root_observer,
                 std::function<void(const ForegroundPagePresentation&)> foreground_page_observer,
                 std::function<std::optional<shell::ShellInputInvocation>()> ui_invocation_poller,
                 std::function<std::optional<shell::ShellNavigationAction>()> routed_action_poller,
                 std::function<void(const std::string&, const std::string&)> app_log_observer)
    : profile_(profile),
      services_(services),
      dispatch_(services_),
      syscall_gateway_(dispatch_,
                       quota_ledger,
                       [this](core::AppPermissionId permission, std::string_view operation) {
                           return require_permission(permission, operation);
                       },
                       [this](const std::string& tag, const std::string& message) {
                           log(tag, message);
                       }),
      ownership_(ownership),
      app_dir_(std::move(app_dir)),
      session_id_(std::move(session_id)),
      granted_permissions_(std::move(granted_permissions)),
      quota_ledger_(quota_ledger),
      ui_root_observer_(std::move(ui_root_observer)),
      foreground_page_observer_(std::move(foreground_page_observer)),
      ui_invocation_poller_(std::move(ui_invocation_poller)),
      routed_action_poller_(std::move(routed_action_poller)),
      app_log_observer_(std::move(app_log_observer)),
      abi_ {.abi_version = AEGIS_HOST_API_ABI_V1,
            .user_data = this,
            .log_write = &HostApi::abi_log_write,
            .get_capability_level = &HostApi::abi_get_capability_level,
            .mem_alloc = &HostApi::abi_mem_alloc,
            .mem_free = &HostApi::abi_mem_free,
            .ui_create_root = &HostApi::abi_ui_create_root,
            .ui_destroy_root = &HostApi::abi_ui_destroy_root,
            .timer_create = &HostApi::abi_timer_create,
            .timer_cancel = &HostApi::abi_timer_cancel,
            .service_call = &HostApi::abi_service_call} {}

void HostApi::log(const std::string& tag, const std::string& message) const {
    if (quota_ledger_ != nullptr) {
        if (quota_ledger_->would_exceed(QuotaResource::LogBytesPerMinute, message.size())) {
            services_.logging()->log("runtime", "drop app log because log quota would be exceeded");
            return;
        }
        quota_ledger_->force_consume(QuotaResource::LogBytesPerMinute, message.size());
    }
    services_.logging()->log(tag, message);
    if (app_log_observer_) {
        app_log_observer_(tag, message);
    }
}

const device::CapabilitySet& HostApi::capabilities() const {
    return profile_.capabilities;
}

std::string HostApi::describe_display() const {
    return services_.display()->describe_surface();
}

std::string HostApi::describe_input() const {
    return services_.input()->describe_input_mode();
}

std::string HostApi::describe_radio() const {
    return services_.radio()->backend_name();
}

std::string HostApi::describe_gps() const {
    return services_.gps()->backend_name();
}

void* HostApi::allocate(std::size_t size, std::size_t alignment) const {
    (void)alignment;
    if (quota_ledger_ != nullptr &&
        !quota_ledger_->try_consume(QuotaResource::HeapBytes, size)) {
        log("runtime", "deny allocation because heap quota would be exceeded");
        return nullptr;
    }
    void* memory = std::malloc(size);
    if (memory != nullptr) {
        const auto resource = "mem:" + std::to_string(reinterpret_cast<std::uintptr_t>(memory));
        ownership_.track(session_id_, resource);
        const auto handle_id = handle_table_.register_handle(AppHandleKind::Allocation, resource);
        owned_allocations_[memory] = AllocationRecord {
            .resource_name = resource,
            .size_bytes = size,
            .handle_id = handle_id,
        };
    } else if (quota_ledger_ != nullptr) {
        quota_ledger_->release(QuotaResource::HeapBytes, size);
    }
    return memory;
}

bool HostApi::free(void* ptr) const {
    if (ptr == nullptr) {
        return false;
    }
    const auto it = owned_allocations_.find(ptr);
    if (it == owned_allocations_.end()) {
        return false;
    }
    const auto _ = ownership_.untrack(session_id_, it->second.resource_name);
    (void)_;
    if (quota_ledger_ != nullptr) {
        quota_ledger_->release(QuotaResource::HeapBytes, it->second.size_bytes);
    }
    const auto released = handle_table_.release(it->second.handle_id);
    (void)released;
    std::free(ptr);
    owned_allocations_.erase(it);
    return true;
}

void HostApi::create_ui_root(const std::string& name) const {
    if (const auto status = require_permission(core::AppPermissionId::UiSurface, "create ui root");
        status.has_value()) {
        return;
    }
    const auto label = "ui_root:" + name;
    ownership_.track(session_id_, label);
    const auto handle_id = handle_table_.register_handle(AppHandleKind::UiRoot, label);
    (void)handle_id;
    if (ui_root_observer_) {
        ui_root_observer_(name);
    }
}

bool HostApi::destroy_ui_root(const std::string& name) const {
    if (const auto status =
            require_permission(core::AppPermissionId::UiSurface, "destroy ui root");
        status.has_value()) {
        return false;
    }
    const auto label = "ui_root:" + name;
    const auto untracked = ownership_.untrack(session_id_, label);
    const auto released = handle_table_.release_by_label(AppHandleKind::UiRoot, label);
    return untracked || released;
}

int HostApi::create_timer(std::uint32_t timeout_ms, bool repeat) const {
    if (const auto status =
            require_permission(core::AppPermissionId::TimerControl, "create timer");
        status.has_value()) {
        return *status;
    }

    const int timer_id = services_.timer()->create(timeout_ms, repeat);
    if (timer_id > 0) {
        if (quota_ledger_ != nullptr &&
            !quota_ledger_->try_consume(QuotaResource::TimerCount, 1)) {
            const auto _ = services_.timer()->cancel(timer_id);
            (void)_;
            log("runtime", "deny timer because timer quota would be exceeded");
            return AEGIS_HOST_STATUS_UNSUPPORTED;
        }
        const auto label = "timer:" + std::to_string(timer_id);
        ownership_.track(session_id_, label, [timer = services_.timer(), timer_id]() {
            const auto _ = timer->cancel(timer_id);
            (void)_;
        });
        const auto handle_id = handle_table_.register_handle(AppHandleKind::Timer, label);
        (void)handle_id;
    }
    return timer_id;
}

bool HostApi::cancel_timer(int timer_id) const {
    if (const auto status =
            require_permission(core::AppPermissionId::TimerControl, "cancel timer");
        status.has_value()) {
        return false;
    }

    const bool canceled = services_.timer()->cancel(timer_id);
    if (canceled) {
        const auto label = "timer:" + std::to_string(timer_id);
        const auto _ = ownership_.untrack(session_id_, label);
        (void)_;
        const auto released = handle_table_.release_by_label(AppHandleKind::Timer, label);
        (void)released;
        if (quota_ledger_ != nullptr) {
            quota_ledger_->release(QuotaResource::TimerCount, 1);
        }
    }
    return canceled;
}

void HostApi::set_setting(const std::string& key, const std::string& value) const {
    if (const auto status =
            require_permission(core::AppPermissionId::SettingsWrite, "set setting");
        status.has_value()) {
        return;
    }
    services_.settings()->set(key, value);
}

std::string HostApi::get_setting(const std::string& key) const {
    return services_.settings()->get(key);
}

void HostApi::notify(const std::string& title, const std::string& body) const {
    if (const auto status =
            require_permission(core::AppPermissionId::NotificationPost, "publish notification");
        status.has_value()) {
        return;
    }
    const auto label = "notification:" + title;
    ownership_.track(session_id_, label);
    const auto handle_id = handle_table_.register_handle(AppHandleKind::Notification, label);
    (void)handle_id;
    services_.notifications()->notify(title, body);
}

int HostApi::capability_level(std::uint32_t capability_id) const {
    return static_cast<int>(profile_.capabilities.level(static_cast<device::CapabilityId>(capability_id)));
}

int HostApi::request_text_input_focus() const {
    if (const auto status =
            require_permission(core::AppPermissionId::TextInputFocus, "request text input focus");
        status.has_value()) {
        return *status;
    }

    const auto service = services_.text_input();
    if (!service) {
        return AEGIS_HOST_STATUS_UNSUPPORTED;
    }

    if (!service->request_focus_for_session(session_id_)) {
        return AEGIS_HOST_STATUS_UNSUPPORTED;
    }

    ownership_.track(session_id_,
                     "text_input_focus",
                     [service, session_id = session_id_]() {
                         const auto _ = service->release_focus_for_session(session_id);
                         (void)_;
                     });
    const auto handle_id =
        handle_table_.register_handle(AppHandleKind::TextInputFocus, "text_input_focus");
    (void)handle_id;
    return AEGIS_HOST_STATUS_OK;
}

int HostApi::release_text_input_focus() const {
    if (const auto status =
            require_permission(core::AppPermissionId::TextInputFocus, "release text input focus");
        status.has_value()) {
        return *status;
    }

    const auto service = services_.text_input();
    if (!service) {
        return AEGIS_HOST_STATUS_UNSUPPORTED;
    }

    if (!service->release_focus_for_session(session_id_)) {
        return AEGIS_HOST_STATUS_UNSUPPORTED;
    }

    const auto _ = ownership_.untrack(session_id_, "text_input_focus");
    (void)_;
    const auto released =
        handle_table_.release_by_label(AppHandleKind::TextInputFocus, "text_input_focus");
    (void)released;
    return AEGIS_HOST_STATUS_OK;
}

int HostApi::set_foreground_page(const aegis_ui_foreground_page_v1_t& page) const {
    if (const auto status =
            require_permission(core::AppPermissionId::UiSurface, "set foreground page");
        status.has_value()) {
        return *status;
    }

    ForegroundPagePresentation presentation;
    presentation.page_id = read_c_field(page.page_id);
    presentation.page_state_token = read_c_field(page.page_state_token);
    presentation.title = read_c_field(page.title);
    presentation.context = read_c_field(page.context);
    presentation.layout_template = from_abi_page_template(page.layout_template);
    if (presentation.page_id.empty()) {
        return AEGIS_HOST_STATUS_INVALID_ARGUMENT;
    }

    for (std::size_t index = 0; index < presentation.softkeys.size(); ++index) {
        const auto& source = page.softkeys[index];
        auto& target = presentation.softkeys[index];
        target.visible = source.visible != 0;
        target.enabled = source.enabled != 0;
        target.role = source.role;
        target.label = read_c_field(source.label);
        target.command_id = read_c_field(source.command_id);
    }

    const auto item_count =
        std::min<std::size_t>(page.item_count, AEGIS_UI_PAGE_ITEM_MAX_V1);
    presentation.items.reserve(item_count);
    for (std::size_t index = 0; index < item_count; ++index) {
        const auto& source = page.items[index];
        auto icon_key = read_c_field(source.icon_key);
        const auto icon_source = from_abi_page_item_icon_source(source.icon_source);
        if (icon_source == ForegroundPageItemIconSource::AppAsset) {
            if (!is_relative_asset_path(icon_key)) {
                log("runtime", "drop invalid app asset icon path=" + icon_key);
                icon_key.clear();
            } else {
                icon_key = join_app_path(app_dir_, icon_key);
            }
        } else {
            icon_key.clear();
        }
        presentation.items.push_back(ForegroundPageItem {
            .item_id = read_c_field(source.item_id),
            .label = read_c_field(source.label),
            .detail = read_c_field(source.detail),
            .focused = source.focused != 0,
            .emphasized = source.emphasized != 0,
            .warning = source.warning != 0,
            .disabled = source.disabled != 0,
            .accessory = from_abi_page_item_accessory(source.accessory),
            .icon_source = icon_source,
            .icon_key = std::move(icon_key),
        });
    }
    presentation.dialog.visible = page.dialog.visible != 0;
    presentation.dialog.title = read_c_field(page.dialog.title);
    presentation.dialog.body = read_c_field(page.dialog.body);
    log("runtime",
        "foreground page accepted id=" + presentation.page_id + " items=" +
            std::to_string(presentation.items.size()) + " template=" +
            std::to_string(static_cast<int>(presentation.layout_template)) + " dialog=" +
            (presentation.dialog.visible ? std::string("1") : std::string("0")) +
            " context=" + presentation.context);

    const auto _ = ownership_.untrack(session_id_, "foreground_page");
    (void)_;
    foreground_page_ = presentation;
    ownership_.track(session_id_, "foreground_page");
    const auto released =
        handle_table_.release_by_label(AppHandleKind::ForegroundPage, "foreground_page");
    (void)released;
    const auto handle_id =
        handle_table_.register_handle(AppHandleKind::ForegroundPage, "foreground_page");
    (void)handle_id;
    if (foreground_page_observer_) {
        foreground_page_observer_(presentation);
    }
    return AEGIS_HOST_STATUS_OK;
}

int HostApi::poll_ui_event(aegis_ui_routed_event_v1_t& event) const {
    std::memset(&event, 0, sizeof(event));
    event.event_type = AEGIS_UI_EVENT_TYPE_NONE;
    event.event_source = AEGIS_UI_EVENT_SOURCE_UNKNOWN;
    event.routed_action = AEGIS_UI_ROUTED_ACTION_NONE;

    if (ui_invocation_poller_) {
        for (;;) {
            const auto invocation = ui_invocation_poller_();
            if (!invocation.has_value()) {
                break;
            }
            if (invocation->target == shell::ShellInputInvocationTarget::PageCommand) {
                const auto page_id = std::string(shell::invocation_page_id(*invocation));
                const auto page_state_token =
                    std::string(shell::invocation_page_state_token(*invocation));
                const auto command_id =
                    std::string(shell::invocation_page_command_id(*invocation));
                if (!foreground_page_.has_value() || page_id != foreground_page_->page_id ||
                    page_state_token != foreground_page_->page_state_token) {
                    log("runtime",
                        "drop stale foreground invocation page=" + page_id + " command=" +
                            command_id);
                    continue;
                }

                event.event_type = AEGIS_UI_EVENT_TYPE_PAGE_COMMAND;
                write_c_field(event.page_id, foreground_page_->page_id);
                write_c_field(event.page_state_token, foreground_page_->page_state_token);
                write_c_field(event.command_id, command_id);
                for (std::size_t index = 0; index < foreground_page_->softkeys.size(); ++index) {
                    const auto& softkey = foreground_page_->softkeys[index];
                    if (softkey.visible && softkey.command_id == command_id) {
                        event.event_source = index == 0 ? AEGIS_UI_EVENT_SOURCE_SOFTKEY_LEFT
                                            : index == 1 ? AEGIS_UI_EVENT_SOURCE_SOFTKEY_CENTER
                                                         : AEGIS_UI_EVENT_SOURCE_SOFTKEY_RIGHT;
                        break;
                    }
                }
                return AEGIS_HOST_STATUS_OK;
            }

            event.event_type = AEGIS_UI_EVENT_TYPE_ROUTED_ACTION;
            event.event_source = AEGIS_UI_EVENT_SOURCE_SOFTKEY_RIGHT;
            event.routed_action = to_abi_routed_action(invocation->system_action);
            if (foreground_page_.has_value()) {
                write_c_field(event.page_id, foreground_page_->page_id);
                write_c_field(event.page_state_token, foreground_page_->page_state_token);
            }
            return AEGIS_HOST_STATUS_OK;
        }
    }

    if (routed_action_poller_) {
        if (const auto action = routed_action_poller_(); action.has_value()) {
            event.event_type = AEGIS_UI_EVENT_TYPE_ROUTED_ACTION;
            event.event_source = AEGIS_UI_EVENT_SOURCE_PHYSICAL;
            event.routed_action = to_abi_routed_action(*action);
            if (foreground_page_.has_value()) {
                write_c_field(event.page_id, foreground_page_->page_id);
                write_c_field(event.page_state_token, foreground_page_->page_state_token);
            }
        }
    }
    return AEGIS_HOST_STATUS_OK;
}

int HostApi::dispatch_service(std::uint32_t domain,
                              std::uint32_t op,
                              const void* input,
                              std::size_t input_size,
                              void* output,
                              std::size_t* output_size) const {
    if (domain == AEGIS_SERVICE_DOMAIN_TEXT_INPUT) {
        if (op == AEGIS_TEXT_INPUT_SERVICE_OP_REQUEST_FOCUS) {
            return request_text_input_focus();
        }
        if (op == AEGIS_TEXT_INPUT_SERVICE_OP_RELEASE_FOCUS) {
            return release_text_input_focus();
        }
    }

    if (domain == AEGIS_SERVICE_DOMAIN_UI) {
        if (op == AEGIS_UI_SERVICE_OP_SET_FOREGROUND_PAGE) {
            if (input == nullptr || input_size != sizeof(aegis_ui_foreground_page_v1_t)) {
                return AEGIS_HOST_STATUS_INVALID_ARGUMENT;
            }
            return set_foreground_page(
                *static_cast<const aegis_ui_foreground_page_v1_t*>(input));
        }
        if (op == AEGIS_UI_SERVICE_OP_POLL_EVENT) {
            if (output == nullptr || output_size == nullptr ||
                *output_size < sizeof(aegis_ui_routed_event_v1_t)) {
                if (output_size != nullptr) {
                    *output_size = sizeof(aegis_ui_routed_event_v1_t);
                }
                return AEGIS_HOST_STATUS_BUFFER_TOO_SMALL;
            }
            auto* event = static_cast<aegis_ui_routed_event_v1_t*>(output);
            const int status = poll_ui_event(*event);
            *output_size = sizeof(aegis_ui_routed_event_v1_t);
            return status;
        }
    }

    return syscall_gateway_.dispatch_service(domain, op, input, input_size, output, output_size);
}

const aegis_host_api_v1_t* HostApi::abi() const {
    return &abi_;
}

bool HostApi::has_permission(core::AppPermissionId permission) const {
    return std::find(granted_permissions_.begin(), granted_permissions_.end(), permission) !=
           granted_permissions_.end();
}

std::optional<int> HostApi::require_permission(core::AppPermissionId permission,
                                               std::string_view operation) const {
    if (has_permission(permission)) {
        return std::nullopt;
    }

    log("runtime",
        std::string("denied operation '") + std::string(operation) +
            "' because manifest did not request permission " +
            std::string(core::to_string(permission)));
    return AEGIS_HOST_STATUS_UNSUPPORTED;
}

int HostApi::abi_log_write(void* user_data, const char* tag, const char* message) {
    if (user_data == nullptr) {
        return AEGIS_HOST_STATUS_INVALID_ARGUMENT;
    }

    static_cast<HostApi*>(user_data)->log(tag ? tag : "", message ? message : "");
    return 0;
}

int HostApi::abi_get_capability_level(void* user_data, std::uint32_t capability_id) {
    if (user_data == nullptr) {
        return AEGIS_HOST_STATUS_INVALID_ARGUMENT;
    }

    return static_cast<HostApi*>(user_data)->capability_level(capability_id);
}

void* HostApi::abi_mem_alloc(void* user_data, std::size_t size, std::size_t alignment) {
    if (user_data == nullptr || size == 0) {
        return nullptr;
    }

    return static_cast<HostApi*>(user_data)->allocate(size, alignment);
}

int HostApi::abi_mem_free(void* user_data, void* ptr) {
    if (user_data == nullptr || ptr == nullptr) {
        return AEGIS_HOST_STATUS_INVALID_ARGUMENT;
    }

    return static_cast<HostApi*>(user_data)->free(ptr) ? AEGIS_HOST_STATUS_OK
                                                       : AEGIS_HOST_STATUS_UNSUPPORTED;
}

int HostApi::abi_ui_create_root(void* user_data, const char* root_name) {
    if (user_data == nullptr) {
        return AEGIS_HOST_STATUS_INVALID_ARGUMENT;
    }

    static_cast<HostApi*>(user_data)->create_ui_root(root_name ? root_name : "");
    return 0;
}

int HostApi::abi_ui_destroy_root(void* user_data, const char* root_name) {
    if (user_data == nullptr) {
        return AEGIS_HOST_STATUS_INVALID_ARGUMENT;
    }

    return static_cast<HostApi*>(user_data)->destroy_ui_root(root_name ? root_name : "")
               ? AEGIS_HOST_STATUS_OK
               : AEGIS_HOST_STATUS_UNSUPPORTED;
}

int HostApi::abi_timer_create(void* user_data, std::uint32_t timeout_ms, int repeat, int* timer_id) {
    if (user_data == nullptr || timer_id == nullptr) {
        return AEGIS_HOST_STATUS_INVALID_ARGUMENT;
    }

    const int created = static_cast<HostApi*>(user_data)->create_timer(timeout_ms, repeat != 0);
    *timer_id = created;
    return created > 0 ? AEGIS_HOST_STATUS_OK : AEGIS_HOST_STATUS_UNSUPPORTED;
}

int HostApi::abi_timer_cancel(void* user_data, int timer_id) {
    if (user_data == nullptr) {
        return AEGIS_HOST_STATUS_INVALID_ARGUMENT;
    }

    return static_cast<HostApi*>(user_data)->cancel_timer(timer_id) ? AEGIS_HOST_STATUS_OK
                                                                    : AEGIS_HOST_STATUS_UNSUPPORTED;
}

int HostApi::abi_service_call(void* user_data,
                              std::uint32_t domain,
                              std::uint32_t op,
                              const void* input,
                              std::size_t input_size,
                              void* output,
                              std::size_t* output_size) {
    if (user_data == nullptr) {
        return AEGIS_HOST_STATUS_INVALID_ARGUMENT;
    }

    return static_cast<HostApi*>(user_data)->dispatch_service(domain, op, input, input_size, output,
                                                              output_size);
}

}  // namespace aegis::runtime
