#include "core/app_registry/app_permissions.hpp"

namespace aegis::core {

std::string_view to_string(AppPermissionId permission) {
    switch (permission) {
        case AppPermissionId::UiSurface:
            return "ui_surface";
        case AppPermissionId::TimerControl:
            return "timer_control";
        case AppPermissionId::SettingsWrite:
            return "settings_write";
        case AppPermissionId::NotificationPost:
            return "notification_post";
        case AppPermissionId::StorageAccess:
            return "storage_access";
        case AppPermissionId::TextInputFocus:
            return "text_input_focus";
        case AppPermissionId::RadioUse:
            return "radio_use";
        case AppPermissionId::GpsUse:
            return "gps_use";
        case AppPermissionId::AudioUse:
            return "audio_use";
        case AppPermissionId::HostlinkUse:
            return "hostlink_use";
    }

    return "unknown";
}

std::optional<AppPermissionId> permission_id_from_string(std::string_view value) {
    static constexpr AppPermissionId all[] {
        AppPermissionId::UiSurface,
        AppPermissionId::TimerControl,
        AppPermissionId::SettingsWrite,
        AppPermissionId::NotificationPost,
        AppPermissionId::StorageAccess,
        AppPermissionId::TextInputFocus,
        AppPermissionId::RadioUse,
        AppPermissionId::GpsUse,
        AppPermissionId::AudioUse,
        AppPermissionId::HostlinkUse,
    };

    for (const auto permission : all) {
        if (to_string(permission) == value) {
            return permission;
        }
    }

    return std::nullopt;
}

}  // namespace aegis::core
