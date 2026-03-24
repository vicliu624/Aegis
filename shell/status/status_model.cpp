#include "shell/status/status_model.hpp"

namespace aegis::shell {

void StatusModel::set_items(std::vector<StatusItem> items) {
    items_ = std::move(items);
}

const std::vector<StatusItem>& StatusModel::items() const {
    return items_;
}

}  // namespace aegis::shell
