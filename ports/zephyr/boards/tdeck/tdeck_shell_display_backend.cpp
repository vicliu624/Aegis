#include "ports/zephyr/boards/tdeck/tdeck_shell_display_backend.hpp"

#include <array>
#include <cstddef>
#include <string_view>
#include <utility>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include "ports/zephyr/zephyr_gpio_pin.hpp"

namespace aegis::ports::zephyr {

namespace {

constexpr uint8_t kSt7796sCmdColumnAddressSet = 0x2A;
constexpr uint8_t kSt7796sCmdRowAddressSet = 0x2B;
constexpr uint8_t kSt7796sCmdMemoryWrite = 0x2C;
constexpr uint8_t kSt7796sCmdMadctl = 0x36;

const ::device* dt_spi2_device() {
#if DT_NODE_EXISTS(DT_NODELABEL(spi2))
    return device_get_binding(DEVICE_DT_NAME(DT_NODELABEL(spi2)));
#else
    return nullptr;
#endif
}

class TdeckDisplayBackend final : public IZephyrShellDisplayBackend {
public:
    TdeckDisplayBackend(platform::Logger& logger,
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
    [[nodiscard]] bool backend_ready() const;
    void log_backend_state(std::string_view stage, int rc) const;
    [[nodiscard]] int send_command(uint8_t cmd, const uint8_t* data, std::size_t len) const;
    [[nodiscard]] int send_command_unlocked(uint8_t cmd, const uint8_t* data, std::size_t len) const;
    [[nodiscard]] int write_pixels(int x,
                                   int y,
                                   int width,
                                   int height,
                                   const uint16_t* pixels,
                                   std::size_t count) const;
    [[nodiscard]] int write_pixels_unlocked(int x,
                                            int y,
                                            int width,
                                            int height,
                                            const uint16_t* pixels,
                                            std::size_t count) const;

    platform::Logger& logger_;
    ZephyrBoardRuntime& runtime_;
    ZephyrBoardBackendConfig config_;
    const struct device* spi_device_ {nullptr};
    spi_config spi_config_ {};
};

bool TdeckDisplayBackend::initialize() {
    if (config_.display_cs_pin < 0 || config_.display_dc_pin < 0) {
        log_backend_state("tdeck-missing-pins", -22);
        return false;
    }

    spi_device_ = dt_spi2_device();
    if (spi_device_ == nullptr || !device_is_ready(spi_device_)) {
        log_backend_state("tdeck-missing-spi", -19);
        return false;
    }

    spi_config_ = {};
    spi_config_.frequency = 40000000U;
    spi_config_.operation =
        static_cast<spi_operation_t>(SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB);
    spi_config_.slave = 0;
    spi_config_.cs = {};
    spi_config_.cs.gpio.port = nullptr;
    spi_config_.cs.cs_is_gpio = false;

    const auto cs_ref = resolve_gpio_pin(config_.display_cs_pin);
    const auto dc_ref = resolve_gpio_pin(config_.display_dc_pin);
    const auto bl_ref = resolve_gpio_pin(config_.display_backlight_pin);
    if (cs_ref.device == nullptr || !device_is_ready(cs_ref.device) ||
        dc_ref.device == nullptr || !device_is_ready(dc_ref.device)) {
        spi_device_ = nullptr;
        log_backend_state("tdeck-gpio-route-failed", -19);
        return false;
    }

    (void)gpio_pin_configure(cs_ref.device, cs_ref.pin, GPIO_OUTPUT_ACTIVE);
    (void)gpio_pin_set(cs_ref.device, cs_ref.pin, 1);
    (void)gpio_pin_configure(dc_ref.device, dc_ref.pin, GPIO_OUTPUT_ACTIVE);
    (void)gpio_pin_set(dc_ref.device, dc_ref.pin, 1);
    if (bl_ref.device != nullptr && device_is_ready(bl_ref.device)) {
        (void)gpio_pin_configure(bl_ref.device, bl_ref.pin, GPIO_OUTPUT_ACTIVE);
        (void)gpio_pin_set(bl_ref.device, bl_ref.pin, 1);
    }

    struct InitCommand {
        uint8_t cmd;
        std::array<uint8_t, 14> data;
        uint8_t len;
        bool delay_after;
    };

    const std::array<InitCommand, 18> init_sequence {{
        {0x11, {}, 0, true},
        {0x13, {}, 0, false},
        {0x3A, {0x55}, 1, false},
        {0xB2, {0x0C, 0x0C, 0x00, 0x33, 0x33}, 5, false},
        {0xB7, {0x75}, 1, false},
        {0xBB, {0x1A}, 1, false},
        {0xC0, {0x2C}, 1, false},
        {0xC2, {0x01}, 1, false},
        {0xC3, {0x13}, 1, false},
        {0xC4, {0x20}, 1, false},
        {0xC6, {0x0F}, 1, false},
        {0xD0, {0xA4, 0xA1}, 2, false},
        {0xE0, {0xD0, 0x0D, 0x14, 0x0D, 0x0D, 0x09, 0x38, 0x44, 0x4E, 0x3A, 0x17, 0x18, 0x2F, 0x30}, 14, false},
        {0xE1, {0xD0, 0x09, 0x0F, 0x08, 0x07, 0x14, 0x37, 0x44, 0x4D, 0x38, 0x15, 0x16, 0x2C, 0x3E}, 14, false},
        {0x21, {}, 0, false},
        {0x2A, {0x00, 0x00, 0x00, 0xEF}, 4, false},
        {0x2B, {0x00, 0x00, 0x01, 0x3F}, 4, false},
        {0x29, {}, 0, true},
    }};

    for (const auto& step : init_sequence) {
        const int rc = send_command(step.cmd, step.len == 0 ? nullptr : step.data.data(), step.len);
        if (rc != 0) {
            spi_device_ = nullptr;
            log_backend_state("tdeck-init-failed", rc);
            return false;
        }
        if (step.delay_after) {
            k_sleep(K_MSEC(120));
        }
    }

    const uint8_t madctl = static_cast<uint8_t>(config_.display_madctl & 0xFF);
    const int madctl_rc = send_command(kSt7796sCmdMadctl, &madctl, 1);
    if (madctl_rc != 0) {
        spi_device_ = nullptr;
        log_backend_state("tdeck-madctl-failed", madctl_rc);
        return false;
    }

    log_backend_state("tdeck-ready", 0);
    return true;
}

void TdeckDisplayBackend::write_region(int x,
                                       int y,
                                       int width,
                                       int height,
                                       const uint16_t* pixels,
                                       std::size_t count) const {
    (void)write_pixels(x, y, width, height, pixels, count);
}

bool TdeckDisplayBackend::backend_ready() const {
    const auto cs_ref = resolve_gpio_pin(config_.display_cs_pin);
    const auto dc_ref = resolve_gpio_pin(config_.display_dc_pin);
    return spi_device_ != nullptr && device_is_ready(spi_device_) &&
           cs_ref.device != nullptr && device_is_ready(cs_ref.device) &&
           dc_ref.device != nullptr && device_is_ready(dc_ref.device);
}

void TdeckDisplayBackend::log_backend_state(std::string_view stage, int rc) const {
    printk("AEGIS TRACE: display stage=%s rc=%d cfg-name=%s\n",
           std::string(stage).c_str(),
           rc,
           config_.display_device_name.c_str());
    logger_.info("display", "stage=" + std::string(stage) + " rc=" + std::to_string(rc));
}

int TdeckDisplayBackend::send_command(uint8_t cmd, const uint8_t* data, std::size_t len) const {
    if (runtime_.ready()) {
        return runtime_.with_coordination_domain(ZephyrBoardCoordinationDomain::DisplayPipeline,
                                                 K_MSEC(50),
                                                 "display.tdeck_send_command",
                                                 [&]() { return send_command_unlocked(cmd, data, len); });
    }
    return send_command_unlocked(cmd, data, len);
}

int TdeckDisplayBackend::send_command_unlocked(uint8_t cmd, const uint8_t* data, std::size_t len) const {
    if (!backend_ready()) {
        return -19;
    }
    const auto cs_ref = resolve_gpio_pin(config_.display_cs_pin);
    const auto dc_ref = resolve_gpio_pin(config_.display_dc_pin);
    (void)gpio_pin_set(cs_ref.device, cs_ref.pin, 0);
    (void)gpio_pin_set(dc_ref.device, dc_ref.pin, 0);
    spi_buf cmd_buf {.buf = &cmd, .len = sizeof(cmd)};
    spi_buf_set cmd_set {.buffers = &cmd_buf, .count = 1};
    int rc = spi_write(spi_device_, &spi_config_, &cmd_set);
    if (rc == 0 && data != nullptr && len > 0) {
        (void)gpio_pin_set(dc_ref.device, dc_ref.pin, 1);
        spi_buf data_buf {.buf = const_cast<uint8_t*>(data), .len = len};
        spi_buf_set data_set {.buffers = &data_buf, .count = 1};
        rc = spi_write(spi_device_, &spi_config_, &data_set);
    }
    (void)gpio_pin_set(cs_ref.device, cs_ref.pin, 1);
    return rc;
}

int TdeckDisplayBackend::write_pixels(int x,
                                      int y,
                                      int width,
                                      int height,
                                      const uint16_t* pixels,
                                      std::size_t count) const {
    if (runtime_.ready()) {
        return runtime_.with_coordination_domain(ZephyrBoardCoordinationDomain::DisplayPipeline,
                                                 K_MSEC(50),
                                                 "display.tdeck_write_pixels",
                                                 [&]() { return write_pixels_unlocked(x, y, width, height, pixels, count); });
    }
    return write_pixels_unlocked(x, y, width, height, pixels, count);
}

int TdeckDisplayBackend::write_pixels_unlocked(int x,
                                               int y,
                                               int width,
                                               int height,
                                               const uint16_t* pixels,
                                               std::size_t count) const {
    if (!backend_ready() || pixels == nullptr || count == 0) {
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
    const auto cs_ref = resolve_gpio_pin(config_.display_cs_pin);
    const auto dc_ref = resolve_gpio_pin(config_.display_dc_pin);

    auto write_bytes = [&](const uint8_t* data, std::size_t len, bool command) -> int {
        (void)gpio_pin_set(dc_ref.device, dc_ref.pin, command ? 0 : 1);
        spi_buf buf {.buf = const_cast<uint8_t*>(data), .len = len};
        spi_buf_set set {.buffers = &buf, .count = 1};
        return spi_write(spi_device_, &spi_config_, &set);
    };

    const uint8_t col_cmd = kSt7796sCmdColumnAddressSet;
    const uint8_t row_cmd = kSt7796sCmdRowAddressSet;
    const uint8_t ramwr_cmd = kSt7796sCmdMemoryWrite;
    (void)gpio_pin_set(cs_ref.device, cs_ref.pin, 0);
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
        rc = write_bytes(reinterpret_cast<const uint8_t*>(pixels),
                         count * sizeof(uint16_t),
                         false);
    }
    (void)gpio_pin_set(cs_ref.device, cs_ref.pin, 1);
    (void)gpio_pin_set(dc_ref.device, dc_ref.pin, 1);
    return rc;
}

}  // namespace

std::unique_ptr<IZephyrShellDisplayBackend> make_tdeck_shell_display_backend(
    platform::Logger& logger,
    ZephyrBoardRuntime& runtime,
    ZephyrBoardBackendConfig config) {
    return std::make_unique<TdeckDisplayBackend>(logger, runtime, std::move(config));
}

}  // namespace aegis::ports::zephyr
