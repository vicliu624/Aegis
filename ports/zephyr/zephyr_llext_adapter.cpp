#include "ports/zephyr/zephyr_llext_adapter.hpp"

#include <array>
#include <cstdint>
#include <cstring>
#include <stdexcept>

#include <esp_attr.h>
#include <zephyr/devicetree.h>
#include <zephyr/fs/fs.h>
#include <zephyr/llext/buf_loader.h>
#include <zephyr/llext/llext.h>

#include "ports/zephyr/zephyr_build_config.hpp"
#include "runtime/loader/compiled_app_contract_registry.hpp"

namespace aegis::ports::zephyr {

namespace {

using AppAbiEntrypointRaw = aegis_app_run_result_v1_t (*)(const aegis_host_api_v1_t*);
constexpr std::size_t kWritableImageBufferCapacity = 32U * 1024U;
EXT_RAM_NOINIT_ATTR std::array<std::uint8_t, kWritableImageBufferCapacity> g_writable_image_buffer {};
bool g_writable_image_buffer_reserved = false;

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

std::uint8_t* reserve_writable_image_buffer(std::size_t size, std::string& detail) {
    if (size == 0U) {
        detail = "empty image";
        return nullptr;
    }
    if (size > kWritableImageBufferCapacity) {
        detail = "writable image buffer too small: need " + std::to_string(size) + " bytes, have " +
                 std::to_string(kWritableImageBufferCapacity);
        return nullptr;
    }
    if (g_writable_image_buffer_reserved) {
        detail = "writable image buffer already reserved";
        return nullptr;
    }
    g_writable_image_buffer_reserved = true;
    detail = "psram-reserved";
    return g_writable_image_buffer.data();
}

void release_writable_image_buffer(bool& in_use, std::size_t& size) {
    if (in_use) {
        g_writable_image_buffer_reserved = false;
        in_use = false;
    }
    size = 0;
}

template <typename T>
const T* view_at(const std::uint8_t* image, std::size_t image_size, std::size_t offset) {
    if (image == nullptr || offset > image_size || image_size - offset < sizeof(T)) {
        return nullptr;
    }
    return reinterpret_cast<const T*>(image + offset);
}

const char* view_c_string(const std::uint8_t* image, std::size_t image_size, std::size_t offset) {
    if (image == nullptr || offset >= image_size) {
        return nullptr;
    }
    return reinterpret_cast<const char*>(image + offset);
}

bool map_section_to_runtime_region(const elf_shdr_t& section, llext_mem& region) {
    if ((section.sh_flags & SHF_ALLOC) == 0U) {
        return false;
    }

    if ((section.sh_flags & SHF_EXECINSTR) != 0U) {
        region = LLEXT_MEM_TEXT;
        return true;
    }
    if ((section.sh_flags & SHF_WRITE) != 0U) {
        region = section.sh_type == SHT_NOBITS ? LLEXT_MEM_BSS : LLEXT_MEM_DATA;
        return true;
    }

    region = LLEXT_MEM_RODATA;
    return true;
}

std::size_t align_up(std::size_t value, std::size_t alignment) {
    if (alignment <= 1U) {
        return value;
    }
    const auto remainder = value % alignment;
    return remainder == 0U ? value : value + (alignment - remainder);
}

std::size_t runtime_offset_for_section(const elf_shdr_t* section_headers,
                                       elf_half section_count,
                                       elf_half target_index) {
    if (section_headers == nullptr || target_index >= section_count) {
        return 0U;
    }

    llext_mem target_region {};
    if (!map_section_to_runtime_region(section_headers[target_index], target_region)) {
        return 0U;
    }

    std::size_t offset = 0U;
    for (elf_half index = 0; index < target_index; ++index) {
        llext_mem region {};
        const auto& section = section_headers[index];
        if (!map_section_to_runtime_region(section, region) || region != target_region) {
            continue;
        }

        offset = align_up(offset, section.sh_addralign);
        offset += section.sh_size;
    }

    offset = align_up(offset, section_headers[target_index].sh_addralign);
    return offset;
}

void* runtime_base_for_section(llext* ext, llext_mem region) {
    return ext != nullptr ? ext->mem[region] : nullptr;
}

void* resolve_symbol_from_raw_elf(llext* ext,
                                  const std::string& entry_symbol,
                                  std::size_t image_size) {
    const auto* image = g_writable_image_buffer.data();
    const auto* elf_header = view_at<elf_ehdr_t>(image, image_size, 0U);
    if (ext == nullptr || elf_header == nullptr || elf_header->e_shentsize != sizeof(elf_shdr_t) ||
        elf_header->e_shnum == 0U) {
        return nullptr;
    }

    if (std::memcmp(elf_header->e_ident, "\x7f"
                                     "ELF",
                    4) != 0) {
        return nullptr;
    }

    const auto section_headers_size =
        static_cast<std::size_t>(elf_header->e_shnum) * sizeof(elf_shdr_t);
    if (elf_header->e_shoff > image_size || image_size - elf_header->e_shoff < section_headers_size) {
        return nullptr;
    }

    const auto* section_headers =
        reinterpret_cast<const elf_shdr_t*>(image + static_cast<std::size_t>(elf_header->e_shoff));

    for (elf_half section_index = 0; section_index < elf_header->e_shnum; ++section_index) {
        const auto& symbol_section = section_headers[section_index];
        if (symbol_section.sh_type != SHT_SYMTAB || symbol_section.sh_entsize != sizeof(elf_sym_t) ||
            symbol_section.sh_size == 0U) {
            continue;
        }
        if (symbol_section.sh_offset > image_size ||
            image_size - symbol_section.sh_offset < symbol_section.sh_size) {
            continue;
        }
        if (symbol_section.sh_link >= elf_header->e_shnum) {
            continue;
        }

        const auto& string_section = section_headers[symbol_section.sh_link];
        if (string_section.sh_type != SHT_STRTAB || string_section.sh_size == 0U ||
            string_section.sh_offset > image_size ||
            image_size - string_section.sh_offset < string_section.sh_size) {
            continue;
        }

        const auto symbol_count = symbol_section.sh_size / sizeof(elf_sym_t);
        const auto* symbols = reinterpret_cast<const elf_sym_t*>(
            image + static_cast<std::size_t>(symbol_section.sh_offset));

        for (std::size_t symbol_index = 0; symbol_index < symbol_count; ++symbol_index) {
            const auto& symbol = symbols[symbol_index];
            if (symbol.st_name >= string_section.sh_size || symbol.st_shndx == SHN_UNDEF ||
                symbol.st_shndx >= elf_header->e_shnum || ELF_ST_TYPE(symbol.st_info) != STT_FUNC) {
                continue;
            }

            const auto* symbol_name = view_c_string(image,
                                                    image_size,
                                                    static_cast<std::size_t>(string_section.sh_offset) +
                                                        symbol.st_name);
            if (symbol_name == nullptr || entry_symbol != symbol_name) {
                continue;
            }

            llext_mem region {};
            if (!map_section_to_runtime_region(section_headers[symbol.st_shndx], region)) {
                return nullptr;
            }

            void* const section_base = runtime_base_for_section(ext, region);
            if (section_base == nullptr) {
                return nullptr;
            }

            const auto merged_offset = runtime_offset_for_section(
                section_headers, elf_header->e_shnum, symbol.st_shndx);
            const auto section_relative_value =
                symbol.st_value - section_headers[symbol.st_shndx].sh_addr;
            const auto address = reinterpret_cast<std::uintptr_t>(section_base) + merged_offset +
                                 section_relative_value;
            return reinterpret_cast<void*>(address);
        }
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
    release_writable_image_buffer(loaded_image_uses_reserved_buffer_, loaded_image_size_);
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
    release_writable_image_buffer(loaded_image_uses_reserved_buffer_, loaded_image_size_);

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

    std::string storage_detail;
    auto* image_buffer =
        reserve_writable_image_buffer(static_cast<std::size_t>(entry.size), storage_detail);
    if (image_buffer == nullptr) {
        fs_close(&file);
        return runtime::AdapterResult {
            .ok = false,
            .detail = storage_detail,
        };
    }

    std::uint32_t crc = 2166136261u;
    std::size_t total = 0U;
    const auto expected_size = static_cast<std::size_t>(entry.size);
    while (total < expected_size) {
        const auto bytes = fs_read(&file, image_buffer + total, expected_size - total);
        if (bytes <= 0) {
            fs_close(&file);
            g_writable_image_buffer_reserved = false;
            return runtime::AdapterResult {.ok = false, .detail = "zephyr fs read failed"};
        }
        crc = rolling_crc(
            crc, reinterpret_cast<const char*>(image_buffer + total), static_cast<std::size_t>(bytes));
        total += static_cast<std::size_t>(bytes);
    }
    fs_close(&file);
    loaded_image_uses_reserved_buffer_ = true;
    loaded_image_size_ = total;

    image.binary_path = binary_path;
    image.image_size_bytes = total;
    image.metadata_crc32 = crc;

    struct llext_buf_loader loader = LLEXT_WRITABLE_BUF_LOADER(image_buffer, loaded_image_size_);
    llext_load_param load_params = LLEXT_LOAD_PARAM_DEFAULT;
    llext* ext = nullptr;
    const int load_rc = llext_load(&loader.loader, binary_path.c_str(), &ext, &load_params);
    if (load_rc == 0 && ext != nullptr) {
        loaded_ext_ = ext;
        loaded_via_llext_ = true;
        image.backend_name = "zephyr-llext-writable-buffer";
        logger_.info("runtime",
                     "zephyr llext loaded image path=" + binary_path + " size=" +
                         std::to_string(total) + " crc=" + std::to_string(crc) +
                         " loader=writable-buffer storage=" + storage_detail);
        return runtime::AdapterResult {
            .ok = true,
            .detail = "zephyr llext loaded native extension image from writable reserved buffer",
        };
    }

    loaded_ext_ = nullptr;
    loaded_via_llext_ = false;
    release_writable_image_buffer(loaded_image_uses_reserved_buffer_, loaded_image_size_);
    logger_.info("runtime",
                 "zephyr llext writable-buffer load failed path=" + binary_path +
                     " rc=" + std::to_string(load_rc));
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
        .detail = "zephyr llext writable-buffer load failed rc=" + std::to_string(load_rc) +
                  " and compiled fallback is disabled",
    };
}

runtime::AdapterResult ZephyrLlextAdapter::resolve_entry(runtime::RuntimeImageHandle& image,
                                                         const std::string& entry_symbol,
                                                         aegis::sdk::AppRuntimeContract& contract) {
    image.entry_symbol = entry_symbol;

    if (loaded_via_llext_ && loaded_ext_ != nullptr) {
        auto* ext = static_cast<llext*>(loaded_ext_);
        if (void* symbol = resolve_symbol_from_raw_elf(ext, entry_symbol, loaded_image_size_);
            symbol != nullptr) {
            symbol = normalize_entry_symbol_address(symbol);
            const auto raw = reinterpret_cast<AppAbiEntrypointRaw>(symbol);
            contract = aegis::sdk::AppRuntimeContract {
                .entry_symbol = entry_symbol,
                .entrypoint = nullptr,
                .abi_entrypoint = raw,
            };
            logger_.info("runtime", "zephyr llext resolved native entry symbol via raw elf");
            return runtime::AdapterResult {
                .ok = true,
                .detail = "entry resolved through raw llext elf symbol table",
            };
        }

        logger_.info("runtime", "zephyr llext entry symbol unavailable");
    }

    if constexpr (kCompiledAppFallbackEnabled) {
        try {
            contract = aegis::runtime::resolve_compiled_contract(entry_symbol);
            logger_.info("runtime", "zephyr llext resolved compiled fallback entry");
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
        release_writable_image_buffer(loaded_image_uses_reserved_buffer_, loaded_image_size_);
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
