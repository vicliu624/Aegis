#pragma once

#include <string>

#include "sdk/include/aegis/app_contract.hpp"

namespace aegis::runtime {

[[nodiscard]] bool can_resolve_compiled_entry_symbol(const std::string& entry_symbol);
[[nodiscard]] sdk::AppRuntimeContract resolve_compiled_contract(const std::string& entry_symbol);

}  // namespace aegis::runtime
