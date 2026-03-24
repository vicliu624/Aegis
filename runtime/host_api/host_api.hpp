#pragma once

#include <string>

#include "device/common/binding/service_binding_registry.hpp"
#include "device/common/profile/device_profile.hpp"
#include "runtime/ownership/resource_ownership_table.hpp"

namespace aegis::runtime {

class HostApi {
public:
    HostApi(const device::DeviceProfile& profile,
            device::ServiceBindingRegistry& services,
            ResourceOwnershipTable& ownership,
            std::string session_id);

    void log(const std::string& tag, const std::string& message) const;
    [[nodiscard]] const device::CapabilitySet& capabilities() const;
    [[nodiscard]] std::string describe_display() const;
    [[nodiscard]] std::string describe_input() const;
    [[nodiscard]] std::string describe_radio() const;
    [[nodiscard]] std::string describe_gps() const;
    void create_ui_root(const std::string& name) const;
    void set_setting(const std::string& key, const std::string& value) const;
    [[nodiscard]] std::string get_setting(const std::string& key) const;
    void notify(const std::string& title, const std::string& body) const;

private:
    const device::DeviceProfile& profile_;
    device::ServiceBindingRegistry& services_;
    ResourceOwnershipTable& ownership_;
    std::string session_id_;
};

}  // namespace aegis::runtime
