#include "runtime/loader/compiled_app_contract_registry.hpp"

#include <stdexcept>

#if defined(AEGIS_ZEPHYR_ENABLE_COMPILED_APP_FALLBACK) && \
    (AEGIS_ZEPHYR_ENABLE_COMPILED_APP_FALLBACK == 0)

namespace aegis::runtime {

bool can_resolve_compiled_entry_symbol(const std::string& entry_symbol) {
    static_cast<void>(entry_symbol);
    return false;
}

sdk::AppRuntimeContract resolve_compiled_contract(const std::string& entry_symbol) {
    throw std::runtime_error("compiled app fallback is disabled for entry symbol: " + entry_symbol);
}

}  // namespace aegis::runtime

#else

namespace aegis::apps {
sdk::AppRuntimeContract demo_background_contract();
sdk::AppRuntimeContract demo_hello_contract();
sdk::AppRuntimeContract demo_hostlink_contract();
sdk::AppRuntimeContract demo_info_contract();
}  // namespace aegis::apps

namespace aegis::runtime {

bool can_resolve_compiled_entry_symbol(const std::string& entry_symbol) {
    return entry_symbol == "demo_background_main" || entry_symbol == "demo_hello_main" ||
           entry_symbol == "demo_info_main" ||
           entry_symbol == "demo_hostlink_main";
}

sdk::AppRuntimeContract resolve_compiled_contract(const std::string& entry_symbol) {
    if (entry_symbol == "demo_background_main") {
        return apps::demo_background_contract();
    }
    if (entry_symbol == "demo_hello_main") {
        return apps::demo_hello_contract();
    }
    if (entry_symbol == "demo_info_main") {
        return apps::demo_info_contract();
    }
    if (entry_symbol == "demo_hostlink_main") {
        return apps::demo_hostlink_contract();
    }

    throw std::runtime_error("unknown app entry symbol: " + entry_symbol);
}

}  // namespace aegis::runtime

#endif
