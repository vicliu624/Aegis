#pragma once

#include <cstddef>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "core/app_registry/app_manifest.hpp"

namespace aegis::runtime {

enum class QuotaResource {
    HeapBytes,
    UiBytes,
    TimerCount,
    EventQueueDepth,
    ServiceCallsPerMinute,
    LogBytesPerMinute,
    CpuTimeSliceMs,
    ForegroundOccupancyMs,
};

struct QuotaLimit {
    QuotaResource resource {QuotaResource::HeapBytes};
    std::size_t limit {0};
    std::size_t used {0};
    bool hard_limit {true};
};

class AppQuotaLedger {
public:
    AppQuotaLedger() = default;
    explicit AppQuotaLedger(const core::AppMemoryBudget& budget);

    void set_limit(QuotaResource resource, std::size_t limit, bool hard_limit = true);
    [[nodiscard]] std::optional<QuotaLimit> limit_for(QuotaResource resource) const;
    [[nodiscard]] bool would_exceed(QuotaResource resource, std::size_t amount) const;
    [[nodiscard]] bool try_consume(QuotaResource resource, std::size_t amount);
    void force_consume(QuotaResource resource, std::size_t amount);
    void release(QuotaResource resource, std::size_t amount);
    [[nodiscard]] std::vector<QuotaLimit> limits() const;

private:
    struct EnumHash {
        template <typename T>
        std::size_t operator()(T value) const noexcept {
            return static_cast<std::size_t>(value);
        }
    };

    std::unordered_map<QuotaResource, QuotaLimit, EnumHash> limits_;
};

[[nodiscard]] constexpr std::string_view to_string(QuotaResource resource) {
    switch (resource) {
        case QuotaResource::HeapBytes:
            return "heap bytes";
        case QuotaResource::UiBytes:
            return "ui bytes";
        case QuotaResource::TimerCount:
            return "timer count";
        case QuotaResource::EventQueueDepth:
            return "event queue depth";
        case QuotaResource::ServiceCallsPerMinute:
            return "service calls per minute";
        case QuotaResource::LogBytesPerMinute:
            return "log bytes per minute";
        case QuotaResource::CpuTimeSliceMs:
            return "cpu time slice ms";
        case QuotaResource::ForegroundOccupancyMs:
            return "foreground occupancy ms";
    }

    return "unknown";
}

}  // namespace aegis::runtime
