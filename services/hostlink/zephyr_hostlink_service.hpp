#pragma once

#include <string>

#include "services/common/service_interfaces.hpp"

namespace aegis::services {

class ZephyrHostlinkService : public IHostlinkService {
public:
    ZephyrHostlinkService(std::string transport_device_name, std::string bridge_name);

    [[nodiscard]] bool available() const override;
    [[nodiscard]] bool connected() const override;
    [[nodiscard]] std::string transport_name() const override;
    [[nodiscard]] std::string bridge_name() const override;

private:
    std::string transport_device_name_;
    std::string bridge_name_;
};

}  // namespace aegis::services
