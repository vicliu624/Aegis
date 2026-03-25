#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include <lvgl.h>

#include "platform/logging/logger.hpp"
#include "ports/zephyr/zephyr_board_backend_config.hpp"
#include "shell/presentation/shell_presentation_sink.hpp"

namespace aegis::ports::zephyr {

class ZephyrLvglShellUi {
public:
    using FlushCallback =
        std::function<void(int x, int y, int width, int height, const uint16_t* pixels, std::size_t count)>;

    ZephyrLvglShellUi(platform::Logger& logger,
                      ZephyrBoardBackendConfig config,
                      FlushCallback flush_callback);

    [[nodiscard]] bool initialize();
    void tick(uint32_t elapsed_ms);
    void show_startup_splash(std::string_view stage);
    void present(const shell::ShellPresentationFrame& frame);

private:
    struct LauncherEntryView {
        std::string label;
        std::string secondary;
        bool focused {false};
        bool blocked {false};
        bool warning {false};
    };

    static void display_flush_bridge(lv_display_t* display, const lv_area_t* area, uint8_t* px_map);

    void flush_display(const lv_area_t* area, uint8_t* px_map);
    void ensure_theme();
    void build_shell_chrome();
    void clear_content();
    void update_top_bar(std::string_view route, std::string_view detail);
    void update_softkeys(std::string_view left, std::string_view center, std::string_view right);
    void present_home(const shell::ShellPresentationFrame& frame);
    void present_launcher(const shell::ShellPresentationFrame& frame);
    void present_generic_surface(const shell::ShellPresentationFrame& frame);
    [[nodiscard]] std::vector<LauncherEntryView> parse_launcher_entries(
        const shell::ShellPresentationFrame& frame) const;
    [[nodiscard]] static std::string trim(std::string_view value);
    [[nodiscard]] static std::string to_route_name(shell::ShellSurface surface);
    [[nodiscard]] lv_color_t hex(uint32_t rgb) const;
    [[nodiscard]] lv_obj_t* create_panel(lv_obj_t* parent) const;
    void pump_once();
    void request_render();

    platform::Logger& logger_;
    ZephyrBoardBackendConfig config_;
    FlushCallback flush_callback_;
    lv_display_t* display_ {nullptr};
    lv_obj_t* splash_screen_ {nullptr};
    lv_obj_t* screen_ {nullptr};
    lv_obj_t* status_bar_ {nullptr};
    lv_obj_t* title_label_ {nullptr};
    lv_obj_t* detail_label_ {nullptr};
    lv_obj_t* content_ {nullptr};
    lv_obj_t* softkey_bar_ {nullptr};
    lv_obj_t* softkey_left_ {nullptr};
    lv_obj_t* softkey_center_ {nullptr};
    lv_obj_t* softkey_right_ {nullptr};
    lv_obj_t* splash_wrap_ {nullptr};
    lv_obj_t* splash_logo_ {nullptr};
    lv_obj_t* splash_subtitle_label_ {nullptr};
    lv_obj_t* splash_stage_label_ {nullptr};
    bool theme_ready_ {false};
    std::vector<uint16_t> draw_buffer_a_;
    std::vector<uint16_t> draw_buffer_b_;
    lv_style_t style_screen_;
    lv_style_t style_status_bar_;
    lv_style_t style_softkey_bar_;
    lv_style_t style_panel_;
    lv_style_t style_tile_;
    lv_style_t style_tile_focus_;
    lv_style_t style_tile_warning_;
    lv_style_t style_title_;
    lv_style_t style_body_;
    lv_style_t style_hint_;
    lv_style_t style_focus_text_;
    bool render_pending_ {false};
    uint32_t flush_count_ {0};
};

}  // namespace aegis::ports::zephyr
