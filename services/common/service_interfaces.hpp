#pragma once

#include <optional>
#include <string>
#include <vector>

namespace aegis::services {

struct DisplayInfo {
    int width {0};
    int height {0};
    bool touch {false};
    std::string layout_class;
    std::string surface_description;
};

struct InputInfo {
    bool keyboard {false};
    bool pointer {false};
    bool joystick {false};
    std::string primary_input;
    std::string input_mode;
};

enum class TextInputModifier : uint32_t {
    Shift = 1u << 0,
    CapsLock = 1u << 1,
    Symbol = 1u << 2,
    Alt = 1u << 3,
};

struct TextInputState {
    bool available {false};
    bool text_entry {false};
    uint32_t last_key_code {0};
    uint32_t modifier_mask {0};
    std::string last_text;
    std::string source_name;
};

enum class TextInputFocusOwner {
    None,
    Shell,
    AppSession,
};

struct TextInputFocusState {
    bool available {false};
    bool focused {false};
    bool text_entry {false};
    TextInputFocusOwner owner {TextInputFocusOwner::None};
    std::string owner_session_id;
    std::string route_name;
};

class IDisplayService {
public:
    virtual ~IDisplayService() = default;
    [[nodiscard]] virtual DisplayInfo display_info() const = 0;
    [[nodiscard]] virtual std::string describe_surface() const = 0;
};

class IInputService {
public:
    virtual ~IInputService() = default;
    [[nodiscard]] virtual InputInfo input_info() const = 0;
    [[nodiscard]] virtual std::string describe_input_mode() const = 0;
};

class ITextInputService {
public:
    virtual ~ITextInputService() = default;
    [[nodiscard]] virtual TextInputState state() const = 0;
    [[nodiscard]] virtual TextInputFocusState focus_state() const = 0;
    virtual bool request_focus_for_session(const std::string& session_id) = 0;
    virtual bool release_focus_for_session(const std::string& session_id) = 0;
    virtual void assign_shell_focus() = 0;
    [[nodiscard]] virtual std::string describe_backend() const = 0;
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

class IAudioService {
public:
    virtual ~IAudioService() = default;
    [[nodiscard]] virtual bool output_available() const = 0;
    [[nodiscard]] virtual bool input_available() const = 0;
    [[nodiscard]] virtual std::string backend_name() const = 0;
};

class ISettingsService {
public:
    virtual ~ISettingsService() = default;
    virtual void set(std::string key, std::string value) = 0;
    [[nodiscard]] virtual std::optional<std::string> find(const std::string& key) const = 0;

    [[nodiscard]] std::string get(const std::string& key) const {
        return find(key).value_or("");
    }
};

class ITimerService {
public:
    virtual ~ITimerService() = default;
    virtual int create(uint32_t timeout_ms, bool repeat) = 0;
    virtual bool cancel(int timer_id) = 0;
};

class INotificationService {
public:
    virtual ~INotificationService() = default;
    virtual void notify(std::string title, std::string body) = 0;
};

class IStorageService {
public:
    virtual ~IStorageService() = default;
    [[nodiscard]] virtual bool available() const = 0;
    [[nodiscard]] virtual std::string describe_backend() const = 0;
};

class IPowerService {
public:
    virtual ~IPowerService() = default;
    [[nodiscard]] virtual bool battery_present() const = 0;
    [[nodiscard]] virtual bool low_power_mode_supported() const = 0;
    [[nodiscard]] virtual std::string describe_status() const = 0;
};

class ITimeService {
public:
    virtual ~ITimeService() = default;
    [[nodiscard]] virtual bool available() const = 0;
    [[nodiscard]] virtual std::optional<int64_t> current_unix_ms() const = 0;
    [[nodiscard]] virtual std::string describe_source() const = 0;
};

class IHostlinkService {
public:
    virtual ~IHostlinkService() = default;
    [[nodiscard]] virtual bool available() const = 0;
    [[nodiscard]] virtual bool connected() const = 0;
    [[nodiscard]] virtual std::string transport_name() const = 0;
    [[nodiscard]] virtual std::string bridge_name() const = 0;
};

class ILoggingService {
public:
    virtual ~ILoggingService() = default;
    virtual void log(std::string tag, std::string message) = 0;
};

}  // namespace aegis::services
