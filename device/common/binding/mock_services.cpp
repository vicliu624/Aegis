#include "device/common/binding/mock_services.hpp"

namespace aegis::device {

MockDisplayService::MockDisplayService(std::string description)
    : description_(std::move(description)) {}

std::string MockDisplayService::describe_surface() const {
    return description_;
}

MockInputService::MockInputService(std::string description)
    : description_(std::move(description)) {}

std::string MockInputService::describe_input_mode() const {
    return description_;
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

}  // namespace aegis::device
