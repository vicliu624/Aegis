#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string_view>

#include "core/app_registry/app_permissions.hpp"
#include "runtime/quota/quota_ledger.hpp"

namespace aegis::runtime {

class ServiceGatewayDispatch;

class SyscallGateway {
public:
    SyscallGateway(const ServiceGatewayDispatch& dispatch,
                   const AppQuotaLedger* quota_ledger,
                   std::function<std::optional<int>(core::AppPermissionId, std::string_view)>
                       permission_checker,
                   std::function<void(const std::string&, const std::string&)> logger);

    int dispatch_service(std::uint32_t domain,
                         std::uint32_t op,
                         const void* input,
                         std::size_t input_size,
                         void* output,
                         std::size_t* output_size) const;

private:
    [[nodiscard]] std::optional<int> require_service_permission(std::uint32_t domain,
                                                                std::uint32_t op) const;

    const ServiceGatewayDispatch& dispatch_;
    const AppQuotaLedger* quota_ledger_ {nullptr};
    std::function<std::optional<int>(core::AppPermissionId, std::string_view)> permission_checker_;
    std::function<void(const std::string&, const std::string&)> logger_;
};

}  // namespace aegis::runtime
