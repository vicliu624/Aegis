#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace aegis::runtime {

enum class AppHandleKind {
    Unknown,
    Allocation,
    UiRoot,
    Timer,
    Notification,
    ForegroundPage,
    TextInputFocus,
};

struct AppHandleRecord {
    std::uint32_t handle_id {0};
    AppHandleKind kind {AppHandleKind::Unknown};
    std::string label;
    std::function<void()> release_hook;
};

class AppHandleTable {
public:
    [[nodiscard]] std::uint32_t register_handle(AppHandleKind kind,
                                                std::string label,
                                                std::function<void()> release_hook = {});
    [[nodiscard]] std::optional<AppHandleRecord> find(std::uint32_t handle_id) const;
    [[nodiscard]] bool release(std::uint32_t handle_id);
    [[nodiscard]] bool release_by_label(AppHandleKind kind, const std::string& label);
    [[nodiscard]] std::vector<AppHandleRecord> snapshot() const;

private:
    std::uint32_t next_handle_id_ {1};
    std::unordered_map<std::uint32_t, AppHandleRecord> handles_;
};

}  // namespace aegis::runtime
