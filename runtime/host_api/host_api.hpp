#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "core/app_registry/app_permissions.hpp"
#include "device/common/binding/service_binding_registry.hpp"
#include "device/common/profile/device_profile.hpp"
#include "runtime/host_api/service_gateway_dispatch.hpp"
#include "runtime/ownership/resource_ownership_table.hpp"
#include "sdk/include/aegis/host_api_abi.h"

namespace aegis::runtime {

class HostApi {
public:
    HostApi(const device::DeviceProfile& profile,
            device::ServiceBindingRegistry& services,
            ResourceOwnershipTable& ownership,
            std::string session_id,
            std::vector<core::AppPermissionId> granted_permissions);

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
    [[nodiscard]] std::optional<int> require_service_permission(std::uint32_t domain,
                                                                std::uint32_t op) const;

    const device::DeviceProfile& profile_;
    device::ServiceBindingRegistry& services_;
    ServiceGatewayDispatch dispatch_;
    ResourceOwnershipTable& ownership_;
    std::string session_id_;
    std::vector<core::AppPermissionId> granted_permissions_;
    mutable std::unordered_map<void*, std::string> owned_allocations_;
    aegis_host_api_v1_t abi_;
};

}  // namespace aegis::runtime
