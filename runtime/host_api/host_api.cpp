#include "runtime/host_api/host_api.hpp"

#include <algorithm>
#include <cstdlib>

#include "sdk/include/aegis/services/audio_service_abi.h"
#include "sdk/include/aegis/services/gps_service_abi.h"
#include "sdk/include/aegis/services/hostlink_service_abi.h"
#include "sdk/include/aegis/services/notification_service_abi.h"
#include "sdk/include/aegis/services/radio_service_abi.h"
#include "sdk/include/aegis/services/settings_service_abi.h"
#include "sdk/include/aegis/services/storage_service_abi.h"
#include "sdk/include/aegis/services/text_input_service_abi.h"

namespace aegis::runtime {

HostApi::HostApi(const device::DeviceProfile& profile,
                 device::ServiceBindingRegistry& services,
                 ResourceOwnershipTable& ownership,
                 std::string session_id,
                 std::vector<core::AppPermissionId> granted_permissions,
                 std::function<void(const std::string&)> ui_root_observer,
                 std::function<void(const std::string&, const std::string&)> app_log_observer)
    : profile_(profile),
      services_(services),
      dispatch_(services_),
      ownership_(ownership),
      session_id_(std::move(session_id)),
      granted_permissions_(std::move(granted_permissions)),
      ui_root_observer_(std::move(ui_root_observer)),
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
    void* memory = std::malloc(size);
    if (memory != nullptr) {
        const auto resource = "mem:" + std::to_string(reinterpret_cast<std::uintptr_t>(memory));
        ownership_.track(session_id_, resource);
        owned_allocations_[memory] = resource;
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
    const auto _ = ownership_.untrack(session_id_, it->second);
    (void)_;
    std::free(ptr);
    owned_allocations_.erase(it);
    return true;
}

void HostApi::create_ui_root(const std::string& name) const {
    if (const auto status = require_permission(core::AppPermissionId::UiSurface, "create ui root");
        status.has_value()) {
        return;
    }
    ownership_.track(session_id_, "ui_root:" + name);
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
    return ownership_.untrack(session_id_, "ui_root:" + name);
}

int HostApi::create_timer(std::uint32_t timeout_ms, bool repeat) const {
    if (const auto status =
            require_permission(core::AppPermissionId::TimerControl, "create timer");
        status.has_value()) {
        return *status;
    }

    const int timer_id = services_.timer()->create(timeout_ms, repeat);
    if (timer_id > 0) {
        ownership_.track(session_id_, "timer:" + std::to_string(timer_id));
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
        const auto _ = ownership_.untrack(session_id_, "timer:" + std::to_string(timer_id));
        (void)_;
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
    ownership_.track(session_id_, "notification:" + title);
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
    return AEGIS_HOST_STATUS_OK;
}

int HostApi::dispatch_service(std::uint32_t domain,
                              std::uint32_t op,
                              const void* input,
                              std::size_t input_size,
                              void* output,
                              std::size_t* output_size) const {
    if (const auto status = require_service_permission(domain, op); status.has_value()) {
        return *status;
    }

    if (domain == AEGIS_SERVICE_DOMAIN_TEXT_INPUT) {
        if (op == AEGIS_TEXT_INPUT_SERVICE_OP_REQUEST_FOCUS) {
            return request_text_input_focus();
        }
        if (op == AEGIS_TEXT_INPUT_SERVICE_OP_RELEASE_FOCUS) {
            return release_text_input_focus();
        }
    }

    return dispatch_.call(domain, op, input, input_size, output, output_size);
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

std::optional<int> HostApi::require_service_permission(std::uint32_t domain, std::uint32_t op) const {
    switch (domain) {
        case AEGIS_SERVICE_DOMAIN_RADIO:
            return require_permission(core::AppPermissionId::RadioUse, "access radio service");
        case AEGIS_SERVICE_DOMAIN_GPS:
            return require_permission(core::AppPermissionId::GpsUse, "access gps service");
        case AEGIS_SERVICE_DOMAIN_AUDIO:
            return require_permission(core::AppPermissionId::AudioUse, "access audio service");
        case AEGIS_SERVICE_DOMAIN_HOSTLINK:
            return require_permission(core::AppPermissionId::HostlinkUse,
                                      "access hostlink service");
        case AEGIS_SERVICE_DOMAIN_STORAGE:
            return require_permission(core::AppPermissionId::StorageAccess,
                                      "access storage service");
        case AEGIS_SERVICE_DOMAIN_NOTIFICATION:
            if (op == AEGIS_NOTIFICATION_SERVICE_OP_PUBLISH) {
                return require_permission(core::AppPermissionId::NotificationPost,
                                          "access notification service");
            }
            return std::nullopt;
        case AEGIS_SERVICE_DOMAIN_SETTINGS:
            if (op == AEGIS_SETTINGS_SERVICE_OP_SET) {
                return require_permission(core::AppPermissionId::SettingsWrite,
                                          "write settings service");
            }
            return std::nullopt;
        case AEGIS_SERVICE_DOMAIN_TEXT_INPUT:
            if (op == AEGIS_TEXT_INPUT_SERVICE_OP_REQUEST_FOCUS ||
                op == AEGIS_TEXT_INPUT_SERVICE_OP_RELEASE_FOCUS) {
                return require_permission(core::AppPermissionId::TextInputFocus,
                                          "control text input focus");
            }
            return std::nullopt;
        default:
            return std::nullopt;
    }
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
