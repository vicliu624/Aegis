#include "services/settings/zephyr_settings_service.hpp"

#include <optional>
#include <string>
#include <unordered_map>

#include <zephyr/settings/settings.h>
#include <zephyr/sys/printk.h>

namespace aegis::services {

namespace {

using SharedCache = std::unordered_map<std::string, std::optional<std::string>>;

struct ReadContext {
    bool found {false};
    std::string value;
};

SharedCache& shared_cache() {
    static SharedCache cache;
    return cache;
}

bool& backend_query_allowed() {
    static bool allowed = true;
    return allowed;
}

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
    printk("[aegis][settings-service] set key=%s value=%s\n", fq_key.c_str(), value.c_str());
    const char* payload = value.empty() ? "" : value.data();
    const auto rc = settings_save_one(fq_key.c_str(), payload, value.size());
    printk("[aegis][settings-service] set result key=%s rc=%d\n", fq_key.c_str(), rc);
    shared_cache()[key] = value;
    if (rc == 0) {
        backend_query_allowed() = true;
    }
}

std::optional<std::string> ZephyrSettingsService::find(const std::string& key) const {
    if (const auto it = shared_cache().find(key); it != shared_cache().end()) {
        return it->second;
    }
    if (!backend_query_allowed()) {
        return std::nullopt;
    }

    ReadContext context;
    const auto fq_key = full_key(key);
    printk("[aegis][settings-service] find key=%s\n", fq_key.c_str());
    const auto rc = settings_load_subtree_direct(fq_key.c_str(), settings_read_string_cb, &context);
    printk("[aegis][settings-service] find result key=%s rc=%d found=%d len=%u\n",
           fq_key.c_str(),
           rc,
           context.found ? 1 : 0,
           static_cast<unsigned int>(context.value.size()));
    if (rc != 0 || !context.found) {
        if (rc != 0) {
            backend_query_allowed() = false;
        }
        shared_cache()[key] = std::nullopt;
        return std::nullopt;
    }

    shared_cache()[key] = context.value;
    return context.value;
}

std::string ZephyrSettingsService::full_key(const std::string& key) const {
    return namespace_root_ + "/" + key;
}

}  // namespace aegis::services
