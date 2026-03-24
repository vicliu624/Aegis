#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "sdk/include/aegis/host_api_abi.h"
#include "sdk/include/aegis/services/service_clients.hpp"

namespace aegis::sdk {

class HostApiClient {
public:
    explicit HostApiClient(const aegis_host_api_v1_t* api) : api_(api) {}

    [[nodiscard]] bool valid() const {
        return api_ != nullptr && api_->abi_version == AEGIS_HOST_API_ABI_V1;
    }

    int log_write(const std::string& tag, const std::string& message) const {
        return api_->log_write(api_->user_data, tag.c_str(), message.c_str());
    }

    [[nodiscard]] int capability_level(std::uint32_t capability_id) const {
        return api_->get_capability_level(api_->user_data, capability_id);
    }

    void* mem_alloc(std::size_t size, std::size_t alignment = alignof(std::max_align_t)) const {
        return api_->mem_alloc(api_->user_data, size, alignment);
    }

    int mem_free(void* ptr) const {
        return api_->mem_free(api_->user_data, ptr);
    }

    int create_ui_root(const std::string& root_name) const {
        return api_->ui_create_root(api_->user_data, root_name.c_str());
    }

    int destroy_ui_root(const std::string& root_name) const {
        return api_->ui_destroy_root(api_->user_data, root_name.c_str());
    }

    int timer_create(std::uint32_t timeout_ms, bool repeat, int& timer_id) const {
        return api_->timer_create(api_->user_data, timeout_ms, repeat ? 1 : 0, &timer_id);
    }

    int timer_cancel(int timer_id) const {
        return api_->timer_cancel(api_->user_data, timer_id);
    }

    [[nodiscard]] UiServiceClient ui() const { return UiServiceClient(api_); }
    [[nodiscard]] RadioServiceClient radio() const { return RadioServiceClient(api_); }
    [[nodiscard]] GpsServiceClient gps() const { return GpsServiceClient(api_); }
    [[nodiscard]] AudioServiceClient audio() const { return AudioServiceClient(api_); }
    [[nodiscard]] SettingsServiceClient settings() const { return SettingsServiceClient(api_); }
    [[nodiscard]] NotificationServiceClient notification() const {
        return NotificationServiceClient(api_);
    }
    [[nodiscard]] StorageServiceClient storage() const { return StorageServiceClient(api_); }
    [[nodiscard]] PowerServiceClient power() const { return PowerServiceClient(api_); }
    [[nodiscard]] TimeServiceClient time() const { return TimeServiceClient(api_); }
    [[nodiscard]] HostlinkServiceClient hostlink() const { return HostlinkServiceClient(api_); }
    [[nodiscard]] TextInputServiceClient text_input() const { return TextInputServiceClient(api_); }

private:
    const aegis_host_api_v1_t* api_ {nullptr};
};

}  // namespace aegis::sdk
