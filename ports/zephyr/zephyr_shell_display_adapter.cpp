#include "ports/zephyr/zephyr_shell_display_adapter.hpp"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cctype>
#include <cstring>

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

const ::device* find_device(const std::string& name) {
    if (name.empty()) {
        return nullptr;
    }
    return device_get_binding(name.c_str());
}

std::string_view surface_name(shell::ShellSurface surface) {
    switch (surface) {
        case shell::ShellSurface::Home:
            return "home";
        case shell::ShellSurface::Launcher:
            return "launcher";
        case shell::ShellSurface::Settings:
            return "settings";
        case shell::ShellSurface::Notifications:
            return "notifications";
        case shell::ShellSurface::AppForeground:
            return "app_foreground";
        case shell::ShellSurface::Recovery:
            return "recovery";
    }

    return "unknown";
}

std::string hex_dump(const uint8_t* data, std::size_t len) {
    static constexpr char kHex[] = "0123456789ABCDEF";
    std::string result;
    for (std::size_t index = 0; index < len; ++index) {
        if (!result.empty()) {
            result.push_back(' ');
        }
        const auto value = data[index];
        result.push_back(kHex[(value >> 4) & 0x0F]);
        result.push_back(kHex[value & 0x0F]);
    }
    return result;
}

std::array<uint8_t, 7> glyph_rows(char ch) {
    switch (std::toupper(static_cast<unsigned char>(ch))) {
        case 'A': return {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11};
        case 'B': return {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E};
        case 'C': return {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E};
        case 'D': return {0x1C, 0x12, 0x11, 0x11, 0x11, 0x12, 0x1C};
        case 'E': return {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F};
        case 'F': return {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10};
        case 'G': return {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F};
        case 'H': return {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11};
        case 'I': return {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E};
        case 'J': return {0x01, 0x01, 0x01, 0x01, 0x11, 0x11, 0x0E};
        case 'K': return {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11};
        case 'L': return {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F};
        case 'M': return {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11};
        case 'N': return {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11};
        case 'O': return {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E};
        case 'P': return {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10};
        case 'Q': return {0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D};
        case 'R': return {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11};
        case 'S': return {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E};
        case 'T': return {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04};
        case 'U': return {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E};
        case 'V': return {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04};
        case 'W': return {0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0A};
        case 'X': return {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11};
        case 'Y': return {0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04};
        case 'Z': return {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F};
        case '0': return {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E};
        case '1': return {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E};
        case '2': return {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F};
        case '3': return {0x1F, 0x02, 0x04, 0x02, 0x01, 0x11, 0x0E};
        case '4': return {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02};
        case '5': return {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E};
        case '6': return {0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E};
        case '7': return {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08};
        case '8': return {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E};
        case '9': return {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x1C};
        case '-': return {0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00};
        case ':': return {0x00, 0x04, 0x04, 0x00, 0x04, 0x04, 0x00};
        case '.': return {0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x06};
        case '/': return {0x01, 0x02, 0x02, 0x04, 0x08, 0x08, 0x10};
        case '_': return {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F};
        case '+': return {0x00, 0x04, 0x04, 0x1F, 0x04, 0x04, 0x00};
        case '=': return {0x00, 0x1F, 0x00, 0x1F, 0x00, 0x00, 0x00};
        case '(': return {0x02, 0x04, 0x08, 0x08, 0x08, 0x04, 0x02};
        case ')': return {0x08, 0x04, 0x02, 0x02, 0x02, 0x04, 0x08};
        case ',': return {0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x08};
        case ' ': return {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        default: return {0x00, 0x0E, 0x01, 0x06, 0x04, 0x00, 0x04};
    }
}

constexpr int kGlyphWidth = 5;
constexpr int kGlyphHeight = 7;
constexpr int kGlyphAdvance = 6;
constexpr int kLineAdvance = 10;

constexpr uint8_t kSt7796sCmdSleepOut = 0x11;
constexpr uint8_t kSt7796sCmdSoftwareReset = 0x01;
constexpr uint8_t kSt7796sCmdDisplayInversionOff = 0x20;
constexpr uint8_t kSt7796sCmdDisplayInversionOn = 0x21;
constexpr uint8_t kSt7796sCmdColumnAddressSet = 0x2A;
constexpr uint8_t kSt7796sCmdRowAddressSet = 0x2B;
constexpr uint8_t kSt7796sCmdMemoryWrite = 0x2C;
constexpr uint8_t kSt7796sCmdDisplayOn = 0x29;
constexpr uint8_t kSt7796sCmdMadctl = 0x36;
constexpr uint8_t kSt7796sCmdColmod = 0x3A;
constexpr uint8_t kSt7796sCmdFrmctr1 = 0xB1;
constexpr uint8_t kSt7796sCmdFrmctr2 = 0xB2;
constexpr uint8_t kSt7796sCmdFrmctr3 = 0xB3;
constexpr uint8_t kSt7796sCmdDisplayInversionControl = 0xB4;
constexpr uint8_t kSt7796sCmdBlankingPorchControl = 0xB5;
constexpr uint8_t kSt7796sCmdDisplayFunctionControl = 0xB6;
constexpr uint8_t kSt7796sCmdPowerControl1 = 0xC0;
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
constexpr uint8_t kSt7796sControl16Bit = 0x05;
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

const ::device* dt_gpio0_device() {
#if DT_NODE_EXISTS(DT_NODELABEL(gpio0))
    return device_get_binding(DEVICE_DT_NAME(DT_NODELABEL(gpio0)));
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

}  // namespace

ZephyrShellDisplayAdapter::ZephyrShellDisplayAdapter(platform::Logger& logger,
                                                     ZephyrBoardRuntime& runtime,
                                                     ZephyrBoardBackendConfig config)
    : logger_(logger),
      runtime_(runtime),
      config_(std::move(config)),
      display_backend_(make_zephyr_shell_display_backend(logger_, runtime_, config_)) {
    k_mutex_init(&boot_log_mutex_);
}

bool ZephyrShellDisplayAdapter::initialize() {
    if (display_backend_ == nullptr || !display_backend_->initialize()) {
        logger_.info("display", "display backend unavailable");
        return false;
    }

    backend_ = BackendKind::DisplayApi;
    lvgl_ui_ = std::make_unique<ZephyrLvglShellUi>(
        logger_,
        config_,
        [this](int x, int y, int width, int height, const uint16_t* pixels, std::size_t count) {
            write_lvgl_region(x, y, width, height, pixels, count);
        });
    if (lvgl_ui_ != nullptr && !lvgl_ui_->initialize()) {
        logger_.info("lvgl", "shell ui init failed, keeping legacy renderer");
        lvgl_ui_.reset();
    }

    if (lvgl_ui_ == nullptr) {
        if (config_.display_smoke_test) {
            run_smoke_test();
        }
        fill_display(color_for(shell::ShellSurface::Home));
    }
    return true;
}

bool ZephyrShellDisplayAdapter::initialize_manual_spi_st7796_backend() {
    if (config_.display_cs_pin < 0 || config_.display_dc_pin < 0) {
        log_backend_state("manual-spi-missing-pins", -22);
        return false;
    }

    const auto* spi = dt_spi2_device();
    const auto* gpio = dt_gpio0_device();
    if (spi == nullptr || !device_is_ready(spi) || gpio == nullptr || !device_is_ready(gpio)) {
        log_backend_state("manual-spi-missing-devices", -19);
        return false;
    }

    manual_spi_device_ = spi;
    manual_gpio_device_ = gpio;
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
        backend_ = BackendKind::None;
        manual_spi_device_ = nullptr;
        manual_gpio_device_ = nullptr;
        log_backend_state("manual-spi-gpio-route-failed", -19);
        return false;
    }

    gpio_pin_configure(cs_ref.device, cs_ref.pin, GPIO_OUTPUT_ACTIVE);
    gpio_pin_set(cs_ref.device, cs_ref.pin, 1);
    gpio_pin_configure(dc_ref.device, dc_ref.pin, GPIO_OUTPUT_ACTIVE);
    gpio_pin_set(dc_ref.device, dc_ref.pin, 1);
    if (config_.display_backlight_pin >= 0) {
        const auto bl_ref = resolve_gpio_pin(config_.display_backlight_pin);
        if (bl_ref.device != nullptr && device_is_ready(bl_ref.device)) {
            gpio_pin_configure(bl_ref.device, bl_ref.pin, GPIO_OUTPUT_ACTIVE);
            gpio_pin_set(bl_ref.device, bl_ref.pin, 1);
        }
    }

    backend_ = BackendKind::ManualSpi;

    struct InitCommand {
        uint8_t cmd;
        std::array<uint8_t, 15> data;
        uint8_t len;
        bool delay_after;
    };

    const std::array<InitCommand, 19> init_sequence {{
        {0x01, {}, 0, true},
        {0x11, {}, 0, true},
        {0xF0, {0xC3}, 1, false},
        {0xF0, {0xC3}, 1, false},
        {0xF0, {0x96}, 1, false},
        {0x3A, {0x55}, 1, false},
        {0xB4, {0x01}, 1, false},
        {0xB6, {0x80, 0x02, 0x3B}, 3, false},
        {0xE8, {0x40, 0x8A, 0x00, 0x00, 0x29, 0x19, 0xA5, 0x33}, 8, false},
        {0xC1, {0x06}, 1, false},
        {0xC2, {0xA7}, 1, false},
        {0xC5, {0x18}, 1, true},
        {0xE0, {0xF0, 0x09, 0x0B, 0x06, 0x04, 0x15, 0x2F, 0x54, 0x42, 0x3C, 0x17, 0x14, 0x18, 0x1B}, 14, false},
        {0xE1, {0xE0, 0x09, 0x0B, 0x06, 0x04, 0x03, 0x2B, 0x43, 0x42, 0x3B, 0x16, 0x14, 0x17, 0x1B}, 14, false},
        {0xF0, {0x3C}, 1, false},
        {0xF0, {0x69}, 1, true},
        {0x21, {0x00}, 1, false},
        {0x29, {0x00}, 1, false},
    }};

    int rc = 0;
    for (const auto& step : init_sequence) {
        rc = manual_send_command(step.cmd, step.len == 0 ? nullptr : step.data.data(), step.len);
        if (rc != 0) {
            backend_ = BackendKind::None;
            manual_spi_device_ = nullptr;
            manual_gpio_device_ = nullptr;
            log_backend_state("manual-spi-init-failed", rc);
            return false;
        }
        if (step.delay_after) {
            k_sleep(K_MSEC(120));
        }
    }

    const uint8_t madctl = static_cast<uint8_t>(config_.display_madctl & 0xFF);
    rc = manual_send_command(kSt7796sCmdMadctl, &madctl, 1);
    if (rc != 0) {
        backend_ = BackendKind::None;
        manual_spi_device_ = nullptr;
        manual_gpio_device_ = nullptr;
        log_backend_state("manual-spi-madctl-failed", rc);
        return false;
    }

    log_manual_panel_diagnostics();
    log_backend_state("manual-spi-ready", 0);
    return true;
}

