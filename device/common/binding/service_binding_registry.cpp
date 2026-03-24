#include "device/common/binding/service_binding_registry.hpp"

namespace aegis::device {

void ServiceBindingRegistry::bind_display(std::shared_ptr<services::IDisplayService> service) {
    display_ = std::move(service);
}

void ServiceBindingRegistry::bind_input(std::shared_ptr<services::IInputService> service) {
    input_ = std::move(service);
}

void ServiceBindingRegistry::bind_radio(std::shared_ptr<services::IRadioService> service) {
    radio_ = std::move(service);
}

void ServiceBindingRegistry::bind_gps(std::shared_ptr<services::IGpsService> service) {
    gps_ = std::move(service);
}

void ServiceBindingRegistry::bind_settings(std::shared_ptr<services::ISettingsService> service) {
    settings_ = std::move(service);
}

void ServiceBindingRegistry::bind_notification(
    std::shared_ptr<services::INotificationService> service) {
    notifications_ = std::move(service);
}

void ServiceBindingRegistry::bind_logging(std::shared_ptr<services::ILoggingService> service) {
    logging_ = std::move(service);
}

std::shared_ptr<services::IDisplayService> ServiceBindingRegistry::display() const {
    return display_;
}

std::shared_ptr<services::IInputService> ServiceBindingRegistry::input() const {
    return input_;
}

std::shared_ptr<services::IRadioService> ServiceBindingRegistry::radio() const {
    return radio_;
}

std::shared_ptr<services::IGpsService> ServiceBindingRegistry::gps() const {
    return gps_;
}

std::shared_ptr<services::ISettingsService> ServiceBindingRegistry::settings() const {
    return settings_;
}

std::shared_ptr<services::INotificationService> ServiceBindingRegistry::notifications() const {
    return notifications_;
}

std::shared_ptr<services::ILoggingService> ServiceBindingRegistry::logging() const {
    return logging_;
}

}  // namespace aegis::device
