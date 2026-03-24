#pragma once

#include <string>

#include "services/common/service_interfaces.hpp"

namespace aegis::device {

class MockDisplayService : public services::IDisplayService {
public:
    explicit MockDisplayService(std::string description);
    [[nodiscard]] std::string describe_surface() const override;

private:
    std::string description_;
};

class MockInputService : public services::IInputService {
public:
    explicit MockInputService(std::string description);
    [[nodiscard]] std::string describe_input_mode() const override;

private:
    std::string description_;
};

class MockRadioService : public services::IRadioService {
public:
    MockRadioService(bool available, std::string backend_name);
    [[nodiscard]] bool available() const override;
    [[nodiscard]] std::string backend_name() const override;

private:
    bool available_ {false};
    std::string backend_name_;
};

class MockGpsService : public services::IGpsService {
public:
    MockGpsService(bool available, std::string backend_name);
    [[nodiscard]] bool available() const override;
    [[nodiscard]] std::string backend_name() const override;

private:
    bool available_ {false};
    std::string backend_name_;
};

}  // namespace aegis::device