bool ZephyrShellDisplayAdapter::initialize_display_api_backend() {
    const auto* device = find_device(config_.display_device_name);
    if (device == nullptr) {
        device = dt_display_device();
    }
    if (device == nullptr || !device_is_ready(device)) {
        log_backend_state("display-api-missing", -19);
        return false;
    }

    const int rc = display_blanking_off(device);
    display_device_ = device;
    backend_ = BackendKind::DisplayApi;
    log_backend_state("display-api-ready", rc);
    return true;
}

bool ZephyrShellDisplayAdapter::initialize_raw_st7796_backend() {
    const auto* mipi = dt_mipi_device();
    if (mipi == nullptr || !device_is_ready(mipi)) {
        log_backend_state("raw-mipi-missing", -19);
        return false;
    }

    raw_mipi_device_ = mipi;
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
        param = static_cast<uint8_t>(kPagerMadctlInit);
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
        backend_ = BackendKind::None;
        raw_mipi_device_ = nullptr;
        log_backend_state("raw-mipi-failed", rc);
        return false;
    }

    log_backend_state("raw-mipi-ready", rc);
    return true;
}

void ZephyrShellDisplayAdapter::log_backend_state(std::string_view stage, int rc) const {
    printk("AEGIS TRACE: display stage=%s backend=%d rc=%d cfg-name=%s\n",
           std::string(stage).c_str(),
           static_cast<int>(backend_),
           rc,
           config_.display_device_name.c_str());
    logger_.info("display",
                 "stage=" + std::string(stage) + " backend=" +
                     std::to_string(static_cast<int>(backend_)) + " rc=" + std::to_string(rc));
}

void ZephyrShellDisplayAdapter::present(const shell::ShellPresentationFrame& frame) {
    if (backend_ == BackendKind::None) {
        return;
    }

    if (lvgl_ui_ != nullptr) {
        lvgl_ui_->present(frame);
        last_surface_valid_ = true;
        last_surface_ = frame.surface;
        last_frame_signature_ = frame_signature(frame);
        last_frame_available_ = true;
        last_frame_ = frame;
        log_frame(frame);
        return;
    }

    const uint32_t start_ms = k_uptime_get_32();
    const auto signature = frame_signature(frame);
    if (signature == last_frame_signature_) {
        return;
    }

    const int width = std::max(0, config_.display_width);
    const int height = std::max(0, config_.display_height);
    if (width <= 0 || height <= 0) {
        return;
    }

    int redraw_top = 0;
    int redraw_bottom = height;
    if (last_frame_available_ && last_frame_.surface == frame.surface) {
        const bool header_changed = last_frame_.headline != frame.headline ||
                                    last_frame_.detail != frame.detail ||
                                    last_frame_.context != frame.context;
        if (header_changed) {
            redraw_top = 34;
            redraw_bottom = std::min(height, 110);
        } else {
            redraw_top = first_body_line_y(frame, last_frame_);
            redraw_bottom = last_body_line_bottom(frame, last_frame_);
        }
    }

    if (redraw_bottom > redraw_top) {
        present_frame_range(frame, redraw_top, redraw_bottom);
    }
    last_surface_valid_ = true;
    last_surface_ = frame.surface;
    last_frame_signature_ = signature;
    last_frame_available_ = true;
    last_frame_ = frame;
    logger_.info("display",
                 "present complete surface=" + std::string(surface_name(frame.surface)) +
                     " elapsed-ms=" + std::to_string(k_uptime_get_32() - start_ms));
    log_frame(frame);
}

