#include "services/power/zephyr_power_service.hpp"

#include <utility>

#include <zephyr/kernel.h>

#include "ports/zephyr/zephyr_tlora_pager_board_runtime.hpp"

namespace aegis::services {

namespace {

std::string_view spi_owner_name(ports::zephyr::ZephyrTloraPagerBoardRuntime::SharedSpiClient client) {
    switch (client) {
        case ports::zephyr::ZephyrTloraPagerBoardRuntime::SharedSpiClient::Unknown:
            return "unknown";
        case ports::zephyr::ZephyrTloraPagerBoardRuntime::SharedSpiClient::Display:
            return "display";
        case ports::zephyr::ZephyrTloraPagerBoardRuntime::SharedSpiClient::Radio:
            return "radio";
        case ports::zephyr::ZephyrTloraPagerBoardRuntime::SharedSpiClient::Storage:
            return "storage";
        case ports::zephyr::ZephyrTloraPagerBoardRuntime::SharedSpiClient::Nfc:
            return "nfc";
    }
    return "unknown";
}

}  // namespace

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
    if (config_.board_family == "lilygo_tlora_pager") {
        if (const auto* runtime = ports::zephyr::try_tlora_pager_board_runtime(); runtime != nullptr) {
            status += " board-runtime=" + std::string(runtime->ready() ? "ready" : "cold");
            status += " expander=" + std::string(runtime->expander_ready() ? "ready" : "missing");
            status += " shared-spi=" + std::string(runtime->shared_spi_ready() ? "ready" : "missing");
            status += " shared-spi-owner=" + std::string(spi_owner_name(runtime->shared_spi_owner()));
            status += " radio=" + std::string(runtime->radio_ready() ? "ready" : "gated");
            status += " gps=" + std::string(runtime->gps_ready() ? "ready" : "gated");
            status += " audio=" + std::string(runtime->audio_ready() ? "ready" : "gated");
            status += " sd-present=" + std::string(runtime->sd_card_present() ? "1" : "0");
        } else {
            status += " board-runtime=unbound";
        }
    }
    return status;
}

}  // namespace aegis::services
