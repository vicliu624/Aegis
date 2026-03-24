#include "ports/zephyr/zephyr_llext_adapter.hpp"

#include <array>
#include <cstdint>
#include <stdexcept>
#include <vector>

#include <zephyr/devicetree.h>
#include <zephyr/fs/fs.h>
#include <zephyr/llext/buf_loader.h>
#include <zephyr/llext/llext.h>

#include "ports/zephyr/zephyr_build_config.hpp"
#include "runtime/loader/compiled_app_contract_registry.hpp"

namespace aegis::ports::zephyr {

namespace {

using AppAbiEntrypointRaw = aegis_app_run_result_v1_t (*)(const aegis_host_api_v1_t*);

bool fs_path_exists(const std::string& path, fs_dirent& entry) {
    return fs_stat(path.c_str(), &entry) == 0;
}

std::uint32_t rolling_crc(std::uint32_t seed, const char* data, std::size_t size) {
    std::uint32_t value = seed;
    for (std::size_t index = 0; index < size; ++index) {
        value = (value * 16777619u) ^ static_cast<std::uint8_t>(data[index]);
    }
    return value;
}

struct LlextLoadArtifacts {
    llext* ext {nullptr};
};

void destroy_artifacts(LlextLoadArtifacts& artifacts) {
    if (artifacts.ext != nullptr) {
        auto* ext = artifacts.ext;
        llext_unload(&ext);
        artifacts.ext = nullptr;
    }
}

void* find_symbol_address(llext* ext, const std::string& entry_symbol) {
    if (ext == nullptr) {
        return nullptr;
    }

    if (ext->sym_tab.syms != nullptr && ext->sym_tab.sym_cnt > 0U) {
        for (std::size_t index = 0; index < ext->sym_tab.sym_cnt; ++index) {
            const auto& sym = ext->sym_tab.syms[index];
            if (sym.name != nullptr && entry_symbol == sym.name) {
                return sym.addr;
            }
        }
    }

    if (const void* exported = llext_find_sym(&ext->exp_tab, entry_symbol.c_str());
        exported != nullptr) {
        return const_cast<void*>(exported);
    }

    return nullptr;
}

void* normalize_entry_symbol_address(void* symbol_address) {
#if defined(CONFIG_SOC_SERIES_ESP32S3)
    constexpr std::uintptr_t kSram0IramStart = DT_REG_ADDR(DT_NODELABEL(sram0));
    constexpr std::uintptr_t kSram0Size = DT_REG_SIZE(DT_NODELABEL(sram0));
    constexpr std::uintptr_t kSram1DramStart = DT_REG_ADDR(DT_NODELABEL(sram1));
    constexpr std::uintptr_t kSram1DramEnd = kSram1DramStart + DT_REG_SIZE(DT_NODELABEL(sram1));
    constexpr std::uintptr_t kSram1IramStart = kSram0IramStart + kSram0Size;
    constexpr std::uintptr_t kIramDramOffset = kSram1IramStart - kSram1DramStart;
    const auto addr = reinterpret_cast<std::uintptr_t>(symbol_address);
    if (addr >= kSram1DramStart && addr < kSram1DramEnd) {
        return reinterpret_cast<void*>(addr + kIramDramOffset);
    }
#endif
    return symbol_address;
}

}  // namespace

ZephyrLlextAdapter::ZephyrLlextAdapter(platform::Logger& logger) : logger_(logger) {}

ZephyrLlextAdapter::~ZephyrLlextAdapter() {
    LlextLoadArtifacts artifacts {
        .ext = static_cast<llext*>(loaded_ext_),
    };
    destroy_artifacts(artifacts);
    loaded_image_bytes_.clear();
}

runtime::AdapterResult ZephyrLlextAdapter::prepare_image(const std::string& binary_path) {
    if (binary_path.empty()) {
        return runtime::AdapterResult {.ok = false, .detail = "binary path missing"};
    }

    fs_dirent entry {};
    if (!fs_path_exists(binary_path, entry)) {
        return runtime::AdapterResult {.ok = false, .detail = "zephyr fs cannot stat binary"};
    }

    logger_.info("runtime",
                 "zephyr llext prepared binary metadata path=" + binary_path + " size=" +
                     std::to_string(entry.size));
    return runtime::AdapterResult {
        .ok = true,
        .detail = "zephyr adapter prepared binary through zephyr fs substrate",
    };
}

runtime::AdapterResult ZephyrLlextAdapter::load_image(const std::string& binary_path,
                                                      runtime::RuntimeImageHandle& image) {
    fs_file_t file;
    fs_file_t_init(&file);
    if (fs_open(&file, binary_path.c_str(), FS_O_READ) != 0) {
        return runtime::AdapterResult {.ok = false, .detail = "zephyr fs open failed"};
    }

    fs_dirent entry {};
    if (!fs_path_exists(binary_path, entry)) {
        fs_close(&file);
        return runtime::AdapterResult {.ok = false, .detail = "zephyr fs cannot stat binary"};
    }

    loaded_image_bytes_.assign(static_cast<std::size_t>(entry.size), 0U);
    std::uint32_t crc = 2166136261u;
    std::size_t total = 0U;
    while (total < loaded_image_bytes_.size()) {
        const auto remaining = loaded_image_bytes_.size() - total;
        const auto bytes =
            fs_read(&file, loaded_image_bytes_.data() + total, remaining);
        if (bytes <= 0) {
            fs_close(&file);
            loaded_image_bytes_.clear();
            return runtime::AdapterResult {.ok = false, .detail = "zephyr fs read failed"};
        }
        crc = rolling_crc(crc,
                          reinterpret_cast<const char*>(loaded_image_bytes_.data() + total),
                          static_cast<std::size_t>(bytes));
        total += static_cast<std::size_t>(bytes);
    }
    fs_close(&file);

    image.binary_path = binary_path;
    image.image_size_bytes = total;
    image.metadata_crc32 = crc;

    struct llext_buf_loader loader =
        LLEXT_WRITABLE_BUF_LOADER(loaded_image_bytes_.data(), loaded_image_bytes_.size());
    llext_load_param load_params = LLEXT_LOAD_PARAM_DEFAULT;
    llext* ext = nullptr;
    if (llext_load(&loader.loader, binary_path.c_str(), &ext, &load_params) == 0 &&
        ext != nullptr) {
        loaded_ext_ = ext;
        loaded_via_llext_ = true;
        image.backend_name = "zephyr-llext-buffer";
        logger_.info("runtime",
                     "zephyr llext loaded image path=" + binary_path + " size=" +
                         std::to_string(total) + " crc=" + std::to_string(crc) +
                         " loader=writable-buffer");
        return runtime::AdapterResult {
            .ok = true,
            .detail = "zephyr llext loaded native extension image from writable buffer",
        };
    }

    loaded_ext_ = nullptr;
    loaded_image_bytes_.clear();
    loaded_via_llext_ = false;
    if constexpr (kCompiledAppFallbackEnabled) {
        image.backend_name = "zephyr-fallback-registry";
        logger_.info("runtime",
                     "zephyr llext load failed for " + binary_path +
                         ", using compiled contract fallback during bootstrap");
        return runtime::AdapterResult {
            .ok = true,
            .detail = "zephyr llext load failed; fallback contract path retained for bootstrap",
        };
    }

    image.backend_name = "zephyr-llext-required";
    return runtime::AdapterResult {
        .ok = false,
        .detail = "zephyr llext load failed and compiled fallback is disabled",
    };
}

runtime::AdapterResult ZephyrLlextAdapter::resolve_entry(runtime::RuntimeImageHandle& image,
                                                         const std::string& entry_symbol,
                                                         aegis::sdk::AppRuntimeContract& contract) {
    image.entry_symbol = entry_symbol;

    if (loaded_via_llext_ && loaded_ext_ != nullptr) {
        auto* ext = static_cast<llext*>(loaded_ext_);
        if (void* symbol = find_symbol_address(ext, entry_symbol); symbol != nullptr) {
            symbol = normalize_entry_symbol_address(symbol);
            const auto raw = reinterpret_cast<AppAbiEntrypointRaw>(symbol);
            contract = aegis::sdk::AppRuntimeContract {
                .entry_symbol = entry_symbol,
                .entrypoint = nullptr,
                .abi_entrypoint = raw,
            };
            logger_.info("runtime",
                         "zephyr llext resolved native symbol=" + entry_symbol + " backend=" +
                             image.backend_name + " entry=" +
                             std::to_string(reinterpret_cast<std::uintptr_t>(symbol)));
            return runtime::AdapterResult {
                .ok = true,
                .detail = "entry resolved through zephyr llext symbol table",
            };
        }

        logger_.info("runtime",
                     "zephyr llext did not expose entry symbol=" + entry_symbol +
                         ", falling back to compiled registry");
    }

    if constexpr (kCompiledAppFallbackEnabled) {
        try {
            contract = aegis::runtime::resolve_compiled_contract(entry_symbol);
            logger_.info("runtime",
                         "zephyr llext resolved entry symbol=" + entry_symbol + " backend=" +
                             image.backend_name);
            return runtime::AdapterResult {
                .ok = true,
                .detail = "entry resolved through zephyr adapter contract registry",
            };
        } catch (const std::exception& ex) {
            return runtime::AdapterResult {.ok = false, .detail = ex.what()};
        }
    }

    return runtime::AdapterResult {
        .ok = false,
        .detail = "entry symbol unavailable in native llext image and compiled fallback is disabled",
    };
}

runtime::AdapterResult ZephyrLlextAdapter::unload_image(runtime::RuntimeImageHandle& image) {
    if (loaded_via_llext_) {
        LlextLoadArtifacts artifacts {
            .ext = static_cast<llext*>(loaded_ext_),
        };
        destroy_artifacts(artifacts);
        loaded_ext_ = nullptr;
        loaded_image_bytes_.clear();
        loaded_via_llext_ = false;
    }

    logger_.info("runtime",
                 "zephyr llext unloaded image path=" + image.binary_path + " size=" +
                     std::to_string(image.image_size_bytes));
    image = runtime::RuntimeImageHandle {};
    return runtime::AdapterResult {
        .ok = true,
        .detail = "zephyr adapter released image bookkeeping",
    };
}

}  // namespace aegis::ports::zephyr
