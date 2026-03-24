#pragma once

#include <string>

#include "services/common/service_interfaces.hpp"

namespace aegis::services {

class ZephyrRadioService : public IRadioService {
public:
    explicit ZephyrRadioService(std::string device_name);

    [[nodiscard]] bool available() const override;
    [[nodiscard]] std::string backend_name() const override;

private:
    std::string device_name_;
};

}  // namespace aegis::services
