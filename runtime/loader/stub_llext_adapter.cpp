#include "runtime/loader/stub_llext_adapter.hpp"

#include "runtime/loader/compiled_app_contract_registry.hpp"

namespace aegis::runtime {

StubLlextAdapter::StubLlextAdapter(platform::Logger& logger) : logger_(logger) {}

AdapterResult StubLlextAdapter::prepare_image(const std::string& binary_path) {
    if (binary_path.empty()) {
        return AdapterResult {.ok = false, .detail = "binary path missing"};
    }
    return AdapterResult {.ok = true, .detail = "stub adapter prepared " + binary_path};
}

AdapterResult StubLlextAdapter::load_image(const std::string& binary_path, RuntimeImageHandle& image) {
    image.binary_path = binary_path;
    logger_.info("runtime", "stub llext loaded image " + binary_path);
    return AdapterResult {.ok = true, .detail = "stub image loaded"};
}

AdapterResult StubLlextAdapter::resolve_entry(RuntimeImageHandle& image,
                                              const std::string& entry_symbol,
                                              sdk::AppRuntimeContract& contract) {
    try {
        image.entry_symbol = entry_symbol;
        contract = resolve_compiled_contract(entry_symbol);
        return AdapterResult {.ok = true, .detail = "stub entry resolved"};
    } catch (const std::exception& ex) {
        return AdapterResult {.ok = false, .detail = ex.what()};
    }
}

AdapterResult StubLlextAdapter::unload_image(RuntimeImageHandle& image) {
    logger_.info("runtime", "stub llext unloaded image " + image.binary_path);
    image = RuntimeImageHandle {};
    return AdapterResult {.ok = true, .detail = "stub image unloaded"};
}

}  // namespace aegis::runtime
