#pragma once

#include "services/common/service_interfaces.hpp"

namespace aegis::services {

class ZephyrTimeService : public ITimeService {
public:
    [[nodiscard]] bool available() const override;
    [[nodiscard]] std::optional<int64_t> current_unix_ms() const override;
    [[nodiscard]] std::string describe_source() const override;
};

}  // namespace aegis::services
