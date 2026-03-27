#include "runtime/syscall/syscall_gateway.hpp"

#include <string>
#include <utility>

#include "runtime/host_api/service_gateway_dispatch.hpp"
#include "sdk/include/aegis/host_api_abi.h"
#include "sdk/include/aegis/services/audio_service_abi.h"
#include "sdk/include/aegis/services/gps_service_abi.h"
#include "sdk/include/aegis/services/hostlink_service_abi.h"
#include "sdk/include/aegis/services/notification_service_abi.h"
#include "sdk/include/aegis/services/radio_service_abi.h"
#include "sdk/include/aegis/services/settings_service_abi.h"
#include "sdk/include/aegis/services/storage_service_abi.h"
#include "sdk/include/aegis/services/text_input_service_abi.h"

namespace aegis::runtime {

SyscallGateway::SyscallGateway(
    const ServiceGatewayDispatch& dispatch,
    const AppQuotaLedger* quota_ledger,
    std::function<std::optional<int>(core::AppPermissionId, std::string_view)> permission_checker,
    std::function<void(const std::string&, const std::string&)> logger)
    : dispatch_(dispatch),
      quota_ledger_(quota_ledger),
      permission_checker_(std::move(permission_checker)),
      logger_(std::move(logger)) {}

int SyscallGateway::dispatch_service(std::uint32_t domain,
                                     std::uint32_t op,
                                     const void* input,
                                     std::size_t input_size,
                                     void* output,
                                     std::size_t* output_size) const {
    if (const auto status = require_service_permission(domain, op); status.has_value()) {
        return *status;
    }

    if (quota_ledger_ != nullptr &&
        quota_ledger_->would_exceed(QuotaResource::ServiceCallsPerMinute, 1)) {
        if (logger_) {
            logger_("runtime", "deny service call because service call quota would be exceeded");
        }
        return AEGIS_HOST_STATUS_UNSUPPORTED;
    }

    return dispatch_.call(domain, op, input, input_size, output, output_size);
}

std::optional<int> SyscallGateway::require_service_permission(std::uint32_t domain,
                                                              std::uint32_t op) const {
    if (!permission_checker_) {
        return std::nullopt;
    }

    switch (domain) {
        case AEGIS_SERVICE_DOMAIN_RADIO:
            return permission_checker_(core::AppPermissionId::RadioUse, "access radio service");
        case AEGIS_SERVICE_DOMAIN_GPS:
            return permission_checker_(core::AppPermissionId::GpsUse, "access gps service");
        case AEGIS_SERVICE_DOMAIN_AUDIO:
            return permission_checker_(core::AppPermissionId::AudioUse, "access audio service");
        case AEGIS_SERVICE_DOMAIN_HOSTLINK:
            return permission_checker_(core::AppPermissionId::HostlinkUse,
                                       "access hostlink service");
        case AEGIS_SERVICE_DOMAIN_STORAGE:
            return permission_checker_(core::AppPermissionId::StorageAccess,
                                       "access storage service");
        case AEGIS_SERVICE_DOMAIN_NOTIFICATION:
            if (op == AEGIS_NOTIFICATION_SERVICE_OP_PUBLISH) {
                return permission_checker_(core::AppPermissionId::NotificationPost,
                                           "access notification service");
            }
            return std::nullopt;
        case AEGIS_SERVICE_DOMAIN_SETTINGS:
            if (op == AEGIS_SETTINGS_SERVICE_OP_SET) {
                return permission_checker_(core::AppPermissionId::SettingsWrite,
                                           "write settings service");
            }
            return std::nullopt;
        case AEGIS_SERVICE_DOMAIN_TEXT_INPUT:
            if (op == AEGIS_TEXT_INPUT_SERVICE_OP_REQUEST_FOCUS ||
                op == AEGIS_TEXT_INPUT_SERVICE_OP_RELEASE_FOCUS) {
                return permission_checker_(core::AppPermissionId::TextInputFocus,
                                           "control text input focus");
            }
            return std::nullopt;
        default:
            return std::nullopt;
    }
}

}  // namespace aegis::runtime
