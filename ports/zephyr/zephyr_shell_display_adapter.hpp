#pragma once

#include <cstdint>
#include <deque>
#include <memory>
#include <string_view>
#include <string>
#include <string>
#include <utility>
#include <vector>

#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>

#include "platform/logging/logger.hpp"
#include "ports/zephyr/zephyr_board_backend_config.hpp"
#include "ports/zephyr/zephyr_board_runtime.hpp"
#include "ports/zephyr/zephyr_lvgl_shell_ui.hpp"
#include "ports/zephyr/zephyr_shell_display_backend.hpp"
#include "shell/presentation/shell_presentation_sink.hpp"

namespace aegis::ports::zephyr {

class ZephyrShellDisplayAdapter : public shell::ShellPresentationSink {
public:
    ZephyrShellDisplayAdapter(platform::Logger& logger,
                              ZephyrBoardRuntime& runtime,
                              ZephyrBoardBackendConfig config);

    [[nodiscard]] bool initialize();
    void present(const shell::ShellPresentationFrame& frame) override;
    void record_boot_log(std::string_view category, std::string_view message);
    void present_boot_log_screen(std::string_view stage) const;
    void tick(uint32_t elapsed_ms);

private:
    enum class BackendKind {
        None,
        ManualSpi,
        DisplayApi,
        RawMipiDbi,
    };

    [[nodiscard]] uint16_t color_for(shell::ShellSurface surface) const;
    [[nodiscard]] bool initialize_manual_spi_st7796_backend();
    [[nodiscard]] bool initialize_display_api_backend();
    [[nodiscard]] bool initialize_raw_st7796_backend();
    void log_backend_state(std::string_view stage, int rc) const;
    void fill_display(uint16_t rgb565) const;
    void run_smoke_test() const;
    void present_text_overlay(const shell::ShellPresentationFrame& frame) const;
    void fill_rect(int x, int y, int width, int height, uint16_t rgb565) const;
    void draw_text_block(int x, int y, std::string_view text, uint16_t fg, uint16_t bg) const;
    void draw_text_block_scaled(int x,
                                int y,
                                std::string_view text,
                                uint16_t fg,
                                uint16_t bg,
                                int scale) const;
    void draw_glyph(int x, int y, char ch, uint16_t fg, uint16_t bg) const;
    void draw_glyph_scaled(int x, int y, char ch, uint16_t fg, uint16_t bg, int scale) const;
    void write_region(int x, int y, int width, int height, const std::vector<uint16_t>& pixels) const;
    void log_frame(const shell::ShellPresentationFrame& frame) const;
    [[nodiscard]] bool raw_backend_ready() const;
    [[nodiscard]] int raw_panel_x(int x, int y) const;
    [[nodiscard]] int raw_panel_y(int x, int y) const;
    [[nodiscard]] int raw_set_cursor(uint16_t x, uint16_t y, uint16_t width, uint16_t height) const;
    [[nodiscard]] int raw_write_pixels(int x,
                                       int y,
                                       int width,
                                       int height,
                                       const std::vector<uint16_t>& pixels) const;
    [[nodiscard]] int raw_write_pixels_unlocked(int x,
                                                int y,
                                                int width,
                                                int height,
                                                const std::vector<uint16_t>& pixels) const;
    [[nodiscard]] int raw_send_command(uint8_t cmd, const uint8_t* data, std::size_t len) const;
    [[nodiscard]] int raw_send_command_unlocked(uint8_t cmd,
                                                const uint8_t* data,
                                                std::size_t len) const;
    [[nodiscard]] bool manual_backend_ready() const;
    [[nodiscard]] int manual_set_cursor(uint16_t x, uint16_t y, uint16_t width, uint16_t height) const;
    [[nodiscard]] int manual_write_pixels(int x,
                                          int y,
                                          int width,
                                          int height,
                                          const std::vector<uint16_t>& pixels) const;
    [[nodiscard]] int manual_write_pixels_unlocked(int x,
                                                   int y,
                                                   int width,
                                                   int height,
                                                   const std::vector<uint16_t>& pixels) const;
    [[nodiscard]] int manual_send_command(uint8_t cmd) const;
    [[nodiscard]] int manual_send_command(uint8_t cmd, const uint8_t* data, std::size_t len) const;
    [[nodiscard]] int manual_send_command_unlocked(uint8_t cmd,
                                                   const uint8_t* data,
                                                   std::size_t len) const;
    [[nodiscard]] int manual_send_data(const uint8_t* data, std::size_t len) const;
    [[nodiscard]] int manual_send_data_unlocked(const uint8_t* data, std::size_t len) const;
    [[nodiscard]] int manual_read_command(uint8_t cmd, uint8_t* data, std::size_t len) const;
    [[nodiscard]] int manual_read_command_unlocked(uint8_t cmd,
                                                   uint8_t* data,
                                                   std::size_t len) const;
    void log_manual_panel_diagnostics() const;
    void prepare_surface_background(const shell::ShellPresentationFrame& frame) const;
    [[nodiscard]] std::string frame_signature(const shell::ShellPresentationFrame& frame) const;
    void ensure_scratch_capacity(std::size_t pixels) const;
    void render_frame_to_scratch(const shell::ShellPresentationFrame& frame,
                                 int band_origin_y,
                                 int band_height) const;
    void present_frame_range(const shell::ShellPresentationFrame& frame,
                             int y0,
                             int y1) const;
    [[nodiscard]] int first_body_line_y(const shell::ShellPresentationFrame& frame,
                                        const shell::ShellPresentationFrame& previous) const;
    [[nodiscard]] int last_body_line_bottom(const shell::ShellPresentationFrame& frame,
                                            const shell::ShellPresentationFrame& previous) const;
    void scratch_fill_rect(int x, int y, int width, int height, uint16_t rgb565) const;
    void scratch_draw_text_block_scaled(int x,
                                        int y,
                                        std::string_view text,
                                        uint16_t fg,
                                        uint16_t bg,
                                        int scale) const;
    void scratch_draw_glyph_scaled(int x,
                                   int y,
                                   char ch,
                                   uint16_t fg,
                                   uint16_t bg,
                                   int scale) const;
    void scratch_write_pixel(int x, int y, uint16_t rgb565) const;
    void blit_scratch() const;
    void write_lvgl_region(int x,
                           int y,
                           int width,
                           int height,
                           const uint16_t* pixels,
                           std::size_t count) const;

    platform::Logger& logger_;
    ZephyrBoardRuntime& runtime_;
    ZephyrBoardBackendConfig config_;
    std::unique_ptr<IZephyrShellDisplayBackend> display_backend_;
    std::unique_ptr<ZephyrLvglShellUi> lvgl_ui_;
    BackendKind backend_ {BackendKind::None};
    const struct device* display_device_ {nullptr};
    const struct device* raw_mipi_device_ {nullptr};
    const struct device* manual_spi_device_ {nullptr};
    const struct device* manual_gpio_device_ {nullptr};
    spi_config manual_spi_config_ {};
    std::deque<std::string> boot_log_lines_;
    mutable bool last_surface_valid_ {false};
    mutable shell::ShellSurface last_surface_ {shell::ShellSurface::Home};
    mutable std::string last_frame_signature_;
    mutable bool last_frame_available_ {false};
    mutable shell::ShellPresentationFrame last_frame_;
    mutable std::vector<uint16_t> scratch_pixels_;
    mutable int scratch_band_origin_y_ {0};
    mutable int scratch_band_height_ {0};
    mutable bool suppress_boot_log_capture_ {false};
    mutable k_mutex boot_log_mutex_ {};
};

}  // namespace aegis::ports::zephyr
