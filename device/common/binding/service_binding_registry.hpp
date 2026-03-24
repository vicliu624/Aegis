#pragma once

#include <memory>

#include "services/common/service_interfaces.hpp"

namespace aegis::device {

class ServiceBindingRegistry {
public:
    void bind_display(std::shared_ptr<services::IDisplayService> service);
    void bind_input(std::shared_ptr<services::IInputService> service);
    void bind_text_input(std::shared_ptr<services::ITextInputService> service);
    void bind_radio(std::shared_ptr<services::IRadioService> service);
    void bind_gps(std::shared_ptr<services::IGpsService> service);
    void bind_audio(std::shared_ptr<services::IAudioService> service);
    void bind_settings(std::shared_ptr<services::ISettingsService> service);
    void bind_timer(std::shared_ptr<services::ITimerService> service);
    void bind_notification(std::shared_ptr<services::INotificationService> service);
    void bind_storage(std::shared_ptr<services::IStorageService> service);
    void bind_power(std::shared_ptr<services::IPowerService> service);
    void bind_time(std::shared_ptr<services::ITimeService> service);
    void bind_hostlink(std::shared_ptr<services::IHostlinkService> service);
    void bind_logging(std::shared_ptr<services::ILoggingService> service);

    [[nodiscard]] std::shared_ptr<services::IDisplayService> display() const;
    [[nodiscard]] std::shared_ptr<services::IInputService> input() const;
    [[nodiscard]] std::shared_ptr<services::ITextInputService> text_input() const;
    [[nodiscard]] std::shared_ptr<services::IRadioService> radio() const;
    [[nodiscard]] std::shared_ptr<services::IGpsService> gps() const;
    [[nodiscard]] std::shared_ptr<services::IAudioService> audio() const;
    [[nodiscard]] std::shared_ptr<services::ISettingsService> settings() const;
    [[nodiscard]] std::shared_ptr<services::ITimerService> timer() const;
    [[nodiscard]] std::shared_ptr<services::INotificationService> notifications() const;
    [[nodiscard]] std::shared_ptr<services::IStorageService> storage() const;
    [[nodiscard]] std::shared_ptr<services::IPowerService> power() const;
    [[nodiscard]] std::shared_ptr<services::ITimeService> time() const;
    [[nodiscard]] std::shared_ptr<services::IHostlinkService> hostlink() const;
    [[nodiscard]] std::shared_ptr<services::ILoggingService> logging() const;

private:
    std::shared_ptr<services::IDisplayService> display_;
    std::shared_ptr<services::IInputService> input_;
    std::shared_ptr<services::ITextInputService> text_input_;
    std::shared_ptr<services::IRadioService> radio_;
    std::shared_ptr<services::IGpsService> gps_;
    std::shared_ptr<services::IAudioService> audio_;
    std::shared_ptr<services::ISettingsService> settings_;
    std::shared_ptr<services::ITimerService> timer_;
    std::shared_ptr<services::INotificationService> notifications_;
    std::shared_ptr<services::IStorageService> storage_;
    std::shared_ptr<services::IPowerService> power_;
    std::shared_ptr<services::ITimeService> time_;
    std::shared_ptr<services::IHostlinkService> hostlink_;
    std::shared_ptr<services::ILoggingService> logging_;
};

}  // namespace aegis::device