void ZephyrShellDisplayAdapter::tick(uint32_t elapsed_ms) {
    if (lvgl_ui_ != nullptr) {
        lvgl_ui_->tick(elapsed_ms);
    }
}

void ZephyrShellDisplayAdapter::record_boot_log(std::string_view category, std::string_view message) {
    if (!config_.display_boot_log_mirror || suppress_boot_log_capture_) {
        return;
    }

    if (k_mutex_lock(&boot_log_mutex_, K_MSEC(50)) != 0) {
        return;
    }
    std::string line = "[";
    line += std::string(category);
    line += "] ";
    line += std::string(message);
    boot_log_lines_.push_back(std::move(line));
    while (boot_log_lines_.size() > 18) {
        boot_log_lines_.pop_front();
    }
    (void)k_mutex_unlock(&boot_log_mutex_);
}

void ZephyrShellDisplayAdapter::write_lvgl_region(int x,
                                                  int y,
                                                  int width,
                                                  int height,
                                                  const uint16_t* pixels,
                                                  std::size_t count) const {
    if (display_backend_ == nullptr || pixels == nullptr || count == 0) {
        return;
    }
    display_backend_->write_region(x, y, width, height, pixels, count);
}

void ZephyrShellDisplayAdapter::present_boot_log_screen(std::string_view stage) const {
    if (backend_ == BackendKind::None) {
        return;
    }

    if (lvgl_ui_ != nullptr) {
        lvgl_ui_->show_startup_splash(stage);
        return;
    }

    suppress_boot_log_capture_ = true;
    last_surface_valid_ = false;
    fill_display(0xFFFF);
    fill_rect(0, 0, config_.display_width, 46, 0xF800);
    draw_text_block_scaled(10, 8, "AEGIS", 0xFFFF, 0xF800, 3);
    draw_text_block_scaled(124, 10, "BOOT", 0xFFFF, 0xF800, 2);
    fill_rect(0, 46, config_.display_width, 34, 0xFFE0);
    draw_text_block_scaled(10, 54, std::string(stage), 0x0000, 0xFFE0, 2);
    int y = 88;
    if (k_mutex_lock(&boot_log_mutex_, K_MSEC(50)) == 0) {
        for (const auto& line : boot_log_lines_) {
            if (y + (kGlyphHeight * 2) >= config_.display_height - 16) {
                break;
            }
            draw_text_block_scaled(8, y, line, 0x0000, 0xFFFF, 2);
            y += (kLineAdvance * 2);
        }
        (void)k_mutex_unlock(&boot_log_mutex_);
    }
    suppress_boot_log_capture_ = false;
}

uint16_t ZephyrShellDisplayAdapter::color_for(shell::ShellSurface surface) const {
    switch (surface) {
        case shell::ShellSurface::Home:
            return 0xFFFF;
        case shell::ShellSurface::Launcher:
            return 0xFFE0;
        case shell::ShellSurface::Settings:
            return 0x07FF;
        case shell::ShellSurface::Notifications:
            return 0xF81F;
        case shell::ShellSurface::AppForeground:
            return 0x07E0;
        case shell::ShellSurface::Recovery:
            return 0xF800;
    }

    return 0x0000;
}

void ZephyrShellDisplayAdapter::fill_display(uint16_t rgb565) const {
    uint16_t width = static_cast<uint16_t>(std::max(config_.display_width, 0));
    uint16_t height = static_cast<uint16_t>(std::max(config_.display_height, 0));
    if (backend_ == BackendKind::DisplayApi && display_device_ != nullptr &&
        device_is_ready(display_device_)) {
        display_capabilities caps {};
        display_get_capabilities(display_device_, &caps);
        width = caps.x_resolution;
        height = caps.y_resolution;
    }
    if (width == 0 || height == 0) {
        return;
    }

    constexpr std::size_t kMaxTransferPixels = 4096;
    const auto rows_per_chunk = static_cast<uint16_t>(
        std::max<std::size_t>(1, kMaxTransferPixels / std::max<std::size_t>(1, width)));
    const auto chunk_pixels = static_cast<std::size_t>(width) * rows_per_chunk;
    ensure_scratch_capacity(chunk_pixels);
    std::fill_n(scratch_pixels_.begin(), chunk_pixels, rgb565);

    for (uint16_t y = 0; y < height; y = static_cast<uint16_t>(y + rows_per_chunk)) {
        const auto remaining_rows = static_cast<uint16_t>(height - y);
        const auto chunk_rows = std::min<uint16_t>(rows_per_chunk, remaining_rows);
        const auto pixels = static_cast<std::size_t>(width) * chunk_rows;
        if (pixels != chunk_pixels) {
            std::fill_n(scratch_pixels_.begin(), pixels, rgb565);
        }
        write_region(0, y, width, chunk_rows, scratch_pixels_);
    }
}

void ZephyrShellDisplayAdapter::run_smoke_test() const {
    logger_.info("display", "smoke test begin");
    last_surface_valid_ = false;
    logger_.info("display", "smoke test step=1 fill_display white");
    fill_display(0xFFFF);
    logger_.info("display", "smoke test step=1 done");
    k_sleep(K_MSEC(500));
    logger_.info("display", "smoke test step=2 fill_display black");
    fill_display(0x0000);
    logger_.info("display", "smoke test step=2 done");
    k_sleep(K_MSEC(500));
    logger_.info("display", "smoke test step=3 fill_rect red");
    fill_rect(0, 0, config_.display_width / 3, config_.display_height, 0xF800);
    logger_.info("display", "smoke test step=3 done");
    logger_.info("display", "smoke test step=4 fill_rect green");
    fill_rect(config_.display_width / 3, 0, config_.display_width / 3, config_.display_height, 0x07E0);
    logger_.info("display", "smoke test step=4 done");
    logger_.info("display", "smoke test step=5 fill_rect blue");
    fill_rect((config_.display_width / 3) * 2,
              0,
              config_.display_width - ((config_.display_width / 3) * 2),
              config_.display_height,
              0x001F);
    logger_.info("display", "smoke test step=5 done");
    logger_.info("display", "smoke test step=6 text AEGIS");
    draw_text_block_scaled(16, 16, "AEGIS", 0xFFFF, 0xF800, 4);
    logger_.info("display", "smoke test step=6 done");
    logger_.info("display", "smoke test step=7 text DISPLAY");
    draw_text_block_scaled(16, 58, "DISPLAY", 0xFFFF, 0xF800, 3);
    logger_.info("display", "smoke test step=7 done");
    logger_.info("display", "smoke test step=8 text SMOKE");
    draw_text_block_scaled(16, 92, "SMOKE", 0xFFFF, 0xF800, 3);
    logger_.info("display", "smoke test step=8 done");
    logger_.info("display", "smoke test step=9 text TEST");
    draw_text_block_scaled(16, 126, "TEST", 0xFFFF, 0xF800, 3);
    logger_.info("display", "smoke test step=9 done");
    k_sleep(K_MSEC(2000));
    logger_.info("display", "smoke test complete");
}

