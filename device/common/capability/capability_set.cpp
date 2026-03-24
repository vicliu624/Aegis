#include "device/common/capability/capability_set.hpp"

namespace aegis::device {

CapabilitySet::CapabilitySet(std::initializer_list<CapabilityDescriptor> descriptors) {
    for (const auto& descriptor : descriptors) {
        set(descriptor);
    }
}

void CapabilitySet::set(CapabilityDescriptor descriptor) {
    descriptors_[descriptor.id] = std::move(descriptor);
}

CapabilityLevel CapabilitySet::level(CapabilityId id) const {
    const auto* descriptor = find(id);
    return descriptor ? descriptor->level : CapabilityLevel::Absent;
}

bool CapabilitySet::has(CapabilityId id, CapabilityLevel minimum) const {
    return static_cast<int>(level(id)) >= static_cast<int>(minimum);
}

const CapabilityDescriptor* CapabilitySet::find(CapabilityId id) const {
    const auto it = descriptors_.find(id);
    return it == descriptors_.end() ? nullptr : &it->second;
}

std::vector<CapabilityDescriptor> CapabilitySet::list() const {
    std::vector<CapabilityDescriptor> result;
    result.reserve(descriptors_.size());
    for (const auto& [_, descriptor] : descriptors_) {
        result.push_back(descriptor);
    }
    return result;
}

std::string_view to_string(CapabilityId id) {
    switch (id) {
        case CapabilityId::Display:
            return "display";
        case CapabilityId::KeyboardInput:
            return "keyboard_input";
        case CapabilityId::PointerInput:
            return "pointer_input";
        case CapabilityId::JoystickInput:
            return "joystick_input";
        case CapabilityId::RadioMessaging:
            return "radio_messaging";
        case CapabilityId::GpsFix:
            return "gps_fix";
        case CapabilityId::AudioOutput:
            return "audio_output";
        case CapabilityId::MicrophoneInput:
            return "microphone_input";
        case CapabilityId::BatteryStatus:
            return "battery_status";
        case CapabilityId::Vibration:
            return "vibration";
        case CapabilityId::RemovableStorage:
            return "removable_storage";
        case CapabilityId::UsbHostlink:
            return "usb_hostlink";
    }

    return "unknown";
}

std::string_view to_string(CapabilityLevel level) {
    switch (level) {
        case CapabilityLevel::Absent:
            return "absent";
        case CapabilityLevel::Degraded:
            return "degraded";
        case CapabilityLevel::Full:
            return "full";
    }

    return "unknown";
}

std::optional<CapabilityId> capability_id_from_string(std::string_view value) {
    static constexpr CapabilityId all[] {
        CapabilityId::Display,
        CapabilityId::KeyboardInput,
        CapabilityId::PointerInput,
        CapabilityId::JoystickInput,
        CapabilityId::RadioMessaging,
        CapabilityId::GpsFix,
        CapabilityId::AudioOutput,
        CapabilityId::MicrophoneInput,
        CapabilityId::BatteryStatus,
        CapabilityId::Vibration,
        CapabilityId::RemovableStorage,
        CapabilityId::UsbHostlink,
    };

    for (const auto id : all) {
        if (to_string(id) == value) {
            return id;
        }
    }

    return std::nullopt;
}

}  // namespace aegis::device
