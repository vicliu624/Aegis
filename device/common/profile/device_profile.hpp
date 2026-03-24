#pragma once

#include <string>

#include "device/common/capability/capability_set.hpp"

namespace aegis::device {

struct DisplayTopology {
    int width {0};
    int height {0};
    bool touch {false};
    std::string layout_class;
};

struct InputTopology {
    bool keyboard {false};
    bool pointer {false};
    bool joystick {false};
    std::string primary_input;
};

struct CommTopology {
    bool radio {false};
    bool gps {false};
    bool usb_hostlink {false};
};

struct PowerTopology {
    bool battery_status {false};
    bool low_power_mode {false};
};

struct StorageTopology {
    bool persistent_storage {true};
    bool removable_storage {false};
};

struct ShellHints {
    std::string launcher_style;
    std::string status_density;
};

struct RuntimeLimits {
    std::size_t max_app_memory_bytes {0};
    int max_foreground_apps {1};
};

struct DeviceProfile {
    std::string device_id;
    std::string board_family;
    std::string display_topology_name;
    std::string input_topology_name;

    DisplayTopology display;
    InputTopology input;
    StorageTopology storage;
    PowerTopology power;
    CommTopology comm;

    CapabilitySet capabilities;
    ShellHints shell_hints;
    RuntimeLimits runtime_limits;
};

}  // namespace aegis::device