void ZephyrShellDisplayAdapter::prepare_surface_background(
    const shell::ShellPresentationFrame& frame) const {
    const bool surface_changed = !last_surface_valid_ || last_surface_ != frame.surface;
    if (surface_changed) {
        fill_display(color_for(frame.surface));
        last_surface_valid_ = true;
        last_surface_ = frame.surface;
        return;
    }

    const uint16_t surface_bg = color_for(frame.surface);
    fill_rect(0, 26, config_.display_width, std::max(0, config_.display_height - 46), surface_bg);
}

void ZephyrShellDisplayAdapter::fill_rect(int x, int y, int width, int height, uint16_t rgb565) const {
    if (width <= 0 || height <= 0) {
        return;
    }

    constexpr std::size_t kMaxTransferPixels = 4096;
    const auto rows_per_chunk =
        std::max(1, static_cast<int>(kMaxTransferPixels / std::max(1, width)));
    const auto chunk_pixels = static_cast<std::size_t>(width) * rows_per_chunk;
    ensure_scratch_capacity(chunk_pixels);
    std::fill_n(scratch_pixels_.begin(), chunk_pixels, rgb565);

    for (int row = 0; row < height; row += rows_per_chunk) {
        const auto chunk_rows = std::min(rows_per_chunk, height - row);
        const auto pixels = static_cast<std::size_t>(width) * chunk_rows;
        if (pixels != chunk_pixels) {
            std::fill_n(scratch_pixels_.begin(), pixels, rgb565);
        }
        write_region(x, y + row, width, chunk_rows, scratch_pixels_);
    }
}

std::string ZephyrShellDisplayAdapter::frame_signature(
    const shell::ShellPresentationFrame& frame) const {
    std::string signature;
    signature.reserve(256);
    signature += std::to_string(static_cast<int>(frame.surface));
    signature += '|';
    signature += frame.headline;
    signature += '|';
    signature += frame.detail;
    signature += '|';
    signature += frame.context;
    for (const auto& line : frame.lines) {
        signature += '|';
        signature += line.emphasized ? '1' : '0';
        signature += ':';
        signature += line.text;
    }
    return signature;
}

void ZephyrShellDisplayAdapter::present_frame_range(const shell::ShellPresentationFrame& frame,
                                                    int y0,
                                                    int y1) const {
    constexpr int kBandRows = 48;
    const int height = std::max(0, config_.display_height);
    const int top = std::clamp(y0, 0, height);
    const int bottom = std::clamp(y1, top, height);
    for (int band_origin_y = top; band_origin_y < bottom; band_origin_y += kBandRows) {
        const int band_height = std::min(kBandRows, bottom - band_origin_y);
        render_frame_to_scratch(frame, band_origin_y, band_height);
        blit_scratch();
    }
}

int ZephyrShellDisplayAdapter::first_body_line_y(const shell::ShellPresentationFrame& frame,
                                                 const shell::ShellPresentationFrame& previous) const {
    const std::size_t min_lines = std::min(frame.lines.size(), previous.lines.size());
    int body_y = 114;
    for (std::size_t index = 0; index < min_lines; ++index) {
        const auto& current = frame.lines[index];
        const auto& prior = previous.lines[index];
        if (current.text != prior.text || current.emphasized != prior.emphasized) {
            return std::max(34, body_y - 4);
        }
        body_y += current.emphasized ? 28 : 20;
    }
    if (frame.lines.size() != previous.lines.size()) {
        return std::max(34, body_y - 4);
    }
    return std::max(0, config_.display_height - 20);
}

int ZephyrShellDisplayAdapter::last_body_line_bottom(const shell::ShellPresentationFrame& frame,
                                                     const shell::ShellPresentationFrame& previous) const {
    const auto line_extent = [](const shell::ShellPresentationFrame& target,
                                std::size_t count) -> int {
        int body_y = 114;
        for (std::size_t index = 0; index < count && index < target.lines.size(); ++index) {
            body_y += target.lines[index].emphasized ? 28 : 20;
        }
        return body_y;
    };

    const std::size_t max_lines = std::max(frame.lines.size(), previous.lines.size());
    std::size_t last_changed = static_cast<std::size_t>(-1);
    for (std::size_t index = 0; index < max_lines; ++index) {
        const bool current_exists = index < frame.lines.size();
        const bool previous_exists = index < previous.lines.size();
        if (current_exists != previous_exists) {
            last_changed = index;
            continue;
        }
        if (!current_exists) {
            continue;
        }
        if (frame.lines[index].text != previous.lines[index].text ||
            frame.lines[index].emphasized != previous.lines[index].emphasized) {
            last_changed = index;
        }
    }

    if (last_changed == static_cast<std::size_t>(-1)) {
        return std::max(0, config_.display_height - 20);
    }

    const int current_bottom = line_extent(frame, last_changed + 1);
    const int previous_bottom = line_extent(previous, last_changed + 1);
    const int dirty_bottom = std::max(current_bottom, previous_bottom) + 4;
    return std::clamp(dirty_bottom, 34, std::max(0, config_.display_height - 20));
}

void ZephyrShellDisplayAdapter::ensure_scratch_capacity(std::size_t pixels) const {
    scratch_pixels_.resize(pixels);
}

