#include "ports/zephyr/boards/tlora_pager/tlora_pager_shell_display_backend.hpp"

#include <array>
#include <cstddef>
#include <string_view>
#include <utility>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mipi_dbi.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/printk.h>

#include "ports/zephyr/zephyr_gpio_pin.hpp"

namespace aegis::ports::zephyr {

namespace {

constexpr uint8_t kSt7796sCmdSleepOut = 0x11;
constexpr uint8_t kSt7796sCmdSoftwareReset = 0x01;
constexpr uint8_t kSt7796sCmdDisplayInversionOn = 0x21;
constexpr uint8_t kSt7796sCmdColumnAddressSet = 0x2A;
constexpr uint8_t kSt7796sCmdRowAddressSet = 0x2B;
constexpr uint8_t kSt7796sCmdMemoryWrite = 0x2C;
constexpr uint8_t kSt7796sCmdDisplayOn = 0x29;
constexpr uint8_t kSt7796sCmdMadctl = 0x36;
constexpr uint8_t kSt7796sCmdColmod = 0x3A;
constexpr uint8_t kSt7796sCmdDisplayInversionControl = 0xB4;
constexpr uint8_t kSt7796sCmdDisplayFunctionControl = 0xB6;
constexpr uint8_t kSt7796sCmdPowerControl2 = 0xC1;
constexpr uint8_t kSt7796sCmdPowerControl3 = 0xC2;
constexpr uint8_t kSt7796sCmdVcomControl = 0xC5;
constexpr uint8_t kSt7796sCmdPositiveGamma = 0xE0;
constexpr uint8_t kSt7796sCmdNegativeGamma = 0xE1;
constexpr uint8_t kSt7796sCmdDisplayOutputControlAdjust = 0xE8;
constexpr uint8_t kSt7796sCmdCommandSetControl = 0xF0;
constexpr uint8_t kSt7796sUnlock1 = 0xC3;
constexpr uint8_t kSt7796sUnlock2 = 0x96;
constexpr uint8_t kSt7796sLock1 = 0x3C;
constexpr uint8_t kSt7796sLock2 = 0x69;
constexpr uint8_t kPagerMadctlInit = 0x48;
constexpr uint8_t kPagerInvertMode = 0x01;
constexpr uint8_t kPagerColmod = 0x55;
constexpr uint8_t kPagerDfc[] = {0x80, 0x02, 0x3B};
constexpr uint8_t kPagerDoca[] = {0x40, 0x8A, 0x00, 0x00, 0x29, 0x19, 0xA5, 0x33};
constexpr uint8_t kPagerPgc[] = {0xF0, 0x09, 0x0B, 0x06, 0x04, 0x15, 0x2F, 0x54, 0x42, 0x3C, 0x17, 0x14, 0x18, 0x1B};
constexpr uint8_t kPagerNgc[] = {0xE0, 0x09, 0x0B, 0x06, 0x04, 0x03, 0x2B, 0x43, 0x42, 0x3B, 0x16, 0x14, 0x17, 0x1B};
constexpr uint8_t kPagerPowerControl2 = 0x06;
constexpr uint8_t kPagerPowerControl3 = 0xA7;
constexpr uint8_t kPagerVcom = 0x18;

const ::device* find_device(const std::string& name) {
    if (name.empty()) {
        return nullptr;
    }
    return device_get_binding(name.c_str());
}

const ::device* dt_display_device() {
#if DT_NODE_EXISTS(DT_ALIAS(display0)) && DT_NODE_HAS_STATUS(DT_ALIAS(display0), okay)
    return device_get_binding(DEVICE_DT_NAME(DT_ALIAS(display0)));
#else
    return nullptr;
#endif
}

const ::device* dt_mipi_device() {
#if DT_NODE_EXISTS(DT_ALIAS(display0)) && DT_NODE_HAS_STATUS(DT_PARENT(DT_ALIAS(display0)), okay)
    return device_get_binding(DEVICE_DT_NAME(DT_PARENT(DT_ALIAS(display0))));
#else
    return nullptr;
#endif
}

const ::device* dt_spi2_device() {
#if DT_NODE_EXISTS(DT_NODELABEL(spi2))
    return device_get_binding(DEVICE_DT_NAME(DT_NODELABEL(spi2)));
#else
    return nullptr;
#endif
}

mipi_dbi_config make_raw_mipi_config() {
#if DT_NODE_EXISTS(DT_ALIAS(display0)) && DT_NODE_HAS_STATUS(DT_ALIAS(display0), okay)
    return mipi_dbi_config {
        .mode = MIPI_DBI_MODE_SPI_4WIRE,
        .config =
            MIPI_DBI_SPI_CONFIG_DT(DT_ALIAS(display0),
                                   SPI_OP_MODE_MASTER | SPI_WORD_SET(8),
                                   0),
    };
#else
    return mipi_dbi_config {};
#endif
}

class PagerDisplayBackend final : public IZephyrShellDisplayBackend {
public:
    PagerDisplayBackend(platform::Logger& logger,
                        ZephyrBoardRuntime& runtime,
                        ZephyrBoardBackendConfig config)
        : logger_(logger), runtime_(runtime), config_(std::move(config)) {}

