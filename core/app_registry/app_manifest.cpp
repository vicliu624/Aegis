#include "core/app_registry/app_manifest.hpp"

#include <cctype>
#include <charconv>
#include <optional>
#include <set>
#include <string_view>

#include "sdk/include/aegis/abi.h"

namespace aegis::core {

namespace {

enum class JsonValueType {
    String,
    Number,
    Bool,
    Object,
    Array,
    Null,
};

struct JsonValue {
    JsonValueType type {JsonValueType::Null};
    std::string string_value;
    std::uint64_t number_value {0};
    bool bool_value {false};
    std::vector<std::pair<std::string, JsonValue>> object_fields;
    std::vector<JsonValue> array_items;
};

class JsonParser {
public:
    explicit JsonParser(std::string_view text) : text_(text) {}

    std::optional<JsonValue> parse(std::vector<AppManifestValidationIssue>& issues) {
        skip_whitespace();
        auto value = parse_value(issues);
        skip_whitespace();
        if (!value) {
            return std::nullopt;
        }
        if (index_ != text_.size()) {
            issues.push_back({.field = "manifest", .message = "unexpected trailing content"});
            return std::nullopt;
        }
        return value;
    }

private:
    [[nodiscard]] bool eof() const {
        return index_ >= text_.size();
    }

    [[nodiscard]] char peek() const {
        return eof() ? '\0' : text_[index_];
    }

    char consume() {
        return eof() ? '\0' : text_[index_++];
    }

    void skip_whitespace() {
        while (!eof() && std::isspace(static_cast<unsigned char>(peek())) != 0) {
            ++index_;
        }
    }

    std::optional<JsonValue> parse_value(std::vector<AppManifestValidationIssue>& issues) {
        skip_whitespace();
        if (eof()) {
            issues.push_back({.field = "manifest", .message = "unexpected end of input"});
            return std::nullopt;
        }

        switch (peek()) {
            case '"':
                return parse_string(issues);
            case '{':
                return parse_object(issues);
            case '[':
                return parse_array(issues);
            case 't':
            case 'f':
                return parse_bool(issues);
            case 'n':
                return parse_null(issues);
            default:
                if (std::isdigit(static_cast<unsigned char>(peek())) != 0) {
                    return parse_number(issues);
                }
                issues.push_back(
                    {.field = "manifest", .message = std::string("unexpected character: ") + peek()});
                return std::nullopt;
        }
    }

    std::optional<JsonValue> parse_string(std::vector<AppManifestValidationIssue>& issues) {
        if (consume() != '"') {
            issues.push_back({.field = "manifest", .message = "expected string"});
            return std::nullopt;
        }

        std::string value;
        while (!eof()) {
            const char ch = consume();
            if (ch == '"') {
                return JsonValue {.type = JsonValueType::String, .string_value = std::move(value)};
            }
            if (ch == '\\') {
                if (eof()) {
                    break;
                }
                const char escaped = consume();
                switch (escaped) {
                    case '"':
                    case '\\':
                    case '/':
                        value.push_back(escaped);
                        break;
                    case 'b':
                        value.push_back('\b');
                        break;
                    case 'f':
                        value.push_back('\f');
                        break;
                    case 'n':
                        value.push_back('\n');
                        break;
                    case 'r':
                        value.push_back('\r');
                        break;
                    case 't':
                        value.push_back('\t');
                        break;
                    default:
                        issues.push_back({.field = "manifest", .message = "unsupported string escape"});
                        return std::nullopt;
                }
                continue;
            }
            value.push_back(ch);
        }

        issues.push_back({.field = "manifest", .message = "unterminated string"});
        return std::nullopt;
    }

