#include "services/settings/zephyr_settings_service.hpp"

#include <string>

#include <zephyr/settings/settings.h>

namespace aegis::services {

namespace {

struct ReadContext {
    std::string value;
};

int settings_read_string_cb(const char* key,
                            size_t len,
                            settings_read_cb read_cb,
                            void* cb_arg,
                            void* param) {
    ARG_UNUSED(key);

    auto* context = static_cast<ReadContext*>(param);
    std::string buffer(len, '\0');
    const auto bytes_read = read_cb(cb_arg, buffer.data(), len);
    if (bytes_read > 0) {
        buffer.resize(static_cast<std::size_t>(bytes_read));
        context->value = std::move(buffer);
    }
    return 0;
}

}  // namespace

ZephyrSettingsService::ZephyrSettingsService(std::string namespace_root)
    : namespace_root_(std::move(namespace_root)) {}

void ZephyrSettingsService::set(std::string key, std::string value) {
    const auto fq_key = full_key(key);
    settings_save_one(fq_key.c_str(), value.c_str(), value.size());
}

std::string ZephyrSettingsService::get(const std::string& key) const {
    ReadContext context;
    const auto fq_key = full_key(key);
    settings_load_subtree_direct(fq_key.c_str(), settings_read_string_cb, &context);
    return context.value;
}

std::string ZephyrSettingsService::full_key(const std::string& key) const {
    return namespace_root_ + "/" + key;
}

}  // namespace aegis::services