void ZephyrShellDisplayAdapter::render_frame_to_scratch(
    const shell::ShellPresentationFrame& frame,
    int band_origin_y,
    int band_height) const {
    const int width = std::max(0, config_.display_width);
    const int height = std::max(0, config_.display_height);
    if (width <= 0 || height <= 0 || band_height <= 0) {
        return;
    }

    scratch_band_origin_y_ = std::clamp(band_origin_y, 0, height);
    scratch_band_height_ = std::clamp(band_height, 0, height - scratch_band_origin_y_);
    ensure_scratch_capacity(static_cast<std::size_t>(width) * scratch_band_height_);
    std::fill_n(scratch_pixels_.begin(),
                static_cast<std::size_t>(width) * scratch_band_height_,
                color_for(frame.surface));

    const uint16_t surface_bg = color_for(frame.surface);
    const uint16_t header_bg = 0x0000;
    const uint16_t panel_bg = 0xF800;
    const uint16_t footer_bg = 0x1082;
    const uint16_t focus_bg = 0xFFE0;
    const uint16_t focus_border = 0xF800;

    scratch_fill_rect(0, 0, width, 26, header_bg);
    scratch_draw_text_block_scaled(8, 4, "AEGIS", 0xFFFF, header_bg, 2);
    scratch_draw_text_block_scaled(90, 6, std::string(surface_name(frame.surface)), 0xFFE0, header_bg, 1);

    scratch_fill_rect(8, 34, std::max(0, width - 16), 46, panel_bg);
    scratch_draw_text_block_scaled(14, 40, frame.headline, 0xFFFF, panel_bg, 2);
    scratch_draw_text_block_scaled(14, 60, frame.detail, 0xFFFF, panel_bg, 1);

    if (!frame.context.empty()) {
        scratch_fill_rect(8, 86, std::max(0, width - 16), 20, surface_bg);
        scratch_draw_text_block_scaled(14, 90, frame.context, 0x0000, surface_bg, 1);
    }

    scratch_fill_rect(8, 110, std::max(0, width - 16), std::max(0, height - 134), surface_bg);

    int body_y = 114;
    for (const auto& line : frame.lines) {
        if (body_y + (kGlyphHeight * 2) >= height - 24) {
            break;
        }
        if (line.emphasized) {
            scratch_fill_rect(8, body_y - 4, std::max(0, width - 16), 24, focus_border);
            scratch_fill_rect(12, body_y - 2, std::max(0, width - 24), 20, focus_bg);
            scratch_draw_text_block_scaled(18, body_y, line.text, 0x0000, focus_bg, 2);
            body_y += 28;
        } else {
            scratch_fill_rect(8, body_y - 2, std::max(0, width - 16), 18, surface_bg);
            scratch_draw_text_block_scaled(14, body_y, line.text, 0x0000, surface_bg, 1);
            body_y += 20;
        }
    }

    scratch_fill_rect(0, std::max(0, height - 20), width, 20, footer_bg);
    scratch_draw_text_block_scaled(8,
                                   std::max(0, height - 16),
                                   std::to_string(width) + "X" + std::to_string(height) + " " +
                                       config_.primary_input_name,
                                   0xFFFF,
                                   footer_bg,
                                   1);
}

void ZephyrShellDisplayAdapter::scratch_fill_rect(int x,
                                                  int y,
                                                  int width,
                                                  int height,
                                                  uint16_t rgb565) const {
    const int display_width = std::max(0, config_.display_width);
    const int display_height = std::max(0, config_.display_height);
    if (display_width <= 0 || display_height <= 0 || width <= 0 || height <= 0 ||
        scratch_band_height_ <= 0) {
        return;
    }

    const int x0 = std::clamp(x, 0, display_width);
    const int y0 = std::clamp(y, scratch_band_origin_y_, scratch_band_origin_y_ + scratch_band_height_);
    const int x1 = std::clamp(x + width, 0, display_width);
    const int y1 = std::clamp(y + height,
                              scratch_band_origin_y_,
                              scratch_band_origin_y_ + scratch_band_height_);
    for (int py = y0; py < y1; ++py) {
        auto* row = scratch_pixels_.data() +
                    (static_cast<std::size_t>(py - scratch_band_origin_y_) * display_width);
        std::fill(row + x0, row + x1, rgb565);
    }
}

void ZephyrShellDisplayAdapter::scratch_draw_text_block_scaled(int x,
                                                               int y,
                                                               std::string_view text,
                                                               uint16_t fg,
                                                               uint16_t bg,
                                                               int scale) const {
    const int safe_scale = std::max(1, scale);
    const int max_width = std::max(0, config_.display_width - x - 8);
    if (max_width < (kGlyphAdvance * safe_scale)) {
        return;
    }

    int cursor_x = x;
    int cursor_y = y;
    for (const char raw_ch : text) {
        if (raw_ch == '\n') {
            cursor_x = x;
            cursor_y += (kLineAdvance * safe_scale);
            continue;
        }
        if ((cursor_x - x) + (kGlyphAdvance * safe_scale) > max_width) {
            cursor_x = x;
            cursor_y += (kLineAdvance * safe_scale);
        }
        if (cursor_y + (kGlyphHeight * safe_scale) >= config_.display_height) {
            return;
        }

        scratch_draw_glyph_scaled(cursor_x, cursor_y, raw_ch, fg, bg, safe_scale);
        cursor_x += (kGlyphAdvance * safe_scale);
    }
}

void ZephyrShellDisplayAdapter::scratch_draw_glyph_scaled(int x,
                                                          int y,
                                                          char ch,
                                                          uint16_t fg,
                                                          uint16_t bg,
                                                          int scale) const {
    const auto rows = glyph_rows(ch);
    const int safe_scale = std::max(1, scale);
    for (int row = 0; row < kGlyphHeight; ++row) {
        for (int col = 0; col < kGlyphWidth; ++col) {
            const bool on = (rows[static_cast<std::size_t>(row)] &
                             (1u << (kGlyphWidth - 1 - col))) != 0;
            for (int sy = 0; sy < safe_scale; ++sy) {
                for (int sx = 0; sx < safe_scale; ++sx) {
                    scratch_write_pixel(x + (col * safe_scale) + sx,
                                        y + (row * safe_scale) + sy,
                                        on ? fg : bg);
                }
            }
        }
    }
}

void ZephyrShellDisplayAdapter::scratch_write_pixel(int x, int y, uint16_t rgb565) const {
    const int width = std::max(0, config_.display_width);
    const int height = std::max(0, config_.display_height);
    if (x < 0 || y < scratch_band_origin_y_ || x >= width ||
        y >= scratch_band_origin_y_ + scratch_band_height_ || y >= height) {
        return;
    }

    scratch_pixels_[static_cast<std::size_t>(y - scratch_band_origin_y_) * width +
                    static_cast<std::size_t>(x)] = rgb565;
}

void ZephyrShellDisplayAdapter::blit_scratch() const {
    const int width = std::max(0, config_.display_width);
    if (width <= 0 || scratch_band_height_ <= 0) {
        return;
    }

    write_region(0, scratch_band_origin_y_, width, scratch_band_height_, scratch_pixels_);
}

