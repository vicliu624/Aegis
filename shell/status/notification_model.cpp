#include "shell/status/notification_model.hpp"

namespace aegis::shell {

void NotificationModel::set_entries(std::vector<NotificationEntry> entries) {
    entries_ = std::move(entries);
}

const std::vector<NotificationEntry>& NotificationModel::entries() const {
    return entries_;
}

}  // namespace aegis::shell
