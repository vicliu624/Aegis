#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "core/app_registry/app_permissions.hpp"
#include "device/common/binding/service_binding_registry.hpp"
#include "device/common/profile/device_profile.hpp"
#include "runtime/handles/app_handle_table.hpp"
#include "runtime/host_api/service_gateway_dispatch.hpp"
#include "runtime/ownership/resource_ownership_table.hpp"
#include "runtime/quota/quota_ledger.hpp"
#include "runtime/syscall/syscall_gateway.hpp"
#include "sdk/include/aegis/host_api_abi.h"
#include "sdk/include/aegis/services/ui_service_abi.h"
#include "shell/control/shell_input_model.hpp"

namespace aegis::runtime {

struct ForegroundPageSoftkeyState {
    std::string label;
    std::string command_id;
    std::uint32_t role {AEGIS_UI_SOFTKEY_ROLE_CUSTOM};
    bool visible {false};
    bool enabled {false};
};

enum class ForegroundPageTemplate {
    TextLog,
    SimpleList,
    ValueList,
    FileList,
};

enum class ForegroundPageItemAccessory {
    None,
    File,
    Folder,
};

enum class ForegroundPageItemIconSource {
    None,
    AppAsset,
};

struct ForegroundPageItem {
    std::string item_id;
    std::string label;
    std::string detail;
    bool focused {false};
    bool emphasized {false};
    bool warning {false};
    bool disabled {false};
    ForegroundPageItemAccessory accessory {ForegroundPageItemAccessory::None};
    ForegroundPageItemIconSource icon_source {ForegroundPageItemIconSource::None};
    std::string icon_key;
};

struct ForegroundPageDialog {
    bool visible {false};
    std::string title;
    std::string body;
};

struct ForegroundPagePresentation {
    std::string page_id;
    std::string page_state_token;
    std::string title;
    std::string context;
    ForegroundPageTemplate layout_template {ForegroundPageTemplate::TextLog};
    std::array<ForegroundPageSoftkeyState, 3> softkeys {};
    std::vector<ForegroundPageItem> items;
    ForegroundPageDialog dialog {};
};

class HostApi {
public:
    HostApi(const device::DeviceProfile& profile,
            device::ServiceBindingRegistry& services,
            ResourceOwnershipTable& ownership,
            std::string app_dir,
            std::string session_id,
            std::vector<core::AppPermissionId> granted_permissions,
            AppQuotaLedger* quota_ledger = nullptr,
            std::function<void(const std::string&)> ui_root_observer = {},
            std::function<void(const ForegroundPagePresentation&)> foreground_page_observer = {},
            std::function<std::optional<shell::ShellInputInvocation>()> ui_invocation_poller = {},
            std::function<std::optional<shell::ShellNavigationAction>()> routed_action_poller = {},
            std::function<void(const std::string&, const std::string&)> app_log_observer = {});

    void log(const std::string& tag, const std::string& message) const;
    [[nodiscard]] const device::CapabilitySet& capabilities() const;
    [[nodiscard]] std::string describe_display() const;
    [[nodiscard]] std::string describe_input() const;
    [[nodiscard]] std::string describe_radio() const;
    [[nodiscard]] std::string describe_gps() const;
    [[nodiscard]] void* allocate(std::size_t size, std::size_t alignment) const;
    bool free(void* ptr) const;
    void create_ui_root(const std::string& name) const;
    bool destroy_ui_root(const std::string& name) const;
    int create_timer(std::uint32_t timeout_ms, bool repeat) const;
    bool cancel_timer(int timer_id) const;
    void set_setting(const std::string& key, const std::string& value) const;
    [[nodiscard]] std::string get_setting(const std::string& key) const;
    void notify(const std::string& title, const std::string& body) const;
    [[nodiscard]] int capability_level(std::uint32_t capability_id) const;
    int request_text_input_focus() const;
    int release_text_input_focus() const;
    int set_foreground_page(const aegis_ui_foreground_page_v1_t& page) const;
    int poll_ui_event(aegis_ui_routed_event_v1_t& event) const;
    int dispatch_service(std::uint32_t domain,
                         std::uint32_t op,
                         const void* input,
                         std::size_t input_size,
                         void* output,
                         std::size_t* output_size) const;
    [[nodiscard]] const aegis_host_api_v1_t* abi() const;

private:
    static int abi_log_write(void* user_data, const char* tag, const char* message);
    static int abi_get_capability_level(void* user_data, std::uint32_t capability_id);
    static void* abi_mem_alloc(void* user_data, std::size_t size, std::size_t alignment);
    static int abi_mem_free(void* user_data, void* ptr);
    static int abi_ui_create_root(void* user_data, const char* root_name);
    static int abi_ui_destroy_root(void* user_data, const char* root_name);
    static int abi_timer_create(void* user_data,
                                std::uint32_t timeout_ms,
                                int repeat,
                                int* timer_id);
    static int abi_timer_cancel(void* user_data, int timer_id);
    static int abi_service_call(void* user_data,
                                std::uint32_t domain,
                                std::uint32_t op,
                                const void* input,
                                std::size_t input_size,
                                void* output,
                                std::size_t* output_size);
    [[nodiscard]] bool has_permission(core::AppPermissionId permission) const;
    [[nodiscard]] std::optional<int> require_permission(core::AppPermissionId permission,
                                                        std::string_view operation) const;

    const device::DeviceProfile& profile_;
    device::ServiceBindingRegistry& services_;
    ServiceGatewayDispatch dispatch_;
    SyscallGateway syscall_gateway_;
    ResourceOwnershipTable& ownership_;
    std::string app_dir_;
    std::string session_id_;
    std::vector<core::AppPermissionId> granted_permissions_;
    AppQuotaLedger* quota_ledger_ {nullptr};
    std::function<void(const std::string&)> ui_root_observer_;
    std::function<void(const ForegroundPagePresentation&)> foreground_page_observer_;
    std::function<std::optional<shell::ShellInputInvocation>()> ui_invocation_poller_;
    std::function<std::optional<shell::ShellNavigationAction>()> routed_action_poller_;
    std::function<void(const std::string&, const std::string&)> app_log_observer_;
    mutable std::optional<ForegroundPagePresentation> foreground_page_;
    mutable AppHandleTable handle_table_;
    struct AllocationRecord {
        std::string resource_name;
        std::size_t size_bytes {0};
        std::uint32_t handle_id {0};
    };
    mutable std::unordered_map<void*, AllocationRecord> owned_allocations_;
    aegis_host_api_v1_t abi_;
};

}  // namespace aegis::runtime
