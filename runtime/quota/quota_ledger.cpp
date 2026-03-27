#include "runtime/quota/quota_ledger.hpp"

#include <algorithm>

namespace aegis::runtime {

AppQuotaLedger::AppQuotaLedger(const core::AppMemoryBudget& budget) {
    set_limit(QuotaResource::HeapBytes, budget.heap_bytes);
    set_limit(QuotaResource::UiBytes, budget.ui_bytes);
}

void AppQuotaLedger::set_limit(QuotaResource resource, std::size_t limit, bool hard_limit) {
    limits_[resource] = QuotaLimit {
        .resource = resource,
        .limit = limit,
        .used = 0,
        .hard_limit = hard_limit,
    };
}

std::optional<QuotaLimit> AppQuotaLedger::limit_for(QuotaResource resource) const {
    const auto it = limits_.find(resource);
    if (it == limits_.end()) {
        return std::nullopt;
    }
    return it->second;
}

bool AppQuotaLedger::would_exceed(QuotaResource resource, std::size_t amount) const {
    const auto limit = limit_for(resource);
    if (!limit) {
        return false;
    }
    if (limit->limit == 0) {
        return amount > 0;
    }
    return amount > (limit->limit - std::min(limit->used, limit->limit));
}

bool AppQuotaLedger::try_consume(QuotaResource resource, std::size_t amount) {
    auto it = limits_.find(resource);
    if (it == limits_.end()) {
        return true;
    }
    if (would_exceed(resource, amount)) {
        return false;
    }
    it->second.used += amount;
    return true;
}

void AppQuotaLedger::force_consume(QuotaResource resource, std::size_t amount) {
    auto it = limits_.find(resource);
    if (it == limits_.end()) {
        return;
    }
    it->second.used += amount;
}

void AppQuotaLedger::release(QuotaResource resource, std::size_t amount) {
    auto it = limits_.find(resource);
    if (it == limits_.end()) {
        return;
    }
    it->second.used = amount >= it->second.used ? 0 : (it->second.used - amount);
}

std::vector<QuotaLimit> AppQuotaLedger::limits() const {
    std::vector<QuotaLimit> result;
    result.reserve(limits_.size());
    for (const auto& [_, limit] : limits_) {
        result.push_back(limit);
    }
    return result;
}

}  // namespace aegis::runtime