    std::optional<JsonValue> parse_number(std::vector<AppManifestValidationIssue>& issues) {
        const std::size_t begin = index_;
        while (!eof() && std::isdigit(static_cast<unsigned char>(peek())) != 0) {
            ++index_;
        }
        const auto token = text_.substr(begin, index_ - begin);
        std::uint64_t value = 0;
        const auto result = std::from_chars(token.data(), token.data() + token.size(), value);
        if (result.ec != std::errc()) {
            issues.push_back({.field = "manifest", .message = "invalid number"});
            return std::nullopt;
        }
        return JsonValue {.type = JsonValueType::Number, .number_value = value};
    }

    std::optional<JsonValue> parse_bool(std::vector<AppManifestValidationIssue>& issues) {
        if (text_.substr(index_, 4) == "true") {
            index_ += 4;
            return JsonValue {.type = JsonValueType::Bool, .bool_value = true};
        }
        if (text_.substr(index_, 5) == "false") {
            index_ += 5;
            return JsonValue {.type = JsonValueType::Bool, .bool_value = false};
        }
        issues.push_back({.field = "manifest", .message = "invalid boolean"});
        return std::nullopt;
    }

    std::optional<JsonValue> parse_null(std::vector<AppManifestValidationIssue>& issues) {
        if (text_.substr(index_, 4) == "null") {
            index_ += 4;
            return JsonValue {.type = JsonValueType::Null};
        }
        issues.push_back({.field = "manifest", .message = "invalid null"});
        return std::nullopt;
    }

    std::optional<JsonValue> parse_array(std::vector<AppManifestValidationIssue>& issues) {
        consume();
        JsonValue value {.type = JsonValueType::Array};
        skip_whitespace();
        if (peek() == ']') {
            consume();
            return value;
        }

        while (!eof()) {
            auto item = parse_value(issues);
            if (!item) {
                return std::nullopt;
            }
            value.array_items.push_back(std::move(*item));
            skip_whitespace();
            if (peek() == ',') {
                consume();
                skip_whitespace();
                continue;
            }
            if (peek() == ']') {
                consume();
                return value;
            }
            break;
        }

        issues.push_back({.field = "manifest", .message = "unterminated array"});
        return std::nullopt;
    }

    std::optional<JsonValue> parse_object(std::vector<AppManifestValidationIssue>& issues) {
        consume();
        JsonValue value {.type = JsonValueType::Object};
        skip_whitespace();
        if (peek() == '}') {
            consume();
            return value;
        }

        while (!eof()) {
            auto key = parse_string(issues);
            if (!key) {
                return std::nullopt;
            }
            skip_whitespace();
            if (consume() != ':') {
                issues.push_back({.field = "manifest", .message = "expected ':' in object"});
                return std::nullopt;
            }
            auto field_value = parse_value(issues);
            if (!field_value) {
                return std::nullopt;
            }
            value.object_fields.emplace_back(key->string_value, std::move(*field_value));
            skip_whitespace();
            if (peek() == ',') {
                consume();
                skip_whitespace();
                continue;
            }
            if (peek() == '}') {
                consume();
                return value;
            }
            break;
        }

        issues.push_back({.field = "manifest", .message = "unterminated object"});
        return std::nullopt;
    }