    [[nodiscard]] bool initialize() override;
    void write_region(int x,
                      int y,
                      int width,
                      int height,
                      const uint16_t* pixels,
                      std::size_t count) const override;

private:
    enum class BackendKind {
        None,
        ManualSpi,
        DisplayApi,
        RawMipiDbi,
    };

    [[nodiscard]] bool initialize_display_api_backend();
    [[nodiscard]] bool initialize_manual_spi_backend();
    [[nodiscard]] bool initialize_raw_mipi_backend();
    void log_backend_state(std::string_view stage, int rc) const;
    [[nodiscard]] bool manual_backend_ready() const;
    [[nodiscard]] bool raw_backend_ready() const;
    [[nodiscard]] int manual_send_command(uint8_t cmd, const uint8_t* data, std::size_t len) const;
    [[nodiscard]] int manual_send_command_unlocked(uint8_t cmd, const uint8_t* data, std::size_t len) const;
    [[nodiscard]] int manual_write_pixels(int x, int y, int width, int height, const uint16_t* pixels, std::size_t count) const;
    [[nodiscard]] int manual_write_pixels_unlocked(int x, int y, int width, int height, const uint16_t* pixels, std::size_t count) const;
    [[nodiscard]] int raw_send_command(uint8_t cmd, const uint8_t* data, std::size_t len) const;
    [[nodiscard]] int raw_send_command_unlocked(uint8_t cmd, const uint8_t* data, std::size_t len) const;
    [[nodiscard]] int raw_set_cursor(uint16_t x, uint16_t y, uint16_t width, uint16_t height) const;
    [[nodiscard]] int raw_write_pixels(int x, int y, int width, int height, const uint16_t* pixels, std::size_t count) const;
    [[nodiscard]] int raw_write_pixels_unlocked(int x, int y, int width, int height, const uint16_t* pixels, std::size_t count) const;

