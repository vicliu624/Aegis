#pragma once

#include <cstdint>
#include <initializer_list>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace aegis::device {

enum class CapabilityId {
    Display,
    KeyboardInput,
    PointerInput,
    JoystickInput,
    RadioMessaging,
    GpsFix,
    AudioOutput,
    MicrophoneInput,
    BatteryStatus,
    Vibration,
    RemovableStorage,
    UsbHostlink,
};

enum class CapabilityLevel {
    Absent,
    Degraded,
    Full,
};

struct CapabilityDescriptor {
    CapabilityId id {};
    CapabilityLevel level {CapabilityLevel::Absent};
    uint32_t flags {0};
    std::string detail;
};

class CapabilitySet {
public:
    CapabilitySet() = default;
    CapabilitySet(std::initializer_list<CapabilityDescriptor> descriptors);

    void set(CapabilityDescriptor descriptor);
    [[nodiscard]] CapabilityLevel level(CapabilityId id) const;
    [[nodiscard]] bool has(CapabilityId id,
                           CapabilityLevel minimum = CapabilityLevel::Degraded) const;
    [[nodiscard]] const CapabilityDescriptor* find(CapabilityId id) const;
    [[nodiscard]] std::vector<CapabilityDescriptor> list() const;

private:
    std::map<CapabilityId, CapabilityDescriptor> descriptors_;
};

[[nodiscard]] std::string_view to_string(CapabilityId id);
[[nodiscard]] std::string_view to_string(CapabilityLevel level);
[[nodiscard]] std::optional<CapabilityId> capability_id_from_string(std::string_view value);

}  // namespace aegis::device