    std::string_view text_;
    std::size_t index_ {0};
};

const JsonValue* find_field(const JsonValue& object, std::string_view key) {
    if (object.type != JsonValueType::Object) {
        return nullptr;
    }
    for (const auto& [field_name, value] : object.object_fields) {
        if (field_name == key) {
            return &value;
        }
    }
    return nullptr;
}

void add_issue(std::vector<AppManifestValidationIssue>& issues,
               std::string field,
               std::string message,
               AppManifestValidationIssue::Severity severity =
                   AppManifestValidationIssue::Severity::Fatal) {
    issues.push_back(AppManifestValidationIssue {
        .field = std::move(field),
        .message = std::move(message),
        .severity = severity,
    });
}

std::optional<std::string> require_string_field(const JsonValue& object,
                                                std::string_view key,
                                                std::vector<AppManifestValidationIssue>& issues) {
    const auto* value = find_field(object, key);
    if (value == nullptr) {
        add_issue(issues, std::string(key), "missing required string field");
        return std::nullopt;
    }
    if (value->type != JsonValueType::String) {
        add_issue(issues, std::string(key), "field must be a string");
        return std::nullopt;
    }
    return value->string_value;
}

std::string optional_string_field(const JsonValue& object,
                                  std::string_view key,
                                  std::string default_value,
                                  std::vector<AppManifestValidationIssue>& issues) {
    const auto* value = find_field(object, key);
    if (value == nullptr || value->type == JsonValueType::Null) {
        return default_value;
    }
    if (value->type != JsonValueType::String) {
        add_issue(issues, std::string(key), "field must be a string");
        return default_value;
    }
    return value->string_value;
}

std::uint32_t optional_uint_field(const JsonValue& object,
                                  std::string_view key,
                                  std::uint32_t default_value,
                                  std::vector<AppManifestValidationIssue>& issues) {
    const auto* value = find_field(object, key);
    if (value == nullptr || value->type == JsonValueType::Null) {
        return default_value;
    }
    if (value->type != JsonValueType::Number) {
        add_issue(issues, std::string(key), "field must be an unsigned integer");
        return default_value;
    }
    return static_cast<std::uint32_t>(value->number_value);
}

bool optional_bool_field(const JsonValue& object,
                         std::string_view key,
                         bool default_value,
                         std::vector<AppManifestValidationIssue>& issues) {
    const auto* value = find_field(object, key);
    if (value == nullptr || value->type == JsonValueType::Null) {
        return default_value;
    }
    if (value->type != JsonValueType::Bool) {
        add_issue(issues, std::string(key), "field must be a boolean");
        return default_value;
    }
    return value->bool_value;
}

device::CapabilityLevel capability_level_from_string(const std::string& value) {
    if (value == "full") {
        return device::CapabilityLevel::Full;
    }
    if (value == "degraded") {
        return device::CapabilityLevel::Degraded;
    }
    return device::CapabilityLevel::Absent;
}

std::vector<AppCapabilityRequirement> parse_capabilities(const JsonValue& object,
                                                         std::string_view key,
                                                         std::vector<AppManifestValidationIssue>& issues) {
    std::vector<AppCapabilityRequirement> requirements;
    const auto* value = find_field(object, key);
    if (value == nullptr || value->type == JsonValueType::Null) {
        return requirements;
    }
    if (value->type != JsonValueType::Array) {
        add_issue(issues, std::string(key), "field must be an array");
        return requirements;
    }

    std::set<device::CapabilityId> seen;
    for (std::size_t index = 0; index < value->array_items.size(); ++index) {
        const auto& item = value->array_items[index];
        const auto field_name = std::string(key) + "[" + std::to_string(index) + "]";
        if (item.type == JsonValueType::String) {
            const auto maybe_id = device::capability_id_from_string(item.string_value);
            if (!maybe_id) {
                add_issue(issues, field_name, "unknown capability id");
                continue;
            }
            if (!seen.insert(*maybe_id).second) {
                add_issue(issues, field_name, "duplicate capability requirement");
                continue;
            }
            requirements.push_back(
                AppCapabilityRequirement {.id = *maybe_id,
                                          .min_level = device::CapabilityLevel::Degraded});
            continue;
        }
        if (item.type != JsonValueType::Object) {
            add_issue(issues, field_name, "capability entry must be a string or object");
            continue;
        }

        const auto maybe_id_text = require_string_field(item, "id", issues);
        const auto maybe_level_text = require_string_field(item, "min_level", issues);
        if (!maybe_id_text || !maybe_level_text) {
            continue;
        }
        const auto maybe_id = device::capability_id_from_string(*maybe_id_text);
        if (!maybe_id) {
            add_issue(issues, field_name, "unknown capability id");
            continue;
        }
        if (*maybe_level_text != "absent" && *maybe_level_text != "degraded" &&
            *maybe_level_text != "full") {
            add_issue(issues, field_name, "invalid capability level");
            continue;
        }
        if (!seen.insert(*maybe_id).second) {
            add_issue(issues, field_name, "duplicate capability requirement");
            continue;
        }
        requirements.push_back(AppCapabilityRequirement {
            .id = *maybe_id,
            .min_level = capability_level_from_string(*maybe_level_text),
        });
    }

    return requirements;
}

std::vector<AppPermissionId> parse_permissions(const JsonValue& object,
                                               std::string_view key,
                                               std::vector<AppManifestValidationIssue>& issues) {
    std::vector<AppPermissionId> permissions;
    const auto* value = find_field(object, key);
    if (value == nullptr || value->type == JsonValueType::Null) {
        return permissions;
    }
    if (value->type != JsonValueType::Array) {
        add_issue(issues, std::string(key), "field must be an array");
        return permissions;
    }

    std::set<AppPermissionId> seen;
    for (std::size_t index = 0; index < value->array_items.size(); ++index) {
        const auto& item = value->array_items[index];
        const auto field_name = std::string(key) + "[" + std::to_string(index) + "]";
        if (item.type != JsonValueType::String) {
            add_issue(issues, field_name, "permission entry must be a string");
            continue;
        }
        const auto maybe_permission = permission_id_from_string(item.string_value);
        if (!maybe_permission) {
            add_issue(issues, field_name, "unknown permission id");
            continue;
        }
        if (!seen.insert(*maybe_permission).second) {
            add_issue(issues, field_name, "duplicate permission request");
            continue;
        }
        permissions.push_back(*maybe_permission);
    }

    return permissions;
}

AppMemoryBudget parse_memory_budget(const JsonValue& object,
                                    std::vector<AppManifestValidationIssue>& issues) {
    AppMemoryBudget budget;
    const auto* value = find_field(object, "memory_budget");
    if (value == nullptr || value->type == JsonValueType::Null) {
        return budget;
    }
    if (value->type != JsonValueType::Object) {
        add_issue(issues, "memory_budget", "field must be an object");
        return budget;
    }

    budget.heap_bytes = optional_uint_field(*value, "heap_bytes", 0, issues);
    budget.ui_bytes = optional_uint_field(*value, "ui_bytes", 0, issues);
    return budget;
}

bool is_valid_app_id(std::string_view app_id) {
    if (app_id.empty()) {
        return false;
    }
    for (const auto ch : app_id) {
        const auto uch = static_cast<unsigned char>(ch);
        if ((uch >= 'a' && uch <= 'z') || (uch >= '0' && uch <= '9') || ch == '_' || ch == '-') {
            continue;
        }
        return false;
    }
    return true;
}

bool is_valid_entry_symbol(std::string_view symbol) {
    if (symbol.empty()) {
        return false;
    }
    const auto first = static_cast<unsigned char>(symbol.front());
    if (!(std::isalpha(first) != 0 || symbol.front() == '_')) {
        return false;
    }
    for (const auto ch : symbol) {
        const auto uch = static_cast<unsigned char>(ch);
        if (std::isalnum(uch) != 0 || ch == '_') {
            continue;
        }
        return false;
    }
    return true;
}

bool is_relative_asset_path(std::string_view path) {
    return !path.empty() && path.front() != '/' && path.find("..") == std::string_view::npos;
}

void validate_manifest_contract(const AppManifest& manifest,
                                std::vector<AppManifestValidationIssue>& issues) {
    if (!is_valid_app_id(manifest.app_id)) {
        add_issue(issues, "app_id", "must be lowercase ASCII with digits, '_' or '-' only");
    }
    if (manifest.display_name.empty()) {
        add_issue(issues, "display_name", "must not be empty");
    }
    if (manifest.version.empty()) {
        add_issue(issues, "version", "must not be empty");
    }
    if (manifest.description.empty()) {
        add_issue(issues, "description", "must not be empty");
    }
    if (!is_relative_asset_path(manifest.binary)) {
        add_issue(issues, "binary", "path must be relative and stay within app package");
    }
    if (manifest.icon.empty()) {
        add_issue(issues,
                  "icon",
                  "launcher icon missing; shell will use a placeholder",
                  AppManifestValidationIssue::Severity::Warning);
    } else if (!is_relative_asset_path(manifest.icon)) {
        add_issue(issues,
                  "icon",
                  "path must be relative and stay within app package",
                  AppManifestValidationIssue::Severity::Warning);
    }
    if (!is_valid_entry_symbol(manifest.entry_symbol)) {
        add_issue(issues, "entry_symbol", "must be a valid symbol identifier");
    }
    if (manifest.abi_version == 0) {
        add_issue(issues, "abi_version", "must be a non-zero ABI version");
    }
}

}  // namespace

ManifestParseResult ManifestParser::parse_text(const std::string& text) const {
    ManifestParseResult result;
    JsonParser parser(text);
    auto root = parser.parse(result.issues);
    if (!root) {
        return result;
    }
    if (root->type != JsonValueType::Object) {
        add_issue(result.issues, "manifest", "root must be a JSON object");
        return result;
    }

    auto maybe_app_id = require_string_field(*root, "app_id", result.issues);
    auto maybe_display_name = require_string_field(*root, "display_name", result.issues);
    auto maybe_version = require_string_field(*root, "version", result.issues);
    auto maybe_entry_symbol = require_string_field(*root, "entry_symbol", result.issues);
    auto maybe_description = require_string_field(*root, "description", result.issues);

    result.manifest.app_id = maybe_app_id.value_or("");
    result.manifest.display_name = maybe_display_name.value_or("");
    result.manifest.version = maybe_version.value_or("");
    result.manifest.abi_version =
        optional_uint_field(*root, "abi_version", AEGIS_APP_ABI_VERSION_V1, result.issues);
    result.manifest.binary = optional_string_field(*root, "binary", "app.llext", result.issues);
    result.manifest.entry_symbol = maybe_entry_symbol.value_or("");
    result.manifest.icon = optional_string_field(*root, "icon", "", result.issues);
    result.manifest.description = maybe_description.value_or("");
    result.manifest.required_capabilities =
        parse_capabilities(*root, "required_capabilities", result.issues);
    result.manifest.optional_capabilities =
        parse_capabilities(*root, "optional_capabilities", result.issues);
    result.manifest.preferred_capabilities =
        parse_capabilities(*root, "preferred_capabilities", result.issues);
    result.manifest.requested_permissions =
        parse_permissions(*root, "requested_permissions", result.issues);
    result.manifest.memory_budget = parse_memory_budget(*root, result.issues);
    result.manifest.singleton = optional_bool_field(*root, "singleton", true, result.issues);
    result.manifest.allow_background =
        optional_bool_field(*root, "allow_background", false, result.issues);
    result.manifest.category = optional_string_field(*root, "category", "apps", result.issues);
    result.manifest.order_hint =
        static_cast<int>(optional_uint_field(*root, "order_hint", 0, result.issues));
    result.manifest.hidden = optional_bool_field(*root, "hidden", false, result.issues);

    validate_manifest_contract(result.manifest, result.issues);
    result.ok = true;
    for (const auto& issue : result.issues) {
        if (issue.severity == AppManifestValidationIssue::Severity::Fatal) {
            result.ok = false;
            break;
        }
    }
    return result;
}

bool AppDescriptor::has_validation_issue_severity(AppManifestValidationIssue::Severity severity) const {
    for (const auto& issue : validation_issues) {
        if (issue.severity == severity) {
            return true;
        }
    }
    return false;
}

std::vector<AppManifestValidationIssue> AppDescriptor::validation_issues_with_severity(
    AppManifestValidationIssue::Severity severity) const {
    std::vector<AppManifestValidationIssue> issues;
    for (const auto& issue : validation_issues) {
        if (issue.severity == severity) {
            issues.push_back(issue);
        }
    }
    return issues;
}

}  // namespace aegis::core
