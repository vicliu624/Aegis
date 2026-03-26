#include "device/common/binding/mock_services.hpp"

namespace aegis::device {

MockDisplayService::MockDisplayService(services::DisplayInfo info) : info_(std::move(info)) {}

services::DisplayInfo MockDisplayService::display_info() const {
    return info_;
}

std::string MockDisplayService::describe_surface() const {
    return info_.surface_description;
}

MockInputService::MockInputService(services::InputInfo info) : info_(std::move(info)) {}

services::InputInfo MockInputService::input_info() const {
    return info_;
}

std::string MockInputService::describe_input_mode() const {
    return info_.input_mode;
}

MockRadioService::MockRadioService(bool available, std::string backend_name)
    : available_(available), backend_name_(std::move(backend_name)) {}

bool MockRadioService::available() const {
    return available_;
}

std::string MockRadioService::backend_name() const {
    return backend_name_;
}

MockGpsService::MockGpsService(bool available, std::string backend_name)
    : available_(available), backend_name_(std::move(backend_name)) {}

bool MockGpsService::available() const {
    return available_;
}

std::string MockGpsService::backend_name() const {
    return backend_name_;
}

MockAudioService::MockAudioService(bool output_available,
                                   bool input_available,
                                   std::string backend_name)
    : output_available_(output_available),
      input_available_(input_available),
      backend_name_(std::move(backend_name)) {}

bool MockAudioService::output_available() const {
    return output_available_;
}

bool MockAudioService::input_available() const {
    return input_available_;
}

std::string MockAudioService::backend_name() const {
    return backend_name_;
}

MockStorageService::MockStorageService(bool available, std::string backend_name)
    : available_(available), backend_name_(std::move(backend_name)) {}

bool MockStorageService::available() const {
    return available_;
}

bool MockStorageService::sd_card_present() const {
    return available_;
}

std::string MockStorageService::mount_root() const {
    return "/";
}

bool MockStorageService::directory_exists(const std::string& path) const {
    return path == "/";
}

std::vector<services::StorageDirectoryEntry> MockStorageService::list_directory(
    const std::string& path) const {
    (void)path;
    return {};
}

std::string MockStorageService::describe_backend() const {
    return backend_name_;
}

MockPowerService::MockPowerService(bool battery_present,
                                   bool low_power_mode_supported,
                                   std::string status)
    : battery_present_(battery_present),
      low_power_mode_supported_(low_power_mode_supported),
      status_(std::move(status)) {}

bool MockPowerService::battery_present() const {
    return battery_present_;
}

bool MockPowerService::low_power_mode_supported() const {
    return low_power_mode_supported_;
}

std::string MockPowerService::describe_status() const {
    return status_;
}

MockTimeService::MockTimeService(bool available, std::string source)
    : available_(available), source_(std::move(source)) {}

bool MockTimeService::available() const {
    return available_;
}

std::optional<int64_t> MockTimeService::current_unix_ms() const {
    return std::nullopt;
}

std::string MockTimeService::describe_source() const {
    return source_;
}

MockHostlinkService::MockHostlinkService(bool available,
                                         bool connected,
                                         std::string transport_name,
                                         std::string bridge_name)
    : available_(available),
      connected_(connected),
      transport_name_(std::move(transport_name)),
      bridge_name_(std::move(bridge_name)) {}

bool MockHostlinkService::available() const {
    return available_;
}

bool MockHostlinkService::connected() const {
    return connected_;
}

std::string MockHostlinkService::transport_name() const {
    return transport_name_;
}

std::string MockHostlinkService::bridge_name() const {
    return bridge_name_;
}

}  // namespace aegis::device