void ZephyrShellDisplayAdapter::present_text_overlay(const shell::ShellPresentationFrame& frame) const {
    const uint16_t surface_bg = color_for(frame.surface);
    const uint16_t header_bg = 0x0000;
    const uint16_t panel_bg = 0xF800;
    const uint16_t footer_bg = 0x1082;
    const uint16_t focus_bg = 0xFFE0;
    const uint16_t focus_border = 0xF800;
    fill_rect(0, 0, config_.display_width, 26, header_bg);
    draw_text_block_scaled(8, 4, "AEGIS", 0xFFFF, header_bg, 2);
    draw_text_block_scaled(90, 6, std::string(surface_name(frame.surface)), 0xFFE0, header_bg, 1);

    fill_rect(8, 34, std::max(0, config_.display_width - 16), 46, panel_bg);
    draw_text_block_scaled(14, 40, frame.headline, 0xFFFF, panel_bg, 2);
    draw_text_block_scaled(14, 60, frame.detail, 0xFFFF, panel_bg, 1);

    if (!frame.context.empty()) {
        fill_rect(8, 86, std::max(0, config_.display_width - 16), 20, surface_bg);
        draw_text_block_scaled(14, 90, frame.context, 0x0000, surface_bg, 1);
    }

    fill_rect(8, 110, std::max(0, config_.display_width - 16), std::max(0, config_.display_height - 134), surface_bg);

    int body_y = 114;
    for (const auto& line : frame.lines) {
        if (body_y + (kGlyphHeight * 2) >= config_.display_height - 24) {
            break;
        }
        if (line.emphasized) {
            fill_rect(8, body_y - 4, std::max(0, config_.display_width - 16), 24, focus_border);
            fill_rect(12, body_y - 2, std::max(0, config_.display_width - 24), 20, focus_bg);
            draw_text_block_scaled(18, body_y, line.text, 0x0000, focus_bg, 2);
            body_y += 28;
        } else {
            fill_rect(8, body_y - 2, std::max(0, config_.display_width - 16), 18, surface_bg);
            draw_text_block_scaled(14, body_y, line.text, 0x0000, surface_bg, 1);
            body_y += 20;
        }
    }

    fill_rect(0, std::max(0, config_.display_height - 20), config_.display_width, 20, footer_bg);
    draw_text_block_scaled(8,
                           std::max(0, config_.display_height - 16),
                           std::to_string(config_.display_width) + "X" +
                               std::to_string(config_.display_height) + " " + config_.primary_input_name,
                           0xFFFF,
                           footer_bg,
                           1);
}

void ZephyrShellDisplayAdapter::draw_text_block(int x,
                                                int y,
                                                std::string_view text,
                                                uint16_t fg,
                                                uint16_t bg) const {
    draw_text_block_scaled(x, y, text, fg, bg, 1);
}

void ZephyrShellDisplayAdapter::draw_text_block_scaled(int x,
                                                       int y,
                                                       std::string_view text,
                                                       uint16_t fg,
                                                       uint16_t bg,
                                                       int scale) const {
    const int safe_scale = std::max(1, scale);
    const int max_width = std::max(0, config_.display_width - x - 8);
    if (max_width < (kGlyphAdvance * safe_scale)) {
        return;
    }

    int cursor_x = x;
    int cursor_y = y;
    for (const char raw_ch : text) {
        if (raw_ch == '\n') {
            cursor_x = x;
            cursor_y += kLineAdvance;
            continue;
        }
        if ((cursor_x - x) + (kGlyphAdvance * safe_scale) > max_width) {
            cursor_x = x;
            cursor_y += (kLineAdvance * safe_scale);
        }
        if (cursor_y + (kGlyphHeight * safe_scale) >= config_.display_height) {
            return;
        }

        draw_glyph_scaled(cursor_x, cursor_y, raw_ch, fg, bg, safe_scale);
        cursor_x += (kGlyphAdvance * safe_scale);
    }
}

void ZephyrShellDisplayAdapter::draw_glyph(int x, int y, char ch, uint16_t fg, uint16_t bg) const {
    draw_glyph_scaled(x, y, ch, fg, bg, 1);
}

void ZephyrShellDisplayAdapter::draw_glyph_scaled(int x,
                                                  int y,
                                                  char ch,
                                                  uint16_t fg,
                                                  uint16_t bg,
                                                  int scale) const {
    const auto rows = glyph_rows(ch);
    const int safe_scale = std::max(1, scale);
    const int width = kGlyphWidth * safe_scale;
    const int height = kGlyphHeight * safe_scale;
    std::vector<uint16_t> pixels(static_cast<std::size_t>(width * height), bg);
    for (int row = 0; row < kGlyphHeight; ++row) {
        for (int col = 0; col < kGlyphWidth; ++col) {
            const bool on = (rows[static_cast<std::size_t>(row)] &
                             (1u << (kGlyphWidth - 1 - col))) != 0;
            for (int sy = 0; sy < safe_scale; ++sy) {
                for (int sx = 0; sx < safe_scale; ++sx) {
                    const int px = (col * safe_scale) + sx;
                    const int py = (row * safe_scale) + sy;
                    pixels[static_cast<std::size_t>(py * width + px)] = on ? fg : bg;
                }
            }
        }
    }
    write_region(x, y, width, height, pixels);
}

void ZephyrShellDisplayAdapter::write_region(int x,
                                             int y,
                                             int width,
                                             int height,
                                             const std::vector<uint16_t>& pixels) const {
    if (width <= 0 || height <= 0) {
        return;
    }
    if (display_backend_ != nullptr) {
        display_backend_->write_region(x, y, width, height, pixels.data(), pixels.size());
    }
}

bool ZephyrShellDisplayAdapter::manual_backend_ready() const {
    const auto cs_ref = resolve_gpio_pin(config_.display_cs_pin);
    const auto dc_ref = resolve_gpio_pin(config_.display_dc_pin);
    return backend_ == BackendKind::ManualSpi && manual_spi_device_ != nullptr &&
           device_is_ready(manual_spi_device_) && cs_ref.device != nullptr &&
           device_is_ready(cs_ref.device) && dc_ref.device != nullptr &&
           device_is_ready(dc_ref.device);
}

int ZephyrShellDisplayAdapter::manual_send_command(uint8_t cmd) const {
    return manual_send_command(cmd, nullptr, 0);
}

int ZephyrShellDisplayAdapter::manual_send_command(uint8_t cmd,
                                                   const uint8_t* data,
                                                   std::size_t len) const {
    if (runtime_.ready()) {
        return runtime_.with_coordination_domain(
            ZephyrBoardCoordinationDomain::DisplayPipeline,
            K_MSEC(50),
            "display.manual_send_command",
            [&]() { return manual_send_command_unlocked(cmd, data, len); });
    }
    return manual_send_command_unlocked(cmd, data, len);
}

int ZephyrShellDisplayAdapter::manual_send_command_unlocked(uint8_t cmd,
                                                            const uint8_t* data,
                                                            std::size_t len) const {
    if (!manual_backend_ready()) {
        return -19;
    }

    const auto cs_ref = resolve_gpio_pin(config_.display_cs_pin);
    const auto dc_ref = resolve_gpio_pin(config_.display_dc_pin);
    gpio_pin_set(cs_ref.device, cs_ref.pin, 0);
    gpio_pin_set(dc_ref.device, dc_ref.pin, 0);
    spi_buf cmd_buf {
        .buf = &cmd,
        .len = sizeof(cmd),
    };
    spi_buf_set cmd_set {
        .buffers = &cmd_buf,
        .count = 1,
    };
    int rc = spi_write(manual_spi_device_, &manual_spi_config_, &cmd_set);
    if (rc == 0 && data != nullptr && len > 0) {
        gpio_pin_set(dc_ref.device, dc_ref.pin, 1);
        spi_buf data_buf {
            .buf = const_cast<uint8_t*>(data),
            .len = len,
        };
        spi_buf_set data_set {
            .buffers = &data_buf,
            .count = 1,
        };
        rc = spi_write(manual_spi_device_, &manual_spi_config_, &data_set);
    }
    gpio_pin_set(cs_ref.device, cs_ref.pin, 1);
    return rc;
}

