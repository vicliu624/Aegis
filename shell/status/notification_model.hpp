#pragma once

#include <vector>

#include "shell/status/notification_entry.hpp"

namespace aegis::shell {

class NotificationModel {
public:
    void set_entries(std::vector<NotificationEntry> entries);
    [[nodiscard]] const std::vector<NotificationEntry>& entries() const;

private:
    std::vector<NotificationEntry> entries_;
};

}  // namespace aegis::shell
