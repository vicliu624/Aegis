#pragma once

#include <array>
#include <optional>
#include <string>
#include <vector>

#include "shell/control/shell_navigation_state.hpp"
#include "shell/control/shell_input_model.hpp"

namespace aegis::shell {

struct ShellPresentationLine {
    std::string text;
    bool emphasized {false};
};

enum class ShellPresentationTemplate {
    TextLog,
    SimpleList,
    ValueList,
    FileList,
};

enum class ShellPresentationAccessory {
    None,
    File,
    Folder,
};

enum class ShellPresentationIconSource {
    None,
    Symbol,
    AssetKey,
    AppPackage,
};

struct ShellPresentationItem {
    std::string item_id;
    std::string label;
    std::string detail;
    bool focused {false};
    bool emphasized {false};
    bool warning {false};
    bool disabled {false};
    ShellPresentationAccessory accessory {ShellPresentationAccessory::None};
    bool actionable {false};
    std::string command_id;
    ShellPresentationIconSource icon_source {ShellPresentationIconSource::None};
    std::string icon_key;
};

struct ShellPresentationDialog {
    bool visible {false};
    std::string title;
    std::string body;
};

struct ShellSoftkeySpec {
    enum class Visibility {
        Hidden,
        Visible,
    };

    enum class Enablement {
        Enabled,
        Disabled,
    };

    enum class Role {
        Primary,
        Secondary,
        Back,
        Menu,
        Confirm,
        Info,
        Custom,
    };

    enum class DispatchTarget {
        System,
        Page,
    };

    std::string id;
    std::string label;
    Visibility visibility {Visibility::Visible};
    Enablement enablement {Enablement::Enabled};
    Role role {Role::Custom};
    DispatchTarget target {DispatchTarget::System};
    std::optional<ShellNavigationAction> system_action;
    std::string page_command_id;
    bool emphasized {false};
};

struct ShellPresentationFrame {
    ShellSurface surface {ShellSurface::Home};
    std::string page_id;
    std::string page_state_token;
    std::string headline;
    std::string detail;
    std::string context;
    ShellPresentationTemplate page_template {ShellPresentationTemplate::TextLog};
    std::vector<ShellPresentationLine> lines;
    std::vector<ShellPresentationItem> items;
    ShellPresentationDialog dialog {};
    std::array<ShellSoftkeySpec, 3> softkeys {};
};

class ShellPresentationSink {
public:
    virtual ~ShellPresentationSink() = default;
    virtual void present(const ShellPresentationFrame& frame) = 0;
};

}  // namespace aegis::shell
