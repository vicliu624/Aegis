#pragma once

#include "services/common/service_interfaces.hpp"

namespace aegis::services {

class ConsoleNotificationService : public INotificationService {
public:
    void notify(std::string title, std::string body) override;
};

}  // namespace aegis::services
