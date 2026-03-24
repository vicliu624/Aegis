#pragma once

#include <vector>

#include "core/app_registry/app_manifest.hpp"

namespace aegis::core {

enum class AppCatalogEntryState {
    Visible,
    Hidden,
    Incompatible,
    Invalid,
};

struct AppCatalogEntry {
    AppDescriptor descriptor;
    AppCatalogEntryState state {AppCatalogEntryState::Visible};
    std::string reason;
    std::vector<std::string> compatibility_notes;
};

class AppCatalog {
public:
    void set_entries(std::vector<AppCatalogEntry> entries);
    [[nodiscard]] const std::vector<AppCatalogEntry>& entries() const;

private:
    std::vector<AppCatalogEntry> entries_;
};

}  // namespace aegis::core
