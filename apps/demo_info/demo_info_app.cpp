#include "sdk/include/aegis/app_contract.hpp"

#include <sstream>

#include "device/common/capability/capability_set.hpp"
#include "sdk/include/aegis/host_api_client.hpp"
#include "sdk/include/aegis/services/service_clients.hpp"

namespace {

std::string focus_owner_name(uint32_t owner) {
    switch (owner) {
        case AEGIS_TEXT_INPUT_FOCUS_OWNER_NONE:
            return "none";
        case AEGIS_TEXT_INPUT_FOCUS_OWNER_SHELL:
            return "shell";
        case AEGIS_TEXT_INPUT_FOCUS_OWNER_APP_SESSION:
            return "app_session";
        default:
            return "unknown";
    }
}

aegis::sdk::AppRunResult run_demo_info_abi(const aegis_host_api_v1_t* host) {
    const aegis::sdk::HostApiClient api(host);
    if (!api.valid()) {
        return aegis::sdk::AppRunResult::Failed;
    }

    api.log_write("demo_info_abi", "enter ABI capability inspection app");
    api.create_ui_root("demo_info_abi.root");
    void* scratch = api.mem_alloc(96);
    int timer_id = 0;
    api.timer_create(1000, false, timer_id);

    std::ostringstream capabilities;
    for (std::uint32_t id = static_cast<std::uint32_t>(aegis::device::CapabilityId::Display);
         id <= static_cast<std::uint32_t>(aegis::device::CapabilityId::UsbHostlink);
         ++id) {
        const auto level = api.capability_level(id);
        if (level >= 0) {
            capabilities << aegis::device::to_string(static_cast<aegis::device::CapabilityId>(id))
                         << '='
                         << aegis::device::to_string(
                                static_cast<aegis::device::CapabilityLevel>(level))
                         << "; ";
        }
    }

    api.log_write("demo_info_abi", capabilities.str());

    aegis_radio_status_v1_t radio {};
    const auto radio_status = api.radio().get_status(radio);
    api.log_write("demo_info_abi",
                  std::string("radio:status=") + std::to_string(radio_status) +
                      ", backend=" + radio.backend_name);

    aegis_gps_status_v1_t gps {};
    const auto gps_status = api.gps().get_status(gps);
    api.log_write("demo_info_abi",
                  std::string("gps:status=") + std::to_string(gps_status) +
                      ", backend=" + gps.backend_name);

    aegis_audio_status_v1_t audio {};
    const auto audio_status = api.audio().get_status(audio);
    api.log_write("demo_info_abi",
                  std::string("audio:status=") + std::to_string(audio_status) +
                      ", backend=" + audio.backend_name);

    aegis_ui_display_info_v1_t display {};
    api.ui().get_display_info(display);
    api.log_write("demo_info_abi", display.surface_description);

    aegis_storage_status_v1_t storage {};
    api.storage().get_status(storage);
    api.log_write("demo_info_abi", storage.backend_name);

    aegis_power_status_v1_t power {};
    api.power().get_status(power);
    api.log_write("demo_info_abi", power.status_text);

    aegis_time_status_v1_t time {};
    api.time().get_status(time);
    api.log_write("demo_info_abi", time.source_name);

    aegis_hostlink_status_v1_t hostlink {};
    const auto hostlink_status = api.hostlink().get_status(hostlink);
    api.log_write("demo_info_abi",
                  std::string("hostlink:status=") + std::to_string(hostlink_status) +
                      ", transport=" + hostlink.transport_name + ", bridge=" +
                      hostlink.bridge_name);

    aegis_text_input_state_v1_t text_input {};
    api.text_input().get_state(text_input);
    api.log_write("demo_info_abi",
                  std::string("text_input:src=") + text_input.source_name + ", last=" +
                      text_input.last_text + ", modifiers=" +
                      std::to_string(text_input.modifier_mask));

    aegis_text_input_focus_state_v1_t focus {};
    api.text_input().get_focus_state(focus);
    api.log_write("demo_info_abi",
                  std::string("text_focus:owner=") + focus_owner_name(focus.owner) +
                      ", route=" + focus.route_name + ", session=" + focus.owner_session_id);

    const auto release_status = api.text_input().release_focus();
    api.text_input().get_focus_state(focus);
    api.log_write("demo_info_abi",
                  std::string("text_focus_after_release:status=") +
                      std::to_string(release_status) + ", owner=" + focus_owner_name(focus.owner) +
                      ", route=" + focus.route_name);

    const auto request_status = api.text_input().request_focus();
    api.text_input().get_focus_state(focus);
    api.log_write("demo_info_abi",
                  std::string("text_focus_after_request:status=") +
                      std::to_string(request_status) + ", owner=" + focus_owner_name(focus.owner) +
                      ", route=" + focus.route_name + ", session=" + focus.owner_session_id);

    api.settings().set("demo_info_abi.last_seen", "ok");
    api.notification().publish("Demo Info ABI", "Host API ABI path exercised");
    api.timer_cancel(timer_id);
    api.destroy_ui_root("demo_info_abi.root");
    if (scratch != nullptr) {
        api.mem_free(scratch);
    }

    return aegis::sdk::AppRunResult::Completed;
}

}  // namespace

extern "C" aegis_app_run_result_v1_t demo_info_main(const aegis_host_api_v1_t* host_api) {
    return aegis::sdk::to_abi_result(run_demo_info_abi(host_api));
}
