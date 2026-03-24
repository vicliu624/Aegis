#pragma once

#include <string>
#include <vector>

namespace aegis::services {

class IDisplayService {
public:
    virtual ~IDisplayService() = default;
    [[nodiscard]] virtual std::string describe_surface() const = 0;
};

class IInputService {
public:
    virtual ~IInputService() = default;
    [[nodiscard]] virtual std::string describe_input_mode() const = 0;
};

class IRadioService {
public:
    virtual ~IRadioService() = default;
    [[nodiscard]] virtual bool available() const = 0;
    [[nodiscard]] virtual std::string backend_name() const = 0;
};

class IGpsService {
public:
    virtual ~IGpsService() = default;
    [[nodiscard]] virtual bool available() const = 0;
    [[nodiscard]] virtual std::string backend_name() const = 0;
};

class ISettingsService {
public:
    virtual ~ISettingsService() = default;
    virtual void set(std::string key, std::string value) = 0;
    [[nodiscard]] virtual std::string get(const std::string& key) const = 0;
};

class INotificationService {
public:
    virtual ~INotificationService() = default;
    virtual void notify(std::string title, std::string body) = 0;
};

class ILoggingService {
public:
    virtual ~ILoggingService() = default;
    virtual void log(std::string tag, std::string message) = 0;
};

}  // namespace aegis::services