int ZephyrShellDisplayAdapter::manual_send_data(const uint8_t* data, std::size_t len) const {
    if (runtime_.ready()) {
        return runtime_.with_coordination_domain(
            ZephyrBoardCoordinationDomain::DisplayPipeline,
            K_MSEC(50),
            "display.manual_send_data",
            [&]() { return manual_send_data_unlocked(data, len); });
    }
    return manual_send_data_unlocked(data, len);
}

int ZephyrShellDisplayAdapter::manual_send_data_unlocked(const uint8_t* data, std::size_t len) const {
    if (!manual_backend_ready()) {
        return -19;
    }
    if (data == nullptr || len == 0) {
        return 0;
    }

    const auto cs_ref = resolve_gpio_pin(config_.display_cs_pin);
    const auto dc_ref = resolve_gpio_pin(config_.display_dc_pin);
    gpio_pin_set(cs_ref.device, cs_ref.pin, 0);
    gpio_pin_set(dc_ref.device, dc_ref.pin, 1);
    spi_buf data_buf {
        .buf = const_cast<uint8_t*>(data),
        .len = len,
    };
    spi_buf_set data_set {
        .buffers = &data_buf,
        .count = 1,
    };
    const int rc = spi_write(manual_spi_device_, &manual_spi_config_, &data_set);
    gpio_pin_set(cs_ref.device, cs_ref.pin, 1);
    return rc;
}

int ZephyrShellDisplayAdapter::manual_read_command(uint8_t cmd, uint8_t* data, std::size_t len) const {
    if (runtime_.ready()) {
        return runtime_.with_coordination_domain(
            ZephyrBoardCoordinationDomain::DisplayPipeline,
            K_MSEC(50),
            "display.manual_read_command",
            [&]() { return manual_read_command_unlocked(cmd, data, len); });
    }
    return manual_read_command_unlocked(cmd, data, len);
}

int ZephyrShellDisplayAdapter::manual_read_command_unlocked(uint8_t cmd,
                                                            uint8_t* data,
                                                            std::size_t len) const {
    if (!manual_backend_ready()) {
        return -19;
    }
    if (data == nullptr || len == 0) {
        return 0;
    }

    const auto cs_ref = resolve_gpio_pin(config_.display_cs_pin);
    const auto dc_ref = resolve_gpio_pin(config_.display_dc_pin);
    std::vector<uint8_t> tx(len, 0x00);
    std::vector<uint8_t> rx(len, 0x00);

    gpio_pin_set(cs_ref.device, cs_ref.pin, 0);
    gpio_pin_set(dc_ref.device, dc_ref.pin, 0);

    spi_buf cmd_buf {
        .buf = &cmd,
        .len = sizeof(cmd),
    };
    spi_buf_set cmd_set {
        .buffers = &cmd_buf,
        .count = 1,
    };
    int rc = spi_write(manual_spi_device_, &manual_spi_config_, &cmd_set);
    if (rc == 0) {
        gpio_pin_set(dc_ref.device, dc_ref.pin, 1);
        spi_buf tx_buf {
            .buf = tx.data(),
            .len = len,
        };
        spi_buf rx_buf {
            .buf = rx.data(),
            .len = len,
        };
        spi_buf_set tx_set {
            .buffers = &tx_buf,
            .count = 1,
        };
        spi_buf_set rx_set {
            .buffers = &rx_buf,
            .count = 1,
        };
        rc = spi_transceive(manual_spi_device_, &manual_spi_config_, &tx_set, &rx_set);
        if (rc == 0) {
            std::memcpy(data, rx.data(), len);
        }
    }

    gpio_pin_set(cs_ref.device, cs_ref.pin, 1);
    gpio_pin_set(dc_ref.device, dc_ref.pin, 1);
    return rc;
}

void ZephyrShellDisplayAdapter::log_manual_panel_diagnostics() const {
    if (!manual_backend_ready()) {
        return;
    }

    struct Probe {
        uint8_t cmd;
        const char* name;
        std::size_t len;
    };

    const std::array<Probe, 4> probes {{
        {0x04, "read-id", 4},
        {0x09, "status", 5},
        {0x0A, "power-mode", 2},
        {0x0B, "madctl", 2},
    }};

    for (const auto& probe : probes) {
        std::array<uint8_t, 5> response {};
        const int rc = manual_read_command(probe.cmd, response.data(), probe.len);
        logger_.info("display",
                     "panel probe " + std::string(probe.name) +
                         " cmd=0x" + hex_dump(&probe.cmd, 1) +
                         " rc=" + std::to_string(rc) +
                         " data=" + hex_dump(response.data(), probe.len));
    }
}

int ZephyrShellDisplayAdapter::manual_set_cursor(uint16_t x,
                                                 uint16_t y,
                                                 uint16_t width,
                                                 uint16_t height) const {
    const uint16_t xs = static_cast<uint16_t>(x + config_.display_x_gap);
    const uint16_t ys = static_cast<uint16_t>(y + config_.display_y_gap);
    const uint16_t xe = static_cast<uint16_t>(xs + width - 1);
    const uint16_t ye = static_cast<uint16_t>(ys + height - 1);

    const std::array<uint8_t, 4> col_params {
        static_cast<uint8_t>(xs >> 8),
        static_cast<uint8_t>(xs & 0xFF),
        static_cast<uint8_t>(xe >> 8),
        static_cast<uint8_t>(xe & 0xFF),
    };
    const std::array<uint8_t, 4> row_params {
        static_cast<uint8_t>(ys >> 8),
        static_cast<uint8_t>(ys & 0xFF),
        static_cast<uint8_t>(ye >> 8),
        static_cast<uint8_t>(ye & 0xFF),
    };

    int rc = manual_send_command(kSt7796sCmdColumnAddressSet, col_params.data(), col_params.size());
    if (rc != 0) {
        return rc;
    }
    rc = manual_send_command(kSt7796sCmdRowAddressSet, row_params.data(), row_params.size());
    if (rc != 0) {
        return rc;
    }
    return manual_send_command(kSt7796sCmdMemoryWrite);
}

int ZephyrShellDisplayAdapter::manual_write_pixels(int x,
                                                   int y,
                                                   int width,
                                                   int height,
                                                   const std::vector<uint16_t>& pixels) const {
    if (runtime_.ready()) {
        return runtime_.with_coordination_domain(
            ZephyrBoardCoordinationDomain::DisplayPipeline,
            K_MSEC(50),
            "display.manual_write_pixels",
            [&]() { return manual_write_pixels_unlocked(x, y, width, height, pixels); });
    }
    return manual_write_pixels_unlocked(x, y, width, height, pixels);
}

