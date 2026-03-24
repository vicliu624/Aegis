#pragma once

#include "services/common/service_interfaces.hpp"

namespace aegis::services {

class ZephyrPowerService : public IPowerService {
public:
    [[nodiscard]] bool battery_present() const override;
    [[nodiscard]] bool low_power_mode_supported() const override;
    [[nodiscard]] std::string describe_status() const override;
};

}  // namespace aegis::services
