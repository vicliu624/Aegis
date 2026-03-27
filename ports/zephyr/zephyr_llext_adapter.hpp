#pragma once

#include <cstdint>
#include <memory>

#include "platform/logging/logger.hpp"
#include "runtime/loader/llext_adapter.hpp"

namespace aegis::ports::zephyr {

class ZephyrLlextAdapter : public runtime::LlextAdapter {
public:
    explicit ZephyrLlextAdapter(platform::Logger& logger);
    ~ZephyrLlextAdapter() override;

    runtime::AdapterResult prepare_image(const std::string& binary_path) override;
    runtime::AdapterResult load_image(const std::string& binary_path,
                                      runtime::RuntimeImageHandle& image) override;
    runtime::AdapterResult resolve_entry(runtime::RuntimeImageHandle& image,
                                         const std::string& entry_symbol,
                                         aegis::sdk::AppRuntimeContract& contract) override;
    runtime::AdapterResult unload_image(runtime::RuntimeImageHandle& image) override;

private:
    platform::Logger& logger_;
    void* loaded_ext_ {nullptr};
    std::unique_ptr<std::uint8_t[]> loaded_image_buffer_ {};
    std::size_t loaded_image_size_ {0};
    bool loaded_via_llext_ {false};
};

}  // namespace aegis::ports::zephyr