    platform::Logger& logger_;
    ZephyrBoardRuntime& runtime_;
    ZephyrBoardBackendConfig config_;
    BackendKind backend_ {BackendKind::None};
    const struct device* display_device_ {nullptr};
    const struct device* raw_mipi_device_ {nullptr};
    const struct device* manual_spi_device_ {nullptr};
    spi_config manual_spi_config_ {};
};

bool PagerDisplayBackend::initialize() {
    if (config_.display_prefer_manual_spi && initialize_manual_spi_backend()) {
        return true;
    }
    if (config_.display_prefer_raw_mipi && initialize_raw_mipi_backend()) {
        return true;
    }
    if (initialize_display_api_backend()) {
        return true;
    }
    if (!config_.display_prefer_manual_spi && initialize_manual_spi_backend()) {
        return true;
    }
    if (!config_.display_prefer_raw_mipi && initialize_raw_mipi_backend()) {
        return true;
    }
    logger_.info("display", "display backend unavailable");
    return false;
}

bool PagerDisplayBackend::initialize_display_api_backend() {
    display_device_ = find_device(config_.display_device_name);
    if (display_device_ == nullptr) {
        display_device_ = dt_display_device();
    }
    if (display_device_ == nullptr || !device_is_ready(display_device_)) {
        log_backend_state("display-api-missing", -19);
        return false;
    }
    const int rc = display_blanking_off(display_device_);
    backend_ = BackendKind::DisplayApi;
    log_backend_state("display-api-ready", rc);
    return true;
}

bool PagerDisplayBackend::initialize_manual_spi_backend() {
    if (config_.display_cs_pin < 0 || config_.display_dc_pin < 0) {
        log_backend_state("manual-spi-missing-pins", -22);
        return false;
    }
    manual_spi_device_ = dt_spi2_device();
    if (manual_spi_device_ == nullptr || !device_is_ready(manual_spi_device_)) {
        log_backend_state("manual-spi-missing-devices", -19);
        return false;
    }

    manual_spi_config_ = {};
    manual_spi_config_.frequency = 40000000U;
    manual_spi_config_.operation =
        static_cast<spi_operation_t>(SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB);
    manual_spi_config_.slave = 0;
    manual_spi_config_.cs = {};
    manual_spi_config_.cs.gpio.port = nullptr;
    manual_spi_config_.cs.cs_is_gpio = false;

    const auto cs_ref = resolve_gpio_pin(config_.display_cs_pin);
    const auto dc_ref = resolve_gpio_pin(config_.display_dc_pin);
    if (cs_ref.device == nullptr || !device_is_ready(cs_ref.device) ||
        dc_ref.device == nullptr || !device_is_ready(dc_ref.device)) {
        manual_spi_device_ = nullptr;
        backend_ = BackendKind::None;
        log_backend_state("manual-spi-gpio-route-failed", -19);
        return false;
    }

    gpio_pin_configure(cs_ref.device, cs_ref.pin, GPIO_OUTPUT_ACTIVE);
    gpio_pin_set(cs_ref.device, cs_ref.pin, 1);
    gpio_pin_configure(dc_ref.device, dc_ref.pin, GPIO_OUTPUT_ACTIVE);
    gpio_pin_set(dc_ref.device, dc_ref.pin, 1);
    backend_ = BackendKind::ManualSpi;

    struct InitCommand {
        uint8_t cmd;
        std::array<uint8_t, 15> data;
        uint8_t len;
        bool delay_after;
    };

    const std::array<InitCommand, 19> init_sequence {{
        {0x01, {}, 0, true}, {0x11, {}, 0, true}, {0xF0, {0xC3}, 1, false}, {0xF0, {0xC3}, 1, false},
        {0xF0, {0x96}, 1, false}, {0x3A, {0x55}, 1, false}, {0xB4, {0x01}, 1, false},
        {0xB6, {0x80, 0x02, 0x3B}, 3, false}, {0xE8, {0x40, 0x8A, 0x00, 0x00, 0x29, 0x19, 0xA5, 0x33}, 8, false},
        {0xC1, {0x06}, 1, false}, {0xC2, {0xA7}, 1, false}, {0xC5, {0x18}, 1, true},
        {0xE0, {0xF0, 0x09, 0x0B, 0x06, 0x04, 0x15, 0x2F, 0x54, 0x42, 0x3C, 0x17, 0x14, 0x18, 0x1B}, 14, false},
        {0xE1, {0xE0, 0x09, 0x0B, 0x06, 0x04, 0x03, 0x2B, 0x43, 0x42, 0x3B, 0x16, 0x14, 0x17, 0x1B}, 14, false},
        {0xF0, {0x3C}, 1, false}, {0xF0, {0x69}, 1, true}, {0x21, {0x00}, 1, false}, {0x29, {0x00}, 1, false},
    }};

    for (const auto& step : init_sequence) {
        const int rc = manual_send_command(step.cmd, step.len == 0 ? nullptr : step.data.data(), step.len);
        if (rc != 0) {
            manual_spi_device_ = nullptr;
            backend_ = BackendKind::None;
            log_backend_state("manual-spi-init-failed", rc);
            return false;
        }
        if (step.delay_after) {
            k_sleep(K_MSEC(120));
        }
    }

    const uint8_t madctl = static_cast<uint8_t>(config_.display_madctl & 0xFF);
    const int rc = manual_send_command(kSt7796sCmdMadctl, &madctl, 1);
    if (rc != 0) {
        manual_spi_device_ = nullptr;
        backend_ = BackendKind::None;
        log_backend_state("manual-spi-madctl-failed", rc);
        return false;
    }

    log_backend_state("manual-spi-ready", 0);
    return true;
}

bool PagerDisplayBackend::initialize_raw_mipi_backend() {
    raw_mipi_device_ = dt_mipi_device();
    if (raw_mipi_device_ == nullptr || !device_is_ready(raw_mipi_device_)) {
        log_backend_state("raw-mipi-missing", -19);
        return false;
    }

    backend_ = BackendKind::RawMipiDbi;
    int rc = raw_send_command(kSt7796sCmdSoftwareReset, nullptr, 0);
    k_sleep(K_MSEC(120));
    if (rc == 0) {
        rc = raw_send_command(kSt7796sCmdSleepOut, nullptr, 0);
        k_sleep(K_MSEC(120));
    }
    uint8_t param = kSt7796sUnlock1;
    if (rc == 0) {
        rc = raw_send_command(kSt7796sCmdCommandSetControl, &param, sizeof(param));
    }
    if (rc == 0) {
        rc = raw_send_command(kSt7796sCmdCommandSetControl, &param, sizeof(param));
    }
    if (rc == 0) {
        param = kSt7796sUnlock2;
        rc = raw_send_command(kSt7796sCmdCommandSetControl, &param, sizeof(param));
    }
    if (rc == 0) {
        param = kPagerMadctlInit;
        rc = raw_send_command(kSt7796sCmdMadctl, &param, sizeof(param));
    }
    if (rc == 0) {
        param = kPagerColmod;
        rc = raw_send_command(kSt7796sCmdColmod, &param, sizeof(param));
    }
    if (rc == 0) {
        param = kPagerInvertMode;
        rc = raw_send_command(kSt7796sCmdDisplayInversionControl, &param, sizeof(param));
    }
    if (rc == 0) {
        rc = raw_send_command(kSt7796sCmdDisplayFunctionControl, kPagerDfc, sizeof(kPagerDfc));
    }
    if (rc == 0) {
        rc = raw_send_command(kSt7796sCmdDisplayOutputControlAdjust, kPagerDoca, sizeof(kPagerDoca));
    }
    if (rc == 0) {
        param = kPagerPowerControl2;
        rc = raw_send_command(kSt7796sCmdPowerControl2, &param, sizeof(param));
    }
    if (rc == 0) {
        param = kPagerPowerControl3;
        rc = raw_send_command(kSt7796sCmdPowerControl3, &param, sizeof(param));
    }
    if (rc == 0) {
        param = kPagerVcom;
        rc = raw_send_command(kSt7796sCmdVcomControl, &param, sizeof(param));
    }
    if (rc == 0) {
        rc = raw_send_command(kSt7796sCmdPositiveGamma, kPagerPgc, sizeof(kPagerPgc));
    }
    if (rc == 0) {
        rc = raw_send_command(kSt7796sCmdNegativeGamma, kPagerNgc, sizeof(kPagerNgc));
    }
    if (rc == 0) {
        param = kSt7796sLock1;
        rc = raw_send_command(kSt7796sCmdCommandSetControl, &param, sizeof(param));
    }
    if (rc == 0) {
        param = kSt7796sLock2;
        rc = raw_send_command(kSt7796sCmdCommandSetControl, &param, sizeof(param));
    }
    if (rc == 0) {
        rc = raw_send_command(kSt7796sCmdDisplayInversionOn, nullptr, 0);
    }
    if (rc == 0) {
        param = static_cast<uint8_t>(config_.display_madctl & 0xFF);
        rc = raw_send_command(kSt7796sCmdMadctl, &param, sizeof(param));
    }
    if (rc == 0) {
        rc = raw_send_command(kSt7796sCmdDisplayOn, nullptr, 0);
        k_sleep(K_MSEC(120));
    }

    if (rc != 0) {
        raw_mipi_device_ = nullptr;
        backend_ = BackendKind::None;
        log_backend_state("raw-mipi-failed", rc);
        return false;
    }

    log_backend_state("raw-mipi-ready", 0);
    return true;
}

void PagerDisplayBackend::log_backend_state(std::string_view stage, int rc) const {
    printk("AEGIS TRACE: display stage=%s rc=%d cfg-name=%s\n",
           std::string(stage).c_str(),
           rc,
           config_.display_device_name.c_str());
    logger_.info("display", "stage=" + std::string(stage) + " rc=" + std::to_string(rc));
}

bool PagerDisplayBackend::manual_backend_ready() const {
    const auto cs_ref = resolve_gpio_pin(config_.display_cs_pin);
    const auto dc_ref = resolve_gpio_pin(config_.display_dc_pin);
    return backend_ == BackendKind::ManualSpi && manual_spi_device_ != nullptr &&
           device_is_ready(manual_spi_device_) && cs_ref.device != nullptr &&
           device_is_ready(cs_ref.device) && dc_ref.device != nullptr &&
           device_is_ready(dc_ref.device);
}

bool PagerDisplayBackend::raw_backend_ready() const {
    return backend_ == BackendKind::RawMipiDbi && raw_mipi_device_ != nullptr &&
           device_is_ready(raw_mipi_device_);
}

int PagerDisplayBackend::manual_send_command(uint8_t cmd, const uint8_t* data, std::size_t len) const {
    if (runtime_.ready()) {
        return runtime_.with_coordination_domain(ZephyrBoardCoordinationDomain::DisplayPipeline,
                                                 K_MSEC(50),
                                                 "display.manual_send_command",
                                                 [&]() { return manual_send_command_unlocked(cmd, data, len); });
    }
    return manual_send_command_unlocked(cmd, data, len);
}

int PagerDisplayBackend::manual_send_command_unlocked(uint8_t cmd, const uint8_t* data, std::size_t len) const {
    if (!manual_backend_ready()) {
        return -19;
    }
    const auto cs_ref = resolve_gpio_pin(config_.display_cs_pin);
    const auto dc_ref = resolve_gpio_pin(config_.display_dc_pin);
    gpio_pin_set(cs_ref.device, cs_ref.pin, 0);
    gpio_pin_set(dc_ref.device, dc_ref.pin, 0);
    spi_buf cmd_buf {.buf = &cmd, .len = sizeof(cmd)};
    spi_buf_set cmd_set {.buffers = &cmd_buf, .count = 1};
    int rc = spi_write(manual_spi_device_, &manual_spi_config_, &cmd_set);
    if (rc == 0 && data != nullptr && len > 0) {
        gpio_pin_set(dc_ref.device, dc_ref.pin, 1);
        spi_buf data_buf {.buf = const_cast<uint8_t*>(data), .len = len};
        spi_buf_set data_set {.buffers = &data_buf, .count = 1};
        rc = spi_write(manual_spi_device_, &manual_spi_config_, &data_set);
    }
    gpio_pin_set(cs_ref.device, cs_ref.pin, 1);
    return rc;
}

int PagerDisplayBackend::manual_write_pixels(int x, int y, int width, int height, const uint16_t* pixels, std::size_t count) const {
    if (runtime_.ready()) {
        return runtime_.with_coordination_domain(ZephyrBoardCoordinationDomain::DisplayPipeline,
                                                 K_MSEC(50),
                                                 "display.manual_write_pixels",
                                                 [&]() { return manual_write_pixels_unlocked(x, y, width, height, pixels, count); });
    }
    return manual_write_pixels_unlocked(x, y, width, height, pixels, count);
}

int PagerDisplayBackend::manual_write_pixels_unlocked(int x, int y, int width, int height, const uint16_t* pixels, std::size_t count) const {
    if (!manual_backend_ready() || pixels == nullptr || count == 0) {
        return -19;
    }
    const uint16_t xs = static_cast<uint16_t>(x + config_.display_x_gap);
    const uint16_t ys = static_cast<uint16_t>(y + config_.display_y_gap);
    const uint16_t xe = static_cast<uint16_t>(xs + width - 1);
    const uint16_t ye = static_cast<uint16_t>(ys + height - 1);
    const std::array<uint8_t, 4> col_params {
        static_cast<uint8_t>(xs >> 8), static_cast<uint8_t>(xs & 0xFF),
        static_cast<uint8_t>(xe >> 8), static_cast<uint8_t>(xe & 0xFF),
    };
    const std::array<uint8_t, 4> row_params {
        static_cast<uint8_t>(ys >> 8), static_cast<uint8_t>(ys & 0xFF),
        static_cast<uint8_t>(ye >> 8), static_cast<uint8_t>(ye & 0xFF),
    };

    auto write_bytes = [&](const uint8_t* data, std::size_t len, bool command) -> int {
        const auto dc_ref = resolve_gpio_pin(config_.display_dc_pin);
        gpio_pin_set(dc_ref.device, dc_ref.pin, command ? 0 : 1);
        spi_buf buf {.buf = const_cast<uint8_t*>(data), .len = len};
        spi_buf_set set {.buffers = &buf, .count = 1};
        return spi_write(manual_spi_device_, &manual_spi_config_, &set);
    };

    const uint8_t col_cmd = kSt7796sCmdColumnAddressSet;
    const uint8_t row_cmd = kSt7796sCmdRowAddressSet;
    const uint8_t ramwr_cmd = kSt7796sCmdMemoryWrite;
    const auto cs_ref = resolve_gpio_pin(config_.display_cs_pin);
    const auto dc_ref = resolve_gpio_pin(config_.display_dc_pin);
    gpio_pin_set(cs_ref.device, cs_ref.pin, 0);
    int rc = write_bytes(&col_cmd, sizeof(col_cmd), true);
    if (rc == 0) { rc = write_bytes(col_params.data(), col_params.size(), false); }
    if (rc == 0) { rc = write_bytes(&row_cmd, sizeof(row_cmd), true); }
    if (rc == 0) { rc = write_bytes(row_params.data(), row_params.size(), false); }
    if (rc == 0) { rc = write_bytes(&ramwr_cmd, sizeof(ramwr_cmd), true); }
    if (rc == 0) {
        rc = write_bytes(reinterpret_cast<const uint8_t*>(pixels), count * sizeof(uint16_t), false);
    }
    gpio_pin_set(cs_ref.device, cs_ref.pin, 1);
    gpio_pin_set(dc_ref.device, dc_ref.pin, 1);
    return rc;
}

int PagerDisplayBackend::raw_send_command(uint8_t cmd, const uint8_t* data, std::size_t len) const {
    if (runtime_.ready()) {
        return runtime_.with_coordination_domain(ZephyrBoardCoordinationDomain::DisplayPipeline,
                                                 K_MSEC(50),
                                                 "display.raw_send_command",
                                                 [&]() { return raw_send_command_unlocked(cmd, data, len); });
    }
    return raw_send_command_unlocked(cmd, data, len);
}

int PagerDisplayBackend::raw_send_command_unlocked(uint8_t cmd, const uint8_t* data, std::size_t len) const {
    if (!raw_backend_ready()) {
        return -19;
    }
    auto raw_config = make_raw_mipi_config();
    return mipi_dbi_command_write(raw_mipi_device_, &raw_config, cmd, data, len);
}

int PagerDisplayBackend::raw_set_cursor(uint16_t x, uint16_t y, uint16_t width, uint16_t height) const {
    uint16_t address_data[2];
    const auto x0 = static_cast<uint16_t>(x + config_.display_x_gap);
    const auto y0 = static_cast<uint16_t>(y + config_.display_y_gap);
    address_data[0] = sys_cpu_to_be16(x0);
    address_data[1] = sys_cpu_to_be16(static_cast<uint16_t>(x0 + width - 1));
    int rc = raw_send_command_unlocked(kSt7796sCmdColumnAddressSet,
                                       reinterpret_cast<const uint8_t*>(address_data),
                                       sizeof(address_data));
    if (rc != 0) {
        return rc;
    }
    address_data[0] = sys_cpu_to_be16(y0);
    address_data[1] = sys_cpu_to_be16(static_cast<uint16_t>(y0 + height - 1));
    return raw_send_command_unlocked(kSt7796sCmdRowAddressSet,
                                     reinterpret_cast<const uint8_t*>(address_data),
                                     sizeof(address_data));
}

int PagerDisplayBackend::raw_write_pixels(int x, int y, int width, int height, const uint16_t* pixels, std::size_t count) const {
    if (runtime_.ready()) {
        return runtime_.with_coordination_domain(ZephyrBoardCoordinationDomain::DisplayPipeline,
                                                 K_MSEC(50),
                                                 "display.raw_write_pixels",
                                                 [&]() { return raw_write_pixels_unlocked(x, y, width, height, pixels, count); });
    }
    return raw_write_pixels_unlocked(x, y, width, height, pixels, count);
}

int PagerDisplayBackend::raw_write_pixels_unlocked(int x, int y, int width, int height, const uint16_t* pixels, std::size_t count) const {
    if (!raw_backend_ready() || pixels == nullptr || count == 0) {
        return -19;
    }
    const int rc = raw_set_cursor(static_cast<uint16_t>(x),
                                  static_cast<uint16_t>(y),
                                  static_cast<uint16_t>(width),
                                  static_cast<uint16_t>(height));
    if (rc != 0) {
        return rc;
    }
    const int memory_write_rc = raw_send_command_unlocked(kSt7796sCmdMemoryWrite, nullptr, 0);
    if (memory_write_rc != 0) {
        return memory_write_rc;
    }
    display_buffer_descriptor desc {
        .buf_size = static_cast<uint32_t>(count * sizeof(uint16_t)),
        .width = static_cast<uint16_t>(width),
        .height = static_cast<uint16_t>(height),
        .pitch = static_cast<uint16_t>(width),
        .frame_incomplete = false,
    };
    auto raw_config = make_raw_mipi_config();
    return mipi_dbi_write_display(raw_mipi_device_,
                                  &raw_config,
                                  reinterpret_cast<const uint8_t*>(pixels),
                                  &desc,
                                  PIXEL_FORMAT_RGB_565);
}

void PagerDisplayBackend::write_region(int x, int y, int width, int height, const uint16_t* pixels, std::size_t count) const {
    if (pixels == nullptr || count == 0) {
        return;
    }
    if (backend_ == BackendKind::DisplayApi) {
        if (display_device_ == nullptr || !device_is_ready(display_device_)) {
            return;
        }
        display_buffer_descriptor desc {
            .buf_size = static_cast<uint32_t>(count * sizeof(uint16_t)),
            .width = static_cast<uint16_t>(width),
            .height = static_cast<uint16_t>(height),
            .pitch = static_cast<uint16_t>(width),
            .frame_incomplete = false,
        };
        display_write(display_device_, x, y, &desc, pixels);
        return;
    }
    if (backend_ == BackendKind::ManualSpi) {
        (void)manual_write_pixels(x, y, width, height, pixels, count);
        return;
    }
    if (backend_ == BackendKind::RawMipiDbi) {
        (void)raw_write_pixels(x, y, width, height, pixels, count);
    }
}

}  // namespace

std::unique_ptr<IZephyrShellDisplayBackend> make_tlora_pager_shell_display_backend(
    platform::Logger& logger,
    ZephyrBoardRuntime& runtime,
    ZephyrBoardBackendConfig config) {
    return std::make_unique<PagerDisplayBackend>(logger, runtime, std::move(config));
}

}  // namespace aegis::ports::zephyr
