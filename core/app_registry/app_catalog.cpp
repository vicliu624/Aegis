#include "core/app_registry/app_catalog.hpp"

namespace aegis::core {

void AppCatalog::set_entries(std::vector<AppCatalogEntry> entries) {
    entries_ = std::move(entries);
}

const std::vector<AppCatalogEntry>& AppCatalog::entries() const {
    return entries_;
}

}  // namespace aegis::core
