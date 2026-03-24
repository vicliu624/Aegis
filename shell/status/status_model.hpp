#pragma once

#include <vector>

#include "shell/status/status_item.hpp"

namespace aegis::shell {

class StatusModel {
public:
    void set_items(std::vector<StatusItem> items);
    [[nodiscard]] const std::vector<StatusItem>& items() const;

private:
    std::vector<StatusItem> items_;
};

}  // namespace aegis::shell
