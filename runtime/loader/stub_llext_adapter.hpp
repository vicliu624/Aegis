#pragma once

#include "platform/logging/logger.hpp"
#include "runtime/loader/llext_adapter.hpp"

namespace aegis::runtime {

class StubLlextAdapter : public LlextAdapter {
public:
    explicit StubLlextAdapter(platform::Logger& logger);

    AdapterResult prepare_image(const std::string& binary_path) override;
    AdapterResult load_image(const std::string& binary_path, RuntimeImageHandle& image) override;
    AdapterResult resolve_entry(RuntimeImageHandle& image,
                                const std::string& entry_symbol,
                                sdk::AppRuntimeContract& contract) override;
    AdapterResult unload_image(RuntimeImageHandle& image) override;

private:
    platform::Logger& logger_;
};

}  // namespace aegis::runtime
