#pragma once

#include <optional>
#include <string_view>

namespace aegis::core {

enum class AppPermissionId {
    UiSurface,
    TimerControl,
    SettingsWrite,
    NotificationPost,
    StorageAccess,
    TextInputFocus,
    RadioUse,
    GpsUse,
    AudioUse,
    HostlinkUse,
};

[[nodiscard]] std::string_view to_string(AppPermissionId permission);
[[nodiscard]] std::optional<AppPermissionId> permission_id_from_string(std::string_view value);

}  // namespace aegis::core
