#include "runtime/host_api/host_api.hpp"

namespace aegis::runtime {

HostApi::HostApi(const device::DeviceProfile& profile,
                 device::ServiceBindingRegistry& services,
                 ResourceOwnershipTable& ownership,
                 std::string session_id)
    : profile_(profile),
      services_(services),
      ownership_(ownership),
      session_id_(std::move(session_id)) {}

void HostApi::log(const std::string& tag, const std::string& message) const {
    services_.logging()->log(tag, message);
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

void HostApi::create_ui_root(const std::string& name) const {
    ownership_.track(session_id_, "ui_root:" + name);
}

void HostApi::set_setting(const std::string& key, const std::string& value) const {
    services_.settings()->set(key, value);
}

std::string HostApi::get_setting(const std::string& key) const {
    return services_.settings()->get(key);
}

void HostApi::notify(const std::string& title, const std::string& body) const {
    ownership_.track(session_id_, "notification:" + title);
    services_.notifications()->notify(title, body);
}

}  // namespace aegis::runtime
