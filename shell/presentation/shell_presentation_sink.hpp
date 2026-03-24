#pragma once

#include <string>
#include <vector>

#include "shell/control/shell_navigation_state.hpp"

namespace aegis::shell {

struct ShellPresentationLine {
    std::string text;
    bool emphasized {false};
};

struct ShellPresentationFrame {
    ShellSurface surface {ShellSurface::Home};
    std::string headline;
    std::string detail;
    std::string context;
    std::vector<ShellPresentationLine> lines;
};

class ShellPresentationSink {
public:
    virtual ~ShellPresentationSink() = default;
    virtual void present(const ShellPresentationFrame& frame) = 0;
};

}  // namespace aegis::shell
