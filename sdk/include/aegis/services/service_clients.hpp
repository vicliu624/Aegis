#pragma once

#include <algorithm>
#include <cstring>
#include <string>

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

namespace aegis::sdk {

namespace detail {

template <typename Request>
void copy_c_string(char* destination, std::size_t capacity, const std::string& value) {
    if (capacity == 0) {
        return;
    }
    const auto count = std::min(capacity - 1, value.size());
    std::memcpy(destination, value.data(), count);
    destination[count] = '\0';
}

}  // namespace detail

class UiServiceClient {
public:
    explicit UiServiceClient(const aegis_host_api_v1_t* api) : api_(api) {}

    int get_display_info(aegis_ui_display_info_v1_t& out) const {
        std::size_t output_size = sizeof(out);
        return api_->service_call(api_->user_data,
                                  AEGIS_SERVICE_DOMAIN_UI,
                                  AEGIS_UI_SERVICE_OP_GET_DISPLAY_INFO,
                                  nullptr,
                                  0,
                                  &out,
                                  &output_size);
    }

    int get_input_info(aegis_ui_input_info_v1_t& out) const {
        std::size_t output_size = sizeof(out);
        return api_->service_call(api_->user_data,
                                  AEGIS_SERVICE_DOMAIN_UI,
                                  AEGIS_UI_SERVICE_OP_GET_INPUT_INFO,
                                  nullptr,
                                  0,
                                  &out,
                                  &output_size);
    }

    int set_foreground_page(const aegis_ui_foreground_page_v1_t& page) const {
        return api_->service_call(api_->user_data,
                                  AEGIS_SERVICE_DOMAIN_UI,
                                  AEGIS_UI_SERVICE_OP_SET_FOREGROUND_PAGE,
                                  &page,
                                  sizeof(page),
                                  nullptr,
                                  nullptr);
    }

    int poll_event(aegis_ui_routed_event_v1_t& out) const {
        std::size_t output_size = sizeof(out);
        return api_->service_call(api_->user_data,
                                  AEGIS_SERVICE_DOMAIN_UI,
                                  AEGIS_UI_SERVICE_OP_POLL_EVENT,
                                  nullptr,
                                  0,
                                  &out,
                                  &output_size);
    }

private:
    const aegis_host_api_v1_t* api_ {nullptr};
};

class RadioServiceClient {
public:
    explicit RadioServiceClient(const aegis_host_api_v1_t* api) : api_(api) {}

    int get_status(aegis_radio_status_v1_t& out) const {
        std::size_t output_size = sizeof(out);
        return api_->service_call(api_->user_data,
                                  AEGIS_SERVICE_DOMAIN_RADIO,
                                  AEGIS_RADIO_SERVICE_OP_GET_STATUS,
                                  nullptr,
                                  0,
                                  &out,
                                  &output_size);
    }

private:
    const aegis_host_api_v1_t* api_ {nullptr};
};

class GpsServiceClient {
public:
    explicit GpsServiceClient(const aegis_host_api_v1_t* api) : api_(api) {}

    int get_status(aegis_gps_status_v1_t& out) const {
        std::size_t output_size = sizeof(out);
        return api_->service_call(api_->user_data,
                                  AEGIS_SERVICE_DOMAIN_GPS,
                                  AEGIS_GPS_SERVICE_OP_GET_STATUS,
                                  nullptr,
                                  0,
                                  &out,
                                  &output_size);
    }

private:
    const aegis_host_api_v1_t* api_ {nullptr};
};

class AudioServiceClient {
public:
    explicit AudioServiceClient(const aegis_host_api_v1_t* api) : api_(api) {}

    int get_status(aegis_audio_status_v1_t& out) const {
        std::size_t output_size = sizeof(out);
        return api_->service_call(api_->user_data,
                                  AEGIS_SERVICE_DOMAIN_AUDIO,
                                  AEGIS_AUDIO_SERVICE_OP_GET_STATUS,
                                  nullptr,
                                  0,
                                  &out,
                                  &output_size);
    }

private:
    const aegis_host_api_v1_t* api_ {nullptr};
};

class SettingsServiceClient {
public:
    explicit SettingsServiceClient(const aegis_host_api_v1_t* api) : api_(api) {}

    int set(const std::string& key, const std::string& value) const {
        aegis_settings_set_request_v1_t request {};
        detail::copy_c_string<aegis_settings_set_request_v1_t>(request.key, sizeof(request.key), key);
        detail::copy_c_string<aegis_settings_set_request_v1_t>(request.value, sizeof(request.value), value);

        return api_->service_call(api_->user_data,
                                  AEGIS_SERVICE_DOMAIN_SETTINGS,
                                  AEGIS_SETTINGS_SERVICE_OP_SET,
                                  &request,
                                  sizeof(request),
                                  nullptr,
                                  nullptr);
    }

    int get(const std::string& key, aegis_settings_get_response_v1_t& out) const {
        aegis_settings_get_request_v1_t request {};
        detail::copy_c_string<aegis_settings_get_request_v1_t>(request.key, sizeof(request.key), key);
        std::size_t output_size = sizeof(out);
        return api_->service_call(api_->user_data,
                                  AEGIS_SERVICE_DOMAIN_SETTINGS,
                                  AEGIS_SETTINGS_SERVICE_OP_GET,
                                  &request,
                                  sizeof(request),
                                  &out,
                                  &output_size);
    }

private:
    const aegis_host_api_v1_t* api_ {nullptr};
};

class NotificationServiceClient {
public:
    explicit NotificationServiceClient(const aegis_host_api_v1_t* api) : api_(api) {}