int ZephyrShellDisplayAdapter::manual_write_pixels_unlocked(
    int x,
    int y,
    int width,
    int height,
    const std::vector<uint16_t>& pixels) const {
    if (!manual_backend_ready()) {
        return -19;
    }

    const uint16_t xs = static_cast<uint16_t>(x + config_.display_x_gap);
    const uint16_t ys = static_cast<uint16_t>(y + config_.display_y_gap);
    const uint16_t xe = static_cast<uint16_t>(xs + width - 1);
    const uint16_t ye = static_cast<uint16_t>(ys + height - 1);
    const std::array<uint8_t, 4> col_params {
        static_cast<uint8_t>(xs >> 8),
        static_cast<uint8_t>(xs & 0xFF),
        static_cast<uint8_t>(xe >> 8),
        static_cast<uint8_t>(xe & 0xFF),
    };
    const std::array<uint8_t, 4> row_params {
        static_cast<uint8_t>(ys >> 8),
        static_cast<uint8_t>(ys & 0xFF),
        static_cast<uint8_t>(ye >> 8),
        static_cast<uint8_t>(ye & 0xFF),
    };

    auto write_bytes = [&](const uint8_t* data, std::size_t len, bool command) -> int {
        const auto dc_ref = resolve_gpio_pin(config_.display_dc_pin);
        gpio_pin_set(dc_ref.device, dc_ref.pin, command ? 0 : 1);
        spi_buf buf {
            .buf = const_cast<uint8_t*>(data),
            .len = len,
        };
        spi_buf_set set {
            .buffers = &buf,
            .count = 1,
        };
        return spi_write(manual_spi_device_, &manual_spi_config_, &set);
    };

    const uint8_t col_cmd = kSt7796sCmdColumnAddressSet;
    const uint8_t row_cmd = kSt7796sCmdRowAddressSet;
    const uint8_t ramwr_cmd = kSt7796sCmdMemoryWrite;

    const auto cs_ref = resolve_gpio_pin(config_.display_cs_pin);
    const auto dc_ref = resolve_gpio_pin(config_.display_dc_pin);
    gpio_pin_set(cs_ref.device, cs_ref.pin, 0);
    int rc = write_bytes(&col_cmd, sizeof(col_cmd), true);
    if (rc == 0) {
        rc = write_bytes(col_params.data(), col_params.size(), false);
    }
    if (rc == 0) {
        rc = write_bytes(&row_cmd, sizeof(row_cmd), true);
    }
    if (rc == 0) {
        rc = write_bytes(row_params.data(), row_params.size(), false);
    }
    if (rc == 0) {
        rc = write_bytes(&ramwr_cmd, sizeof(ramwr_cmd), true);
    }
    if (rc == 0) {
        rc = write_bytes(reinterpret_cast<const uint8_t*>(pixels.data()),
                         pixels.size() * sizeof(uint16_t),
                         false);
    }
    gpio_pin_set(cs_ref.device, cs_ref.pin, 1);
    gpio_pin_set(dc_ref.device, dc_ref.pin, 1);
    return rc;
}

bool ZephyrShellDisplayAdapter::raw_backend_ready() const {
    return backend_ == BackendKind::RawMipiDbi && raw_mipi_device_ != nullptr &&
           device_is_ready(raw_mipi_device_);
}

int ZephyrShellDisplayAdapter::raw_panel_x(int x, int y) const {
    ARG_UNUSED(y);
    return x + config_.display_x_gap;
}

int ZephyrShellDisplayAdapter::raw_panel_y(int x, int y) const {
    ARG_UNUSED(x);
    return y + config_.display_y_gap;
}

int ZephyrShellDisplayAdapter::raw_set_cursor(uint16_t x,
                                              uint16_t y,
                                              uint16_t width,
                                              uint16_t height) const {
    const auto x0 = static_cast<uint16_t>(raw_panel_x(x, y));
    const auto y0 = static_cast<uint16_t>(raw_panel_y(x, y));
    uint16_t address_data[2];
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

int ZephyrShellDisplayAdapter::raw_write_pixels(int x,
                                                int y,
                                                int width,
                                                int height,
                                                const std::vector<uint16_t>& pixels) const {
    if (runtime_.ready()) {
        return runtime_.with_coordination_domain(
            ZephyrBoardCoordinationDomain::DisplayPipeline,
            K_MSEC(50),
            "display.raw_write_pixels",
            [&]() { return raw_write_pixels_unlocked(x, y, width, height, pixels); });
    }
    return raw_write_pixels_unlocked(x, y, width, height, pixels);
}

int ZephyrShellDisplayAdapter::raw_write_pixels_unlocked(int x,
                                                         int y,
                                                         int width,
                                                         int height,
                                                         const std::vector<uint16_t>& pixels) const {
    if (!raw_backend_ready()) {
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
        .buf_size = static_cast<uint32_t>(pixels.size() * sizeof(uint16_t)),
        .width = static_cast<uint16_t>(width),
        .height = static_cast<uint16_t>(height),
        .pitch = static_cast<uint16_t>(width),
        .frame_incomplete = false,
    };
    auto raw_config = make_raw_mipi_config();
    return mipi_dbi_write_display(raw_mipi_device_,
                                  &raw_config,
                                  reinterpret_cast<const uint8_t*>(pixels.data()),
                                  &desc,
                                  PIXEL_FORMAT_RGB_565);
}

int ZephyrShellDisplayAdapter::raw_send_command(uint8_t cmd,
                                                const uint8_t* data,
                                                std::size_t len) const {
    if (runtime_.ready()) {
        return runtime_.with_coordination_domain(
            ZephyrBoardCoordinationDomain::DisplayPipeline,
            K_MSEC(50),
            "display.raw_send_command",
            [&]() { return raw_send_command_unlocked(cmd, data, len); });
    }
    return raw_send_command_unlocked(cmd, data, len);
}

int ZephyrShellDisplayAdapter::raw_send_command_unlocked(uint8_t cmd,
                                                         const uint8_t* data,
                                                         std::size_t len) const {
    if (!raw_backend_ready()) {
        return -19;
    }

    auto raw_config = make_raw_mipi_config();
    return mipi_dbi_command_write(raw_mipi_device_, &raw_config, cmd, data, len);
}

void ZephyrShellDisplayAdapter::log_frame(const shell::ShellPresentationFrame& frame) const {
    logger_.info("display",
                 "frame surface=" + std::string(surface_name(frame.surface)) +
                     " headline=" + frame.headline + " detail=" + frame.detail +
                     " context=" + frame.context + " lines=" +
                     std::to_string(frame.lines.size()));
    for (std::size_t index = 0; index < frame.lines.size() && index < 4; ++index) {
        logger_.info("display",
                     "frame line[" + std::to_string(index) + "]=" + frame.lines[index].text +
                         (frame.lines[index].emphasized ? " *" : ""));
    }
}

}  // namespace aegis::ports::zephyr
