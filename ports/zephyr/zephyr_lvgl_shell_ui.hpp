#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <utility>
#include <string>
#include <string_view>
#include <vector>

#include <lvgl.h>

#include "platform/logging/logger.hpp"
#include "ports/zephyr/zephyr_board_backend_config.hpp"
#include "shell/control/shell_input_model.hpp"
#include "shell/presentation/shell_presentation_sink.hpp"

namespace aegis::ports::zephyr {

class ZephyrLvglShellUi {
public:
    using FlushCallback =
        std::function<void(int x, int y, int width, int height, const uint16_t* pixels, std::size_t count)>;
    using ActionCallback = std::function<void(const shell::ShellInputInvocation& invocation)>;
    using TouchReadCallback = std::function<bool(int16_t& x, int16_t& y, bool& pressed)>;
    using StatusDetailCallback = std::function<std::string()>;
    using ClockTextCallback = std::function<std::string()>;
    struct StatusIcons {
        bool ble {false};
        bool gps {false};
    };
    using StatusIconsCallback = std::function<StatusIcons()>;

    ZephyrLvglShellUi(platform::Logger& logger,
                      ZephyrBoardBackendConfig config,
                      FlushCallback flush_callback,
                      ActionCallback action_callback,
                      TouchReadCallback touch_read_callback,
                      StatusDetailCallback status_detail_callback,
                      ClockTextCallback clock_text_callback,
                      StatusIconsCallback status_icons_callback);

    [[nodiscard]] bool initialize();
    void tick(uint32_t elapsed_ms);
    void show_startup_splash(std::string_view stage);
    void present(const shell::ShellPresentationFrame& frame);

private:
    struct TouchTarget {
        lv_obj_t* object {nullptr};
        shell::ShellInputInvocation invocation {};
        bool is_softkey {false};
        bool supports_pressed_feedback {false};
    };

    struct LauncherEntryView {
        std::string label;
        std::string secondary;
        bool focused {false};
        bool blocked {false};
        bool warning {false};
    };

    struct SettingsEntryView {
        std::string label;
        std::string value;
        bool focused {false};
        bool dirty {false};
    };

    struct AppPackageIconCacheEntry {
        struct LvMemDeleter {
            void operator()(std::uint8_t* ptr) const {
                if (ptr != nullptr) {
                    lv_free(ptr);
                }
            }
        };

        std::string path;
        std::unique_ptr<std::uint8_t, LvMemDeleter> payload;
        std::size_t payload_size {0};
        lv_image_dsc_t image {};
        bool attempted {false};
        bool valid {false};
    };

    enum class SoftkeySlot {
        Left,
        Center,
        Right,
    };

    static void display_flush_bridge(lv_display_t* display, const lv_area_t* area, uint8_t* px_map);
    static void softkey_event_bridge(lv_event_t* event);

    void flush_display(const lv_area_t* area, uint8_t* px_map);
    void sample_touch_state();
    void ensure_theme();
    void build_shell_chrome();
    void clear_content();
    void update_top_bar(std::string_view route, std::string_view detail);
    void update_softkeys(const std::array<shell::ShellSoftkeySpec, 3>& softkeys);
    void present_home(const shell::ShellPresentationFrame& frame);
    void present_launcher(const shell::ShellPresentationFrame& frame);
    void present_structured_surface(const shell::ShellPresentationFrame& frame);
    void present_generic_surface(const shell::ShellPresentationFrame& frame);
    [[nodiscard]] shell::ShellInputInvocation invocation_for_softkey(SoftkeySlot slot) const;
    [[nodiscard]] static std::string trim(std::string_view value);
    [[nodiscard]] static std::string to_route_name(shell::ShellSurface surface);
    [[nodiscard]] lv_color_t hex(uint32_t rgb) const;
    [[nodiscard]] lv_obj_t* create_panel(lv_obj_t* parent) const;
    [[nodiscard]] const lv_image_dsc_t* find_app_icon_image(std::string_view icon_path) const;
    [[nodiscard]] const lv_image_dsc_t* find_item_icon_image(
        const shell::ShellPresentationItem& item) const;
    [[nodiscard]] const char* find_symbol_icon(std::string_view key) const;
    [[nodiscard]] AppPackageIconCacheEntry* find_or_create_icon_cache_entry(std::string_view path) const;
    [[nodiscard]] bool load_app_package_icon(AppPackageIconCacheEntry& entry) const;
    void render_system_menu(const shell::ShellPresentationFrame& frame);
    void create_launcher_tile(lv_obj_t* parent,
                              const shell::ShellPresentationFrame& frame,
                              const shell::ShellPresentationItem& item);
    void update_status_icons();
    void update_status_clock();
    void update_touch_feedback();
    void poll_touch_actions();
    void register_touch_target(lv_obj_t* object,
                               shell::ShellInputInvocation invocation,
                               bool is_softkey = false,
                               bool supports_pressed_feedback = false);
    [[nodiscard]] TouchTarget* find_touch_target(int16_t x, int16_t y);
    void set_touch_target_pressed(TouchTarget* target, bool pressed);
    void set_softkey_pressed(SoftkeySlot slot, bool pressed);
    void clear_softkey_pressed_state();
    void dispatch_invocation(const shell::ShellInputInvocation& invocation);
    void pump_once();
    void request_render();

