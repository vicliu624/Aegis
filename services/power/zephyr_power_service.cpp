#include "services/power/zephyr_power_service.hpp"

#include <utility>

#include <zephyr/kernel.h>

#include "ports/zephyr/zephyr_board_runtime.hpp"

namespace aegis::services {

ZephyrPowerService::ZephyrPowerService(ports::zephyr::ZephyrBoardBackendConfig config)
    : config_(std::move(config)) {}

bool ZephyrPowerService::battery_present() const {
    return true;
}

bool ZephyrPowerService::low_power_mode_supported() const {
#if defined(CONFIG_PM)
    return true;
#else
    return false;
#endif
}

std::string ZephyrPowerService::describe_status() const {
    auto status = "zephyr-power uptime_ms=" + std::to_string(k_uptime_get());
    if (const auto* runtime = ports::zephyr::try_active_zephyr_board_runtime(); runtime != nullptr &&
        runtime->config().backend_id == config_.backend_id) {
        status += " board-runtime=" + std::string(runtime->ready() ? "ready" : "cold");
        status += " expander=" + std::string(runtime->expander_ready() ? "ready" : "missing");
        status += " shared-spi=" + std::string(runtime->shared_spi_ready() ? "ready" : "missing");
        status += " shared-spi-owner=" + runtime->shared_spi_owner_name();
        status += " radio=" + std::string(runtime->radio_ready() ? "ready" : "gated");
        status += " gps=" + std::string(runtime->gps_ready() ? "ready" : "gated");
        status += " audio=" + std::string(runtime->audio_ready() ? "ready" : "gated");
        status += " sd-present=" + std::string(runtime->sd_card_present() ? "1" : "0");
    }
    return status;
}

}  // namespace aegis::services
