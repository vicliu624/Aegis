#include "core/app_registry/app_manifest.hpp"

#include <fstream>
#include <regex>
#include <sstream>
#include <stdexcept>

namespace aegis::core {

namespace {

std::string read_file(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("failed to open manifest: " + path.string());
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

std::string match_string(const std::string& text, const std::string& key) {
    const std::regex pattern("\"" + key + "\"\\s*:\\s*\"([^\"]+)\"");
    std::smatch match;
    if (!std::regex_search(text, match, pattern)) {
        throw std::runtime_error("missing string field: " + key);
    }

    return match[1].str();
}

std::vector<device::CapabilityId> match_capabilities(const std::string& text,
                                                     const std::string& key) {
    const std::regex pattern("\"" + key + "\"\\s*:\\s*\\[([^\\]]*)\\]");
    std::smatch match;
    std::vector<device::CapabilityId> ids;
    if (!std::regex_search(text, match, pattern)) {
        return ids;
    }

    const std::string payload = match[1].str();
    const std::regex item_pattern("\"([^\"]+)\"");
    auto begin = std::sregex_iterator(payload.begin(), payload.end(), item_pattern);
    const auto end = std::sregex_iterator();
    for (auto it = begin; it != end; ++it) {
        const auto maybe_id = device::capability_id_from_string((*it)[1].str());
        if (maybe_id) {
            ids.push_back(*maybe_id);
        }
    }

    return ids;
}

}  // namespace

AppManifest ManifestParser::parse_file(const std::filesystem::path& manifest_path) const {
    const std::string text = read_file(manifest_path);

    return AppManifest {
        .app_id = match_string(text, "app_id"),
        .display_name = match_string(text, "display_name"),
        .version = match_string(text, "version"),
        .abi_version = match_string(text, "abi_version"),
        .entry_symbol = match_string(text, "entry_symbol"),
        .description = match_string(text, "description"),
        .required_capabilities = match_capabilities(text, "required_capabilities"),
        .optional_capabilities = match_capabilities(text, "optional_capabilities"),
    };
}

}  // namespace aegis::core