    int publish(const std::string& title, const std::string& body) const {
        aegis_notification_publish_request_v1_t request {};
        detail::copy_c_string<aegis_notification_publish_request_v1_t>(request.title,
                                                                       sizeof(request.title),
                                                                       title);
        detail::copy_c_string<aegis_notification_publish_request_v1_t>(request.body,
                                                                       sizeof(request.body),
                                                                       body);

        return api_->service_call(api_->user_data,
                                  AEGIS_SERVICE_DOMAIN_NOTIFICATION,
                                  AEGIS_NOTIFICATION_SERVICE_OP_PUBLISH,
                                  &request,
                                  sizeof(request),
                                  nullptr,
                                  nullptr);
    }

private:
    const aegis_host_api_v1_t* api_ {nullptr};
};

class StorageServiceClient {
public:
    explicit StorageServiceClient(const aegis_host_api_v1_t* api) : api_(api) {}

    int get_status(aegis_storage_status_v1_t& out) const {
        std::size_t output_size = sizeof(out);
        return api_->service_call(api_->user_data,
                                  AEGIS_SERVICE_DOMAIN_STORAGE,
                                  AEGIS_STORAGE_SERVICE_OP_GET_STATUS,
                                  nullptr,
                                  0,
                                  &out,
                                  &output_size);
    }

    int list_directory(const aegis_storage_list_directory_request_v1_t& request,
                       aegis_storage_list_directory_response_v1_t& out) const {
        std::size_t output_size = sizeof(out);
        return api_->service_call(api_->user_data,
                                  AEGIS_SERVICE_DOMAIN_STORAGE,
                                  AEGIS_STORAGE_SERVICE_OP_LIST_DIRECTORY,
                                  &request,
                                  sizeof(request),
                                  &out,
                                  &output_size);
    }

private:
    const aegis_host_api_v1_t* api_ {nullptr};
};

class PowerServiceClient {
public:
    explicit PowerServiceClient(const aegis_host_api_v1_t* api) : api_(api) {}

    int get_status(aegis_power_status_v1_t& out) const {
        std::size_t output_size = sizeof(out);
        return api_->service_call(api_->user_data,
                                  AEGIS_SERVICE_DOMAIN_POWER,
                                  AEGIS_POWER_SERVICE_OP_GET_STATUS,
                                  nullptr,
                                  0,
                                  &out,
                                  &output_size);
    }

private:
    const aegis_host_api_v1_t* api_ {nullptr};
};

class TimeServiceClient {
public:
    explicit TimeServiceClient(const aegis_host_api_v1_t* api) : api_(api) {}

    int get_status(aegis_time_status_v1_t& out) const {
        std::size_t output_size = sizeof(out);
        return api_->service_call(api_->user_data,
                                  AEGIS_SERVICE_DOMAIN_TIME,
                                  AEGIS_TIME_SERVICE_OP_GET_STATUS,
                                  nullptr,
                                  0,
                                  &out,
                                  &output_size);
    }

private:
    const aegis_host_api_v1_t* api_ {nullptr};
};

class HostlinkServiceClient {
public:
    explicit HostlinkServiceClient(const aegis_host_api_v1_t* api) : api_(api) {}

    int get_status(aegis_hostlink_status_v1_t& out) const {
        std::size_t output_size = sizeof(out);
        return api_->service_call(api_->user_data,
                                  AEGIS_SERVICE_DOMAIN_HOSTLINK,
                                  AEGIS_HOSTLINK_SERVICE_OP_GET_STATUS,
                                  nullptr,
                                  0,
                                  &out,
                                  &output_size);
    }

private:
    const aegis_host_api_v1_t* api_ {nullptr};
};

class TextInputServiceClient {
public:
    explicit TextInputServiceClient(const aegis_host_api_v1_t* api) : api_(api) {}

    int get_state(aegis_text_input_state_v1_t& out) const {
        std::size_t output_size = sizeof(out);
        return api_->service_call(api_->user_data,
                                  AEGIS_SERVICE_DOMAIN_TEXT_INPUT,
                                  AEGIS_TEXT_INPUT_SERVICE_OP_GET_STATE,
                                  nullptr,
                                  0,
                                  &out,
                                  &output_size);
    }

    int get_focus_state(aegis_text_input_focus_state_v1_t& out) const {
        std::size_t output_size = sizeof(out);
        return api_->service_call(api_->user_data,
                                  AEGIS_SERVICE_DOMAIN_TEXT_INPUT,
                                  AEGIS_TEXT_INPUT_SERVICE_OP_GET_FOCUS_STATE,
                                  nullptr,
                                  0,
                                  &out,
                                  &output_size);
    }

    int request_focus() const {
        return api_->service_call(api_->user_data,
                                  AEGIS_SERVICE_DOMAIN_TEXT_INPUT,
                                  AEGIS_TEXT_INPUT_SERVICE_OP_REQUEST_FOCUS,
                                  nullptr,
                                  0,
                                  nullptr,
                                  nullptr);
    }

    int release_focus() const {
        return api_->service_call(api_->user_data,
                                  AEGIS_SERVICE_DOMAIN_TEXT_INPUT,
                                  AEGIS_TEXT_INPUT_SERVICE_OP_RELEASE_FOCUS,
                                  nullptr,
                                  0,
                                  nullptr,
                                  nullptr);
    }

private:
    const aegis_host_api_v1_t* api_ {nullptr};
};

}  // namespace aegis::sdk
