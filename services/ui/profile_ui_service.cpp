#include "services/ui/profile_ui_service.hpp"

namespace aegis::services {

ProfileDisplayService::ProfileDisplayService(DisplayInfo info) : info_(std::move(info)) {}

DisplayInfo ProfileDisplayService::display_info() const {
    return info_;
}

std::string ProfileDisplayService::describe_surface() const {
    return info_.surface_description;
}

ProfileInputService::ProfileInputService(InputInfo info) : info_(std::move(info)) {}

InputInfo ProfileInputService::input_info() const {
    return info_;
}

std::string ProfileInputService::describe_input_mode() const {
    return info_.input_mode;
}

}  // namespace aegis::services
