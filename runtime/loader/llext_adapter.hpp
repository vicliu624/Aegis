#pragma once

#include <functional>
#include <memory>
#include <string>

#include "sdk/include/aegis/app_contract.hpp"

namespace aegis::runtime {

struct RuntimeImageHandle {
    std::string binary_path;
    std::string entry_symbol;
    std::string backend_name;
    std::size_t image_size_bytes {0};
    std::uint32_t metadata_crc32 {0};
};

struct AdapterResult {
    bool ok {false};
    std::string detail;
};

class LlextAdapter {
public:
    virtual ~LlextAdapter() = default;

    virtual AdapterResult prepare_image(const std::string& binary_path) = 0;
    virtual AdapterResult load_image(const std::string& binary_path, RuntimeImageHandle& image) = 0;
    virtual AdapterResult resolve_entry(RuntimeImageHandle& image,
                                        const std::string& entry_symbol,
                                        sdk::AppRuntimeContract& contract) = 0;
    virtual AdapterResult unload_image(RuntimeImageHandle& image) = 0;
};

using LlextAdapterPtr = std::unique_ptr<LlextAdapter>;

}  // namespace aegis::runtime