    platform::Logger& logger_;
    ZephyrBoardBackendConfig config_;
    FlushCallback flush_callback_;
    ActionCallback action_callback_;
    TouchReadCallback touch_read_callback_;
    StatusDetailCallback status_detail_callback_;
    ClockTextCallback clock_text_callback_;
    StatusIconsCallback status_icons_callback_;
    lv_display_t* display_ {nullptr};
    lv_indev_t* touch_indev_ {nullptr};
    lv_obj_t* splash_screen_ {nullptr};
    lv_obj_t* screen_ {nullptr};
    lv_obj_t* status_bar_ {nullptr};
    lv_obj_t* title_label_ {nullptr};
    lv_obj_t* status_icons_wrap_ {nullptr};
    lv_obj_t* ble_icon_ {nullptr};
    lv_obj_t* gps_icon_ {nullptr};
    lv_obj_t* clock_label_ {nullptr};
    lv_obj_t* detail_label_ {nullptr};
    lv_obj_t* content_ {nullptr};
    lv_obj_t* softkey_bar_ {nullptr};
    lv_obj_t* touch_feedback_overlay_ {nullptr};
    lv_obj_t* softkey_left_slot_ {nullptr};
    lv_obj_t* softkey_center_slot_ {nullptr};
    lv_obj_t* softkey_right_slot_ {nullptr};
    lv_obj_t* softkey_left_ {nullptr};
    lv_obj_t* softkey_center_ {nullptr};
    lv_obj_t* softkey_right_ {nullptr};
    lv_obj_t* page_dialog_popup_ {nullptr};
    lv_obj_t* page_dialog_popup_title_ {nullptr};
    lv_obj_t* page_dialog_popup_body_ {nullptr};
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
    lv_style_t style_icon_label_;
    lv_style_t style_softkey_pressed_;
    lv_style_t style_tile_pressed_;
    SoftkeySlot softkey_left_id_ {SoftkeySlot::Left};
    SoftkeySlot softkey_center_id_ {SoftkeySlot::Center};
    SoftkeySlot softkey_right_id_ {SoftkeySlot::Right};
    std::vector<TouchTarget> touch_targets_;
    shell::ShellPresentationFrame active_frame_ {};
    shell::ShellSurface active_surface_ {shell::ShellSurface::Home};
    TouchTarget* touch_pressed_target_ {nullptr};
    bool softkey_pressed_active_ {false};
    bool touch_tracking_active_ {false};
    bool touch_pressed_last_ {false};
    int16_t cached_touch_x_ {0};
    int16_t cached_touch_y_ {0};
    bool cached_touch_pressed_ {false};
    bool cached_touch_valid_ {false};
    StatusIcons last_status_icons_ {};
    bool last_status_icons_valid_ {false};
    uint32_t last_clock_update_ms_ {0};
    std::string last_clock_text_;
    uint32_t last_touch_poll_ms_ {0};
    TouchTarget* touch_feedback_target_ {nullptr};
    uint32_t touch_feedback_deadline_ms_ {0};
    bool render_pending_ {false};
    uint32_t flush_count_ {0};
    mutable std::deque<AppPackageIconCacheEntry> app_package_icon_cache_;
};

}  // namespace aegis::ports::zephyr
