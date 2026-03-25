#pragma once

#include <string>

#include "services/common/service_interfaces.hpp"

namespace aegis::device {

class MockDisplayService : public services::IDisplayService {
public:
    explicit MockDisplayService(services::DisplayInfo info);
    [[nodiscard]] services::DisplayInfo display_info() const override;
    [[nodiscard]] std::string describe_surface() const override;

private:
    services::DisplayInfo info_;
};

class MockInputService : public services::IInputService {
public:
    explicit MockInputService(services::InputInfo info);
    [[nodiscard]] services::InputInfo input_info() const override;
    [[nodiscard]] std::string describe_input_mode() const override;

private:
    services::InputInfo info_;
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

class MockAudioService : public services::IAudioService {
public:
    MockAudioService(bool output_available, bool input_available, std::string backend_name);
    [[nodiscard]] bool output_available() const override;
    [[nodiscard]] bool input_available() const override;
    [[nodiscard]] std::string backend_name() const override;

private:
    bool output_available_ {false};
    bool input_available_ {false};
    std::string backend_name_;
};

class MockStorageService : public services::IStorageService {
public:
    MockStorageService(bool available, std::string backend_name);
    [[nodiscard]] bool available() const override;
    [[nodiscard]] std::string describe_backend() const override;

private:
    bool available_ {false};
    std::string backend_name_;
};

class MockPowerService : public services::IPowerService {
public:
    MockPowerService(bool battery_present, bool low_power_mode_supported, std::string status);
    [[nodiscard]] bool battery_present() const override;
    [[nodiscard]] bool low_power_mode_supported() const override;
    [[nodiscard]] std::string describe_status() const override;

private:
    bool battery_present_ {false};
    bool low_power_mode_supported_ {false};
    std::string status_;
};

class MockTimeService : public services::ITimeService {
public:
    MockTimeService(bool available, std::string source);
    [[nodiscard]] bool available() const override;
    [[nodiscard]] std::optional<int64_t> current_unix_ms() const override;
    [[nodiscard]] std::string describe_source() const override;

private:
    bool available_ {false};
    std::string source_;
};

class MockHostlinkService : public services::IHostlinkService {
public:
    MockHostlinkService(bool available,
                        bool connected,
                        std::string transport_name,
                        std::string bridge_name);
    [[nodiscard]] bool available() const override;
    [[nodiscard]] bool connected() const override;
    [[nodiscard]] std::string transport_name() const override;
    [[nodiscard]] std::string bridge_name() const override;

private:
    bool available_ {false};
    bool connected_ {false};
    std::string transport_name_;
    std::string bridge_name_;
};

}  // namespace aegis::device
