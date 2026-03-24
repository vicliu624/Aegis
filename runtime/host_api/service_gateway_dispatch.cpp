#include "runtime/host_api/service_gateway_dispatch.hpp"

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

namespace aegis::runtime {

namespace {

template <typename T>
int copy_struct_result(const T& value, void* output, std::size_t* output_size) {
    if (output_size == nullptr) {
        return AEGIS_HOST_STATUS_INVALID_ARGUMENT;
    }

    const auto required = sizeof(T);
    if (output == nullptr || *output_size < required) {
        *output_size = required;
        return AEGIS_HOST_STATUS_BUFFER_TOO_SMALL;
    }

    std::memcpy(output, &value, required);
    *output_size = required;
    return AEGIS_HOST_STATUS_OK;
}

void copy_text(char* destination, std::size_t capacity, const std::string& value) {
    if (capacity == 0) {
        return;
    }

    const auto count = std::min(capacity - 1, value.size());
    std::memcpy(destination, value.data(), count);
    destination[count] = '\0';
}

uint32_t to_abi_focus_owner(services::TextInputFocusOwner owner) {
    switch (owner) {
        case services::TextInputFocusOwner::None:
            return AEGIS_TEXT_INPUT_FOCUS_OWNER_NONE;
        case services::TextInputFocusOwner::Shell:
            return AEGIS_TEXT_INPUT_FOCUS_OWNER_SHELL;
        case services::TextInputFocusOwner::AppSession:
            return AEGIS_TEXT_INPUT_FOCUS_OWNER_APP_SESSION;
    }

    return AEGIS_TEXT_INPUT_FOCUS_OWNER_NONE;
}

}  // namespace

ServiceGatewayDispatch::ServiceGatewayDispatch(device::ServiceBindingRegistry& services)
    : services_(services) {}

int ServiceGatewayDispatch::call(uint32_t domain,
                                 uint32_t op,
                                 const void* input,
                                 std::size_t input_size,
                                 void* output,
                                 std::size_t* output_size) const {
    switch (domain) {
        case AEGIS_SERVICE_DOMAIN_UI:
            if (op == AEGIS_UI_SERVICE_OP_GET_DISPLAY_INFO) {
                aegis_ui_display_info_v1_t info {};
                const auto display = services_.display()->display_info();
                info.width = static_cast<uint32_t>(display.width);
                info.height = static_cast<uint32_t>(display.height);
                info.touch = display.touch ? 1u : 0u;
                copy_text(info.layout_class, sizeof(info.layout_class), display.layout_class);
                copy_text(info.surface_description,
                          sizeof(info.surface_description),
                          display.surface_description);
                return copy_struct_result(info, output, output_size);
            }
            if (op == AEGIS_UI_SERVICE_OP_GET_INPUT_INFO) {
                aegis_ui_input_info_v1_t info {};
                const auto input_info = services_.input()->input_info();
                info.keyboard = input_info.keyboard ? 1u : 0u;
                info.pointer = input_info.pointer ? 1u : 0u;
                info.joystick = input_info.joystick ? 1u : 0u;
                copy_text(info.primary_input, sizeof(info.primary_input), input_info.primary_input);
                copy_text(info.input_mode, sizeof(info.input_mode), input_info.input_mode);
                return copy_struct_result(info, output, output_size);
            }
            return AEGIS_HOST_STATUS_UNSUPPORTED;

        case AEGIS_SERVICE_DOMAIN_RADIO:
            if (op == AEGIS_RADIO_SERVICE_OP_GET_STATUS) {
                aegis_radio_status_v1_t status {};
                status.available = services_.radio()->available() ? 1u : 0u;
                copy_text(status.backend_name,
                          sizeof(status.backend_name),
                          services_.radio()->backend_name());
                return copy_struct_result(status, output, output_size);
            }
            return AEGIS_HOST_STATUS_UNSUPPORTED;

        case AEGIS_SERVICE_DOMAIN_GPS:
            if (op == AEGIS_GPS_SERVICE_OP_GET_STATUS) {
                aegis_gps_status_v1_t status {};
                status.available = services_.gps()->available() ? 1u : 0u;
                copy_text(status.backend_name, sizeof(status.backend_name), services_.gps()->backend_name());
                return copy_struct_result(status, output, output_size);
            }
            return AEGIS_HOST_STATUS_UNSUPPORTED;

        case AEGIS_SERVICE_DOMAIN_AUDIO:
            if (op == AEGIS_AUDIO_SERVICE_OP_GET_STATUS) {
                aegis_audio_status_v1_t status {};
                status.output_available = services_.audio()->output_available() ? 1u : 0u;
                status.input_available = services_.audio()->input_available() ? 1u : 0u;
                copy_text(status.backend_name,
                          sizeof(status.backend_name),
                          services_.audio()->backend_name());
                return copy_struct_result(status, output, output_size);
            }
            return AEGIS_HOST_STATUS_UNSUPPORTED;

        case AEGIS_SERVICE_DOMAIN_SETTINGS:
            if (op == AEGIS_SETTINGS_SERVICE_OP_SET) {
                if (input == nullptr || input_size != sizeof(aegis_settings_set_request_v1_t)) {
                    return AEGIS_HOST_STATUS_INVALID_ARGUMENT;
                }
                const auto* request = static_cast<const aegis_settings_set_request_v1_t*>(input);
                services_.settings()->set(request->key, request->value);
                return AEGIS_HOST_STATUS_OK;
            }
            if (op == AEGIS_SETTINGS_SERVICE_OP_GET) {
                if (input == nullptr || input_size != sizeof(aegis_settings_get_request_v1_t)) {
                    return AEGIS_HOST_STATUS_INVALID_ARGUMENT;
                }
                const auto* request = static_cast<const aegis_settings_get_request_v1_t*>(input);
                aegis_settings_get_response_v1_t response {};
                const auto value = services_.settings()->get(request->key);
                response.found = value.empty() ? 0u : 1u;
                copy_text(response.value, sizeof(response.value), value);
                return copy_struct_result(response, output, output_size);
            }
            return AEGIS_HOST_STATUS_UNSUPPORTED;

        case AEGIS_SERVICE_DOMAIN_NOTIFICATION:
            if (op == AEGIS_NOTIFICATION_SERVICE_OP_PUBLISH) {
                if (input == nullptr || input_size != sizeof(aegis_notification_publish_request_v1_t)) {
                    return AEGIS_HOST_STATUS_INVALID_ARGUMENT;
                }
                const auto* request = static_cast<const aegis_notification_publish_request_v1_t*>(input);
                services_.notifications()->notify(request->title, request->body);
                return AEGIS_HOST_STATUS_OK;
            }
            return AEGIS_HOST_STATUS_UNSUPPORTED;

        case AEGIS_SERVICE_DOMAIN_STORAGE:
            if (op == AEGIS_STORAGE_SERVICE_OP_GET_STATUS) {
                aegis_storage_status_v1_t status {};
                status.available = services_.storage()->available() ? 1u : 0u;
                copy_text(status.backend_name,
                          sizeof(status.backend_name),
                          services_.storage()->describe_backend());
                return copy_struct_result(status, output, output_size);
            }
            return AEGIS_HOST_STATUS_UNSUPPORTED;

        case AEGIS_SERVICE_DOMAIN_POWER:
            if (op == AEGIS_POWER_SERVICE_OP_GET_STATUS) {
                aegis_power_status_v1_t status {};
                status.battery_present = services_.power()->battery_present() ? 1u : 0u;
                status.low_power_mode_supported =
                    services_.power()->low_power_mode_supported() ? 1u : 0u;
                copy_text(status.status_text,
                          sizeof(status.status_text),
                          services_.power()->describe_status());
                return copy_struct_result(status, output, output_size);
            }
            return AEGIS_HOST_STATUS_UNSUPPORTED;

        case AEGIS_SERVICE_DOMAIN_TIME:
            if (op == AEGIS_TIME_SERVICE_OP_GET_STATUS) {
                aegis_time_status_v1_t status {};
                status.available = services_.time()->available() ? 1u : 0u;
                copy_text(status.source_name,
                          sizeof(status.source_name),
                          services_.time()->describe_source());
                return copy_struct_result(status, output, output_size);
            }
            return AEGIS_HOST_STATUS_UNSUPPORTED;

        case AEGIS_SERVICE_DOMAIN_HOSTLINK:
            if (op == AEGIS_HOSTLINK_SERVICE_OP_GET_STATUS) {
                aegis_hostlink_status_v1_t status {};
                status.available = services_.hostlink()->available() ? 1u : 0u;
                status.connected = services_.hostlink()->connected() ? 1u : 0u;
                copy_text(status.transport_name,
                          sizeof(status.transport_name),
                          services_.hostlink()->transport_name());
                copy_text(status.bridge_name,
                          sizeof(status.bridge_name),
                          services_.hostlink()->bridge_name());
                return copy_struct_result(status, output, output_size);
            }
            return AEGIS_HOST_STATUS_UNSUPPORTED;

        case AEGIS_SERVICE_DOMAIN_TEXT_INPUT:
            if (op == AEGIS_TEXT_INPUT_SERVICE_OP_GET_STATE) {
                aegis_text_input_state_v1_t state {};
                const auto text = services_.text_input()->state();
                state.available = text.available ? 1u : 0u;
                state.text_entry = text.text_entry ? 1u : 0u;
                state.last_key_code = text.last_key_code;
                state.modifier_mask = text.modifier_mask;
                copy_text(state.last_text, sizeof(state.last_text), text.last_text);
                copy_text(state.source_name, sizeof(state.source_name), text.source_name);
                return copy_struct_result(state, output, output_size);
            }
            if (op == AEGIS_TEXT_INPUT_SERVICE_OP_GET_FOCUS_STATE) {
                aegis_text_input_focus_state_v1_t state {};
                const auto focus = services_.text_input()->focus_state();
                state.available = focus.available ? 1u : 0u;
                state.focused = focus.focused ? 1u : 0u;
                state.text_entry = focus.text_entry ? 1u : 0u;
                state.owner = to_abi_focus_owner(focus.owner);
                copy_text(state.owner_session_id,
                          sizeof(state.owner_session_id),
                          focus.owner_session_id);
                copy_text(state.route_name, sizeof(state.route_name), focus.route_name);
                return copy_struct_result(state, output, output_size);
            }
            return AEGIS_HOST_STATUS_UNSUPPORTED;

        default:
            return AEGIS_HOST_STATUS_UNSUPPORTED;
    }
}

}  // namespace aegis::runtime
