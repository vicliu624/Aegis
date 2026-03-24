#pragma once

#include "services/common/service_interfaces.hpp"

namespace aegis::services {

class ProfileDisplayService : public IDisplayService {
public:
    explicit ProfileDisplayService(DisplayInfo info);

    [[nodiscard]] DisplayInfo display_info() const override;
    [[nodiscard]] std::string describe_surface() const override;

private:
    DisplayInfo info_;
};

class ProfileInputService : public IInputService {
public:
    explicit ProfileInputService(InputInfo info);

    [[nodiscard]] InputInfo input_info() const override;
    [[nodiscard]] std::string describe_input_mode() const override;

private:
    InputInfo info_;
};

}  // namespace aegis::services
