#include "services/settings/zephyr_settings_service.hpp"

#include <string>

#include <zephyr/settings/settings.h>

namespace aegis::services {

namespace {

struct ReadContext {
    bool found {false};
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
    if (bytes_read < 0) {
        return bytes_read;
    }
    context->found = true;
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
    const char* payload = value.empty() ? "" : value.data();
    const auto rc = settings_save_one(fq_key.c_str(), payload, value.size());
    if (rc == 0) {
        cache_[std::move(key)] = std::move(value);
    }
}

std::optional<std::string> ZephyrSettingsService::find(const std::string& key) const {
    if (const auto it = cache_.find(key); it != cache_.end()) {
        return it->second;
    }

    ReadContext context;
    const auto fq_key = full_key(key);
    const auto rc = settings_load_subtree_direct(fq_key.c_str(), settings_read_string_cb, &context);
    if (rc != 0 || !context.found) {
        return std::nullopt;
    }

    if (context.found) {
        cache_[key] = context.value;
    }
    return context.value;
}

std::string ZephyrSettingsService::full_key(const std::string& key) const {
    return namespace_root_ + "/" + key;
}

}  // namespace aegis::services
