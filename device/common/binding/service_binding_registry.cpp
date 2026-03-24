#include "device/common/binding/service_binding_registry.hpp"

namespace aegis::device {

void ServiceBindingRegistry::bind_display(std::shared_ptr<services::IDisplayService> service) {
    display_ = std::move(service);
}

void ServiceBindingRegistry::bind_input(std::shared_ptr<services::IInputService> service) {
    input_ = std::move(service);
}

void ServiceBindingRegistry::bind_text_input(std::shared_ptr<services::ITextInputService> service) {
    text_input_ = std::move(service);
}

void ServiceBindingRegistry::bind_radio(std::shared_ptr<services::IRadioService> service) {
    radio_ = std::move(service);
}

void ServiceBindingRegistry::bind_gps(std::shared_ptr<services::IGpsService> service) {
    gps_ = std::move(service);
}

void ServiceBindingRegistry::bind_audio(std::shared_ptr<services::IAudioService> service) {
    audio_ = std::move(service);
}

void ServiceBindingRegistry::bind_settings(std::shared_ptr<services::ISettingsService> service) {
    settings_ = std::move(service);
}

void ServiceBindingRegistry::bind_timer(std::shared_ptr<services::ITimerService> service) {
    timer_ = std::move(service);
}

void ServiceBindingRegistry::bind_notification(
    std::shared_ptr<services::INotificationService> service) {
    notifications_ = std::move(service);
}

void ServiceBindingRegistry::bind_storage(std::shared_ptr<services::IStorageService> service) {
    storage_ = std::move(service);
}

void ServiceBindingRegistry::bind_power(std::shared_ptr<services::IPowerService> service) {
    power_ = std::move(service);
}

void ServiceBindingRegistry::bind_time(std::shared_ptr<services::ITimeService> service) {
    time_ = std::move(service);
}

void ServiceBindingRegistry::bind_hostlink(std::shared_ptr<services::IHostlinkService> service) {
    hostlink_ = std::move(service);
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

std::shared_ptr<services::ITextInputService> ServiceBindingRegistry::text_input() const {
    return text_input_;
}

std::shared_ptr<services::IRadioService> ServiceBindingRegistry::radio() const {
    return radio_;
}

std::shared_ptr<services::IGpsService> ServiceBindingRegistry::gps() const {
    return gps_;
}

std::shared_ptr<services::IAudioService> ServiceBindingRegistry::audio() const {
    return audio_;
}

std::shared_ptr<services::ISettingsService> ServiceBindingRegistry::settings() const {
    return settings_;
}

std::shared_ptr<services::ITimerService> ServiceBindingRegistry::timer() const {
    return timer_;
}

std::shared_ptr<services::INotificationService> ServiceBindingRegistry::notifications() const {
    return notifications_;
}

std::shared_ptr<services::IStorageService> ServiceBindingRegistry::storage() const {
    return storage_;
}

std::shared_ptr<services::IPowerService> ServiceBindingRegistry::power() const {
    return power_;
}

std::shared_ptr<services::ITimeService> ServiceBindingRegistry::time() const {
    return time_;
}

std::shared_ptr<services::IHostlinkService> ServiceBindingRegistry::hostlink() const {
    return hostlink_;
}

std::shared_ptr<services::ILoggingService> ServiceBindingRegistry::logging() const {
    return logging_;
}

}  // namespace aegis::device
