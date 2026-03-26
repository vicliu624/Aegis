#include "ports/zephyr/zephyr_lvgl_shell_ui.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <ctime>
#include <string_view>
#include <utility>

#include "ports/zephyr/zephyr_lvgl_assets.hpp"

namespace aegis::ports::zephyr {

namespace {

constexpr uint32_t kColorBgRoot = 0xD9DDE2;
constexpr uint32_t kColorBgPanel = 0xFFFFFF;
constexpr uint32_t kColorBgStatus = 0x184D84;
constexpr uint32_t kColorBgStatusAccent = 0x2D6CA5;
constexpr uint32_t kColorBgSoftkey = 0x405261;
constexpr uint32_t kColorBgSoftkeyAccent = 0x2F3E4B;
constexpr uint32_t kColorTextPrimary = 0x1F2A33;
constexpr uint32_t kColorTextSecondary = 0x4D5A66;
constexpr uint32_t kColorTextMuted = 0x6A7681;
constexpr uint32_t kColorTextInverse = 0xFFFFFF;
constexpr uint32_t kColorBorderStrong = 0x7E8B95;
constexpr uint32_t kColorBorderDefault = 0xB4BBC2;
constexpr uint32_t kColorFocusFill = 0xDCEBFB;
constexpr uint32_t kColorFocusBorder = 0x2567A6;
constexpr uint32_t kColorWarning = 0xD67C2C;
constexpr uint32_t kColorPolicy = 0x6C557A;
constexpr uint32_t kColorReady = 0x3B7D45;
constexpr uint32_t kColorPanelDivider = 0xCDD3D9;

constexpr int kStatusBarHeight = 26;
constexpr int kSoftkeyBarHeight = 26;
constexpr int kContentInset = 0;
constexpr int kTopbarIconSize = 24;
constexpr int kTopbarIconGap = 4;
constexpr int kTopbarIconLeftInset = 4;
constexpr int kTopbarIconSafeWidth =
    (kTopbarIconSize * 2) + kTopbarIconGap + (kTopbarIconLeftInset * 2);
constexpr int kGridColumns = 3;
constexpr int kTileWidth = 86;
constexpr int kTileHeight = 76;
constexpr int kIconWrapSize = 44;
constexpr int kFileRowIconPaddingRight = 2;
constexpr uint32_t kClockRefreshMs = 500;
constexpr uint32_t kTouchFeedbackHoldMs = 90;
constexpr uint32_t kTileActionDelayMs = 70;
constexpr uint32_t kSplashDelayMs = 0;

std::string_view surface_name(shell::ShellSurface surface) {
    switch (surface) {
        case shell::ShellSurface::Home: return "home";
        case shell::ShellSurface::Launcher: return "launcher";
        case shell::ShellSurface::Files: return "files";
        case shell::ShellSurface::Settings: return "settings";
        case shell::ShellSurface::Notifications: return "notifications";
        case shell::ShellSurface::AppForeground: return "app_foreground";
        case shell::ShellSurface::Recovery: return "recovery";
    }
    return "unknown";
}
}  // namespace

ZephyrLvglShellUi::ZephyrLvglShellUi(platform::Logger& logger,
                                     ZephyrBoardBackendConfig config,
                                     FlushCallback flush_callback,
                                     ActionCallback action_callback,
                                     TouchReadCallback touch_read_callback,
                                     StatusDetailCallback status_detail_callback,
                                     ClockTextCallback clock_text_callback,
                                     StatusIconsCallback status_icons_callback)
    : logger_(logger),
      config_(std::move(config)),
      flush_callback_(std::move(flush_callback)),
      action_callback_(std::move(action_callback)),
      touch_read_callback_(std::move(touch_read_callback)),
      status_detail_callback_(std::move(status_detail_callback)),
      clock_text_callback_(std::move(clock_text_callback)),
      status_icons_callback_(std::move(status_icons_callback)) {}

bool ZephyrLvglShellUi::initialize() {
    logger_.info("lvgl", "ui init begin");
    lv_init();
    logger_.info("lvgl", "ui init lv_init done");

    const std::size_t width = static_cast<std::size_t>(std::max(1, config_.display_width));
    const std::size_t band_height =
        std::min<std::size_t>(24, static_cast<std::size_t>(std::max(12, config_.display_height / 6)));
    const std::size_t pixel_count = width * band_height;
    draw_buffer_a_.assign(pixel_count, 0U);
    draw_buffer_b_.clear();
    logger_.info("lvgl",
                 "ui init draw buffer pixels=" + std::to_string(pixel_count) +
                     " bytes=" + std::to_string(pixel_count * sizeof(uint16_t)) +
                     " mode=single");

    display_ = lv_display_create(config_.display_width, config_.display_height);
    if (display_ == nullptr) {
        logger_.info("lvgl", "display create failed");
        return false;
    }
    logger_.info("lvgl", "ui init display created");

    lv_display_set_user_data(display_, this);
    lv_display_set_color_format(display_, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display_,
                           draw_buffer_a_.data(),
                           nullptr,
                           static_cast<uint32_t>(pixel_count * sizeof(uint16_t)),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display_, display_flush_bridge);
    lv_display_set_default(display_);
    logger_.info("lvgl", "ui init display callbacks set");

    ensure_theme();
    logger_.info("lvgl", "ui init theme ready");
    build_shell_chrome();
    logger_.info("lvgl", "ui init chrome ready");
    show_startup_splash("Starting system");
    logger_.info("lvgl", "ui init splash shown");
    return true;
}

void ZephyrLvglShellUi::tick(uint32_t elapsed_ms) {
    lv_tick_inc(elapsed_ms == 0 ? 1U : elapsed_ms);
    sample_touch_state();
    update_status_icons();
    update_status_clock();
    update_touch_feedback();
    update_pending_action();
    poll_touch_actions();
    pump_once();
}

void ZephyrLvglShellUi::show_startup_splash(std::string_view stage) {
    if (display_ == nullptr) {
        return;
    }

    if (splash_screen_ == nullptr) {
        splash_screen_ = lv_obj_create(nullptr);
        lv_obj_remove_style_all(splash_screen_);
        lv_obj_add_style(splash_screen_, &style_screen_, 0);
        lv_obj_set_size(splash_screen_, config_.display_width, config_.display_height);
        lv_obj_set_style_pad_all(splash_screen_, 0, 0);
    }

    if (splash_wrap_ == nullptr) {
        splash_wrap_ = lv_obj_create(splash_screen_);
        lv_obj_remove_style_all(splash_wrap_);
        lv_obj_set_size(splash_wrap_, lv_pct(100), lv_pct(100));
        lv_obj_set_style_bg_color(splash_wrap_, hex(kColorBgRoot), 0);
        lv_obj_set_style_bg_opa(splash_wrap_, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(splash_wrap_, 0, 0);
        lv_obj_set_style_pad_all(splash_wrap_, 0, 0);
        lv_obj_set_flex_flow(splash_wrap_, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(
            splash_wrap_, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_center(splash_wrap_);
        lv_obj_set_style_pad_row(splash_wrap_, 10, 0);

        splash_logo_ = lv_image_create(splash_wrap_);
        lv_image_set_src(splash_logo_, &g_aegis_logo_image);

        splash_subtitle_label_ = lv_label_create(splash_wrap_);
        lv_obj_add_style(splash_subtitle_label_, &style_body_, 0);
        lv_label_set_text(splash_subtitle_label_, "Governed device workspace");

        splash_stage_label_ = lv_label_create(splash_wrap_);
        lv_obj_add_style(splash_stage_label_, &style_hint_, 0);
    }

    if (lv_screen_active() != splash_screen_) {
        lv_screen_load(splash_screen_);
    }

    if (splash_stage_label_ != nullptr) {
        lv_label_set_text(splash_stage_label_, trim(stage).c_str());
    }

    request_render();
    if constexpr (kSplashDelayMs > 0) {
        lv_tick_inc(kSplashDelayMs);
    }
    pump_once();
}

void ZephyrLvglShellUi::present(const shell::ShellPresentationFrame& frame) {
    if (screen_ == nullptr) {
        return;
    }
    logger_.info("lvgl",
                 "present surface=" + std::string(surface_name(frame.surface)) +
                     " lines=" + std::to_string(frame.lines.size()));
    active_surface_ = frame.surface;

    if (lv_screen_active() != screen_) {
        lv_screen_load(screen_);
    }
    if (splash_screen_ != nullptr) {
        lv_obj_delete(splash_screen_);
        splash_screen_ = nullptr;
        splash_wrap_ = nullptr;
        splash_logo_ = nullptr;
        splash_subtitle_label_ = nullptr;
        splash_stage_label_ = nullptr;
    }
    if (touch_feedback_overlay_ != nullptr) {
        lv_obj_add_flag(touch_feedback_overlay_, LV_OBJ_FLAG_HIDDEN);
    }
    touch_feedback_target_ = nullptr;
    touch_feedback_deadline_ms_ = 0;

    switch (frame.surface) {
        case shell::ShellSurface::Home:
            present_home(frame);
            break;
        case shell::ShellSurface::Launcher:
            present_launcher(frame);
            break;
        case shell::ShellSurface::Files:
            present_files_surface(frame);
            break;
        case shell::ShellSurface::Settings:
            present_settings_surface(frame);
            break;
        case shell::ShellSurface::Notifications:
        case shell::ShellSurface::AppForeground:
        case shell::ShellSurface::Recovery:
            present_generic_surface(frame);
            break;
    }

    request_render();
    pump_once();
}

void ZephyrLvglShellUi::builtin_tile_event_bridge(lv_event_t* event) {
    if (event == nullptr) {
        return;
    }

    const auto code = lv_event_get_code(event);
    if (code != LV_EVENT_CLICKED && code != LV_EVENT_SHORT_CLICKED) {
        return;
    }

    auto* action = static_cast<shell::ShellNavigationAction*>(lv_event_get_user_data(event));
    auto* tile = static_cast<lv_obj_t*>(lv_event_get_target(event));
    if (action == nullptr || tile == nullptr) {
        return;
    }

    auto* screen = lv_obj_get_screen(tile);
    auto* display = screen != nullptr ? lv_obj_get_display(screen) : nullptr;
    auto* self = display != nullptr
                     ? static_cast<ZephyrLvglShellUi*>(lv_display_get_user_data(display))
                     : nullptr;
    if (self != nullptr) {
        self->dispatch_action(*action);
    }
}

void ZephyrLvglShellUi::softkey_event_bridge(lv_event_t* event) {
    if (event == nullptr) {
        return;
    }

    const auto code = lv_event_get_code(event);
    if (code != LV_EVENT_CLICKED && code != LV_EVENT_SHORT_CLICKED) {
        return;
    }

    auto* slot = static_cast<SoftkeySlot*>(lv_event_get_user_data(event));
    auto* target = static_cast<lv_obj_t*>(lv_event_get_target(event));
    if (slot == nullptr || target == nullptr) {
        return;
    }

    auto* screen = lv_obj_get_screen(target);
    auto* display = screen != nullptr ? lv_obj_get_display(screen) : nullptr;
    auto* self = display != nullptr
                     ? static_cast<ZephyrLvglShellUi*>(lv_display_get_user_data(display))
                     : nullptr;
    if (self == nullptr) {
        return;
    }

    const auto action = self->action_for_softkey(*slot);
    self->dispatch_action(action);
}

void ZephyrLvglShellUi::display_flush_bridge(lv_display_t* display,
                                             const lv_area_t* area,
                                             uint8_t* px_map) {
    auto* self = static_cast<ZephyrLvglShellUi*>(lv_display_get_user_data(display));
    if (self != nullptr) {
        self->flush_display(area, px_map);
    }
    lv_display_flush_ready(display);
}

void ZephyrLvglShellUi::flush_display(const lv_area_t* area, uint8_t* px_map) {
    if (flush_callback_ == nullptr || area == nullptr || px_map == nullptr) {
        return;
    }

    const int width = (area->x2 - area->x1) + 1;
    const int height = (area->y2 - area->y1) + 1;
    lv_draw_sw_rgb565_swap(px_map, static_cast<uint32_t>(width * height));
    const auto* pixels = reinterpret_cast<const uint16_t*>(px_map);
    ++flush_count_;
    if (flush_count_ <= 20 || (flush_count_ % 25) == 0) {
        logger_.info("lvgl",
                     "flush count=" + std::to_string(flush_count_) +
                         " area=" + std::to_string(area->x1) + "," + std::to_string(area->y1) +
                         " " + std::to_string(width) + "x" + std::to_string(height));
    }
    flush_callback_(area->x1,
                    area->y1,
                    width,
                    height,
                    pixels,
                    static_cast<std::size_t>(width * height));
}

void ZephyrLvglShellUi::sample_touch_state() {
    if (!touch_read_callback_) {
        return;
    }

    const uint32_t now_ms = lv_tick_get();
    if (!cached_touch_pressed_ && last_touch_poll_ms_ != 0 && (now_ms - last_touch_poll_ms_) < 16U) {
        return;
    }
    last_touch_poll_ms_ = now_ms;

    int16_t x = 0;
    int16_t y = 0;
    bool pressed = false;
    const bool sample_ok = touch_read_callback_(x, y, pressed);
    if (!sample_ok) {
        cached_touch_valid_ = false;
        return;
    }
    touch_pressed_last_ = pressed;

    if (pressed || !cached_touch_valid_) {
        cached_touch_x_ = x;
        cached_touch_y_ = y;
    }
    cached_touch_pressed_ = pressed;
    cached_touch_valid_ = true;
}

void ZephyrLvglShellUi::ensure_theme() {
    if (theme_ready_) {
        return;
    }

    lv_style_init(&style_screen_);
    lv_style_set_bg_color(&style_screen_, hex(kColorBgRoot));
    lv_style_set_bg_opa(&style_screen_, LV_OPA_COVER);
    lv_style_set_text_color(&style_screen_, hex(kColorTextPrimary));

    lv_style_init(&style_status_bar_);
    lv_style_set_bg_color(&style_status_bar_, hex(kColorBgStatus));
    lv_style_set_bg_grad_color(&style_status_bar_, hex(kColorBgStatusAccent));
    lv_style_set_bg_grad_dir(&style_status_bar_, LV_GRAD_DIR_HOR);
    lv_style_set_bg_opa(&style_status_bar_, LV_OPA_COVER);
    lv_style_set_border_color(&style_status_bar_, hex(kColorBorderStrong));
    lv_style_set_border_width(&style_status_bar_, 0);
    lv_style_set_pad_left(&style_status_bar_, 8);
    lv_style_set_pad_right(&style_status_bar_, 8);
    lv_style_set_pad_top(&style_status_bar_, 4);
    lv_style_set_pad_bottom(&style_status_bar_, 4);

    lv_style_init(&style_softkey_bar_);
    lv_style_set_bg_color(&style_softkey_bar_, hex(kColorBgSoftkey));
    lv_style_set_bg_grad_color(&style_softkey_bar_, hex(kColorBgSoftkeyAccent));
    lv_style_set_bg_grad_dir(&style_softkey_bar_, LV_GRAD_DIR_HOR);
    lv_style_set_bg_opa(&style_softkey_bar_, LV_OPA_COVER);
    lv_style_set_border_color(&style_softkey_bar_, hex(kColorBorderStrong));
    lv_style_set_border_width(&style_softkey_bar_, 0);
    lv_style_set_pad_left(&style_softkey_bar_, 8);
    lv_style_set_pad_right(&style_softkey_bar_, 8);
    lv_style_set_pad_top(&style_softkey_bar_, 3);
    lv_style_set_pad_bottom(&style_softkey_bar_, 3);

    lv_style_init(&style_panel_);
    lv_style_set_bg_color(&style_panel_, hex(kColorBgPanel));
    lv_style_set_bg_opa(&style_panel_, LV_OPA_COVER);
    lv_style_set_border_width(&style_panel_, 0);
    lv_style_set_radius(&style_panel_, 0);
    lv_style_set_pad_all(&style_panel_, 0);

    lv_style_init(&style_tile_);
    lv_style_set_bg_opa(&style_tile_, LV_OPA_TRANSP);
    lv_style_set_border_width(&style_tile_, 0);
    lv_style_set_shadow_width(&style_tile_, 0);
    lv_style_set_radius(&style_tile_, 0);
    lv_style_set_pad_left(&style_tile_, 2);
    lv_style_set_pad_right(&style_tile_, 2);
    lv_style_set_pad_top(&style_tile_, 2);
    lv_style_set_pad_bottom(&style_tile_, 2);

    lv_style_init(&style_tile_focus_);
    lv_style_set_bg_opa(&style_tile_focus_, LV_OPA_TRANSP);
    lv_style_set_border_width(&style_tile_focus_, 0);
    lv_style_set_shadow_width(&style_tile_focus_, 0);

    lv_style_init(&style_tile_warning_);
    lv_style_set_border_width(&style_tile_warning_, 0);

    lv_style_init(&style_title_);
    lv_style_set_text_color(&style_title_, hex(kColorTextPrimary));
    lv_style_set_text_font(&style_title_, &lv_font_montserrat_16);

    lv_style_init(&style_body_);
    lv_style_set_text_color(&style_body_, hex(kColorTextPrimary));
    lv_style_set_text_font(&style_body_, &lv_font_montserrat_14);

    lv_style_init(&style_hint_);
    lv_style_set_text_color(&style_hint_, hex(kColorTextSecondary));
    lv_style_set_text_font(&style_hint_, &lv_font_montserrat_12);

    lv_style_init(&style_focus_text_);
    lv_style_set_text_color(&style_focus_text_, hex(kColorTextInverse));
    lv_style_set_text_font(&style_focus_text_, &lv_font_montserrat_14);

    lv_style_init(&style_icon_label_);
    lv_style_set_text_color(&style_icon_label_, hex(kColorTextInverse));
    lv_style_set_text_font(&style_icon_label_, &lv_font_montserrat_16);

    lv_style_init(&style_softkey_pressed_);
    lv_style_set_bg_color(&style_softkey_pressed_, hex(kColorBgStatusAccent));
    lv_style_set_bg_opa(&style_softkey_pressed_, LV_OPA_70);
    lv_style_set_border_width(&style_softkey_pressed_, 0);

    lv_style_init(&style_tile_pressed_);
    lv_style_set_bg_color(&style_tile_pressed_, hex(kColorFocusBorder));
    lv_style_set_bg_opa(&style_tile_pressed_, LV_OPA_COVER);
    lv_style_set_radius(&style_tile_pressed_, 10);
    lv_style_set_border_color(&style_tile_pressed_, hex(kColorBgStatusAccent));
    lv_style_set_border_width(&style_tile_pressed_, 2);
    lv_style_set_outline_color(&style_tile_pressed_, hex(kColorTextInverse));
    lv_style_set_outline_width(&style_tile_pressed_, 1);
    lv_style_set_outline_pad(&style_tile_pressed_, 0);
    lv_style_set_shadow_color(&style_tile_pressed_, hex(kColorFocusBorder));
    lv_style_set_shadow_width(&style_tile_pressed_, 10);
    lv_style_set_shadow_opa(&style_tile_pressed_, LV_OPA_40);

    theme_ready_ = true;
}

void ZephyrLvglShellUi::build_shell_chrome() {
    screen_ = lv_obj_create(nullptr);
    lv_obj_remove_style_all(screen_);
    lv_obj_add_style(screen_, &style_screen_, 0);
    lv_obj_set_size(screen_, config_.display_width, config_.display_height);
    lv_obj_set_style_pad_all(screen_, 0, 0);
    lv_obj_clear_flag(screen_, LV_OBJ_FLAG_SCROLLABLE);

    status_bar_ = lv_obj_create(screen_);
    lv_obj_remove_style_all(status_bar_);
    lv_obj_add_style(status_bar_, &style_status_bar_, 0);
    lv_obj_set_size(status_bar_, lv_pct(100), kStatusBarHeight);
    lv_obj_align(status_bar_, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_clear_flag(status_bar_, LV_OBJ_FLAG_SCROLLABLE);

    title_label_ = lv_label_create(status_bar_);
    lv_obj_add_style(title_label_, &style_hint_, 0);
    lv_label_set_text(title_label_, "");
    lv_obj_align(title_label_, LV_ALIGN_LEFT_MID, 6, 0);

    status_icons_wrap_ = lv_obj_create(status_bar_);
    lv_obj_remove_style_all(status_icons_wrap_);
    lv_obj_set_size(status_icons_wrap_, kTopbarIconSafeWidth, kTopbarIconSize);
    lv_obj_set_style_bg_opa(status_icons_wrap_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(status_icons_wrap_, 0, 0);
    lv_obj_set_style_pad_all(status_icons_wrap_, 0, 0);
    lv_obj_align(status_icons_wrap_, LV_ALIGN_LEFT_MID, kTopbarIconLeftInset, 0);
    lv_obj_clear_flag(status_icons_wrap_, LV_OBJ_FLAG_SCROLLABLE);
    gps_icon_ = lv_image_create(status_icons_wrap_);
    lv_image_set_src(gps_icon_, &g_gps_topbar_image);
    lv_obj_set_pos(gps_icon_, 0, 0);
    lv_obj_add_flag(gps_icon_, LV_OBJ_FLAG_HIDDEN);

    ble_icon_ = lv_image_create(status_icons_wrap_);
    lv_image_set_src(ble_icon_, &g_ble_topbar_image);
    lv_obj_set_pos(ble_icon_, kTopbarIconSize + kTopbarIconGap, 0);
    lv_obj_add_flag(ble_icon_, LV_OBJ_FLAG_HIDDEN);

    clock_label_ = lv_label_create(status_bar_);
    lv_obj_add_style(clock_label_, &style_title_, 0);
    lv_obj_set_style_text_color(clock_label_, hex(kColorTextInverse), 0);
    lv_label_set_text(clock_label_, "--:--");
    lv_obj_align(clock_label_, LV_ALIGN_CENTER, 0, 0);

    detail_label_ = lv_label_create(status_bar_);
    lv_obj_add_style(detail_label_, &style_hint_, 0);
    lv_obj_set_style_text_color(detail_label_, hex(kColorTextInverse), 0);
    lv_label_set_text(detail_label_, "");
    lv_obj_align(detail_label_, LV_ALIGN_RIGHT_MID, -6, 0);

    content_ = lv_obj_create(screen_);
    lv_obj_remove_style_all(content_);
    lv_obj_set_style_bg_color(content_, hex(kColorBgRoot), 0);
    lv_obj_set_style_bg_opa(content_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(content_, 0, 0);
    lv_obj_set_style_pad_all(content_, kContentInset, 0);
    lv_obj_set_size(content_,
                    lv_pct(100),
                    config_.display_height - kStatusBarHeight - kSoftkeyBarHeight);
    lv_obj_align(content_, LV_ALIGN_TOP_LEFT, 0, kStatusBarHeight);
    lv_obj_clear_flag(content_, LV_OBJ_FLAG_SCROLLABLE);

    softkey_bar_ = lv_obj_create(screen_);
    lv_obj_remove_style_all(softkey_bar_);
    lv_obj_add_style(softkey_bar_, &style_softkey_bar_, 0);
    lv_obj_set_size(softkey_bar_, lv_pct(100), kSoftkeyBarHeight);
    lv_obj_align(softkey_bar_, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_clear_flag(softkey_bar_, LV_OBJ_FLAG_SCROLLABLE);

    touch_feedback_overlay_ = lv_obj_create(screen_);
    lv_obj_remove_style_all(touch_feedback_overlay_);
    lv_obj_set_style_bg_color(touch_feedback_overlay_, hex(kColorFocusFill), 0);
    lv_obj_set_style_bg_opa(touch_feedback_overlay_, LV_OPA_30, 0);
    lv_obj_set_style_border_color(touch_feedback_overlay_, hex(kColorFocusBorder), 0);
    lv_obj_set_style_border_width(touch_feedback_overlay_, 1, 0);
    lv_obj_set_style_radius(touch_feedback_overlay_, 8, 0);
    lv_obj_set_style_shadow_width(touch_feedback_overlay_, 0, 0);
    lv_obj_set_style_outline_width(touch_feedback_overlay_, 0, 0);
    lv_obj_add_flag(touch_feedback_overlay_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(touch_feedback_overlay_, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_clear_flag(touch_feedback_overlay_, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_move_foreground(touch_feedback_overlay_);

    files_info_popup_ = lv_obj_create(screen_);
    lv_obj_remove_style_all(files_info_popup_);
    lv_obj_set_size(files_info_popup_, 224, 92);
    lv_obj_center(files_info_popup_);
    lv_obj_set_style_bg_color(files_info_popup_, hex(kColorBgPanel), 0);
    lv_obj_set_style_bg_opa(files_info_popup_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(files_info_popup_, hex(kColorFocusBorder), 0);
    lv_obj_set_style_border_width(files_info_popup_, 2, 0);
    lv_obj_set_style_radius(files_info_popup_, 6, 0);
    lv_obj_set_style_pad_left(files_info_popup_, 10, 0);
    lv_obj_set_style_pad_right(files_info_popup_, 10, 0);
    lv_obj_set_style_pad_top(files_info_popup_, 8, 0);
    lv_obj_set_style_pad_bottom(files_info_popup_, 8, 0);
    lv_obj_add_flag(files_info_popup_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(files_info_popup_, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_move_foreground(files_info_popup_);

    files_info_popup_title_ = lv_label_create(files_info_popup_);
    lv_obj_add_style(files_info_popup_title_, &style_title_, 0);
    lv_obj_align(files_info_popup_title_, LV_ALIGN_TOP_LEFT, 0, 0);

    files_info_popup_body_ = lv_label_create(files_info_popup_);
    lv_obj_add_style(files_info_popup_body_, &style_body_, 0);
    lv_label_set_long_mode(files_info_popup_body_, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(files_info_popup_body_, 200);
    lv_obj_align(files_info_popup_body_, LV_ALIGN_TOP_LEFT, 0, 26);

    auto make_softkey_slot = [&](SoftkeySlot slot, lv_align_t align) -> lv_obj_t* {
        (void)slot;
        auto* node = lv_obj_create(softkey_bar_);
        lv_obj_remove_style_all(node);
        lv_obj_set_size(node, config_.display_width / 3, kSoftkeyBarHeight);
        lv_obj_align(node, align, 0, 0);
        lv_obj_set_style_bg_opa(node, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(node, 0, 0);
        lv_obj_add_flag(node, LV_OBJ_FLAG_CLICKABLE);
        return node;
    };

    softkey_left_slot_ = make_softkey_slot(SoftkeySlot::Left, LV_ALIGN_LEFT_MID);
    softkey_center_slot_ = make_softkey_slot(SoftkeySlot::Center, LV_ALIGN_CENTER);
    softkey_right_slot_ = make_softkey_slot(SoftkeySlot::Right, LV_ALIGN_RIGHT_MID);
    lv_obj_add_event_cb(softkey_left_slot_, softkey_event_bridge, LV_EVENT_CLICKED, &softkey_left_id_);
    lv_obj_add_event_cb(softkey_left_slot_, softkey_event_bridge, LV_EVENT_SHORT_CLICKED, &softkey_left_id_);
    lv_obj_add_event_cb(softkey_center_slot_, softkey_event_bridge, LV_EVENT_CLICKED, &softkey_center_id_);
    lv_obj_add_event_cb(softkey_center_slot_, softkey_event_bridge, LV_EVENT_SHORT_CLICKED, &softkey_center_id_);
    lv_obj_add_event_cb(softkey_right_slot_, softkey_event_bridge, LV_EVENT_CLICKED, &softkey_right_id_);
    lv_obj_add_event_cb(softkey_right_slot_, softkey_event_bridge, LV_EVENT_SHORT_CLICKED, &softkey_right_id_);

    softkey_left_ = lv_label_create(softkey_left_slot_);
    lv_obj_add_style(softkey_left_, &style_hint_, 0);
    lv_obj_add_flag(softkey_left_, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_center(softkey_left_);
    softkey_center_ = lv_label_create(softkey_center_slot_);
    lv_obj_add_style(softkey_center_, &style_hint_, 0);
    lv_obj_add_flag(softkey_center_, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_center(softkey_center_);
    softkey_right_ = lv_label_create(softkey_right_slot_);
    lv_obj_add_style(softkey_right_, &style_hint_, 0);
    lv_obj_add_flag(softkey_right_, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_center(softkey_right_);

    register_touch_target(softkey_left_slot_, action_for_softkey(SoftkeySlot::Left), true, true);
    register_touch_target(softkey_center_slot_, action_for_softkey(SoftkeySlot::Center), true, true);
    register_touch_target(softkey_right_slot_, action_for_softkey(SoftkeySlot::Right), true, true);
}

void ZephyrLvglShellUi::clear_content() {
    if (content_ != nullptr) {
        tile_actions_.clear();
        touch_targets_.clear();
        register_touch_target(softkey_left_slot_, action_for_softkey(SoftkeySlot::Left), true, true);
        register_touch_target(softkey_center_slot_, action_for_softkey(SoftkeySlot::Center), true, true);
        register_touch_target(softkey_right_slot_, action_for_softkey(SoftkeySlot::Right), true, true);
        touch_pressed_target_ = nullptr;
        touch_tracking_active_ = false;
        clear_softkey_pressed_state();
        touch_feedback_target_ = nullptr;
        touch_feedback_deadline_ms_ = 0;
        pending_action_active_ = false;
        pending_action_deadline_ms_ = 0;
        files_info_popup_visible_ = false;
        current_file_entries_.clear();
        if (touch_feedback_overlay_ != nullptr) {
            lv_obj_add_flag(touch_feedback_overlay_, LV_OBJ_FLAG_HIDDEN);
        }
        if (files_info_popup_ != nullptr) {
            lv_obj_add_flag(files_info_popup_, LV_OBJ_FLAG_HIDDEN);
        }
        lv_obj_clean(content_);
    }
}

void ZephyrLvglShellUi::update_top_bar(std::string_view route, std::string_view detail) {
    (void)route;
    (void)detail;
    if (title_label_ == nullptr || detail_label_ == nullptr || clock_label_ == nullptr) {
        return;
    }
    lv_label_set_text(title_label_, "");
    if (status_detail_callback_) {
        const auto detail_text = status_detail_callback_();
        lv_label_set_text(detail_label_, detail_text.c_str());
    } else {
        lv_label_set_text(detail_label_, "");
    }
    update_status_icons();
    update_status_clock();
}

void ZephyrLvglShellUi::update_status_icons() {
    if (status_icons_wrap_ == nullptr) {
        return;
    }

    StatusIcons icons {};
    if (status_icons_callback_) {
        icons = status_icons_callback_();
    }

    const bool changed = !last_status_icons_valid_ || last_status_icons_.ble != icons.ble ||
                         last_status_icons_.gps != icons.gps;
    last_status_icons_ = icons;
    last_status_icons_valid_ = true;

    if (ble_icon_ != nullptr) {
        if (icons.ble) {
            lv_obj_clear_flag(ble_icon_, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(ble_icon_, LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (gps_icon_ != nullptr) {
        if (icons.gps) {
            lv_obj_clear_flag(gps_icon_, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(gps_icon_, LV_OBJ_FLAG_HIDDEN);
        }
    }

    const int visible_count = (icons.gps ? 1 : 0) + (icons.ble ? 1 : 0);
    if (visible_count == 0) {
        lv_obj_add_flag(status_icons_wrap_, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_clear_flag(status_icons_wrap_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_align(status_icons_wrap_, LV_ALIGN_LEFT_MID, kTopbarIconLeftInset, 0);
        if (gps_icon_ != nullptr) {
            lv_obj_set_pos(gps_icon_, 0, 0);
        }
        if (ble_icon_ != nullptr) {
            const int ble_x = icons.gps ? (kTopbarIconSize + kTopbarIconGap) : 0;
            lv_obj_set_pos(ble_icon_, ble_x, 0);
        }
    }

    if (changed) {
        request_render();
    }
}

void ZephyrLvglShellUi::update_softkeys(std::string_view left,
                                        std::string_view center,
                                        std::string_view right) {
    lv_label_set_text(softkey_left_, trim(left).c_str());
    lv_label_set_text(softkey_center_, trim(center).c_str());
    lv_label_set_text(softkey_right_, trim(right).c_str());
    lv_obj_set_style_text_color(softkey_left_, hex(kColorTextInverse), 0);
    lv_obj_set_style_text_color(softkey_center_, hex(kColorTextInverse), 0);
    lv_obj_set_style_text_color(softkey_right_, hex(kColorTextInverse), 0);
    lv_obj_set_style_bg_opa(softkey_center_slot_, LV_OPA_TRANSP, 0);
}

void ZephyrLvglShellUi::update_files_softkey_state(bool info_enabled) {
    if (softkey_center_ == nullptr || softkey_center_slot_ == nullptr) {
        return;
    }
    if (info_enabled) {
        lv_obj_set_style_text_color(softkey_center_, hex(kColorTextInverse), 0);
        lv_obj_set_style_bg_opa(softkey_center_slot_, LV_OPA_TRANSP, 0);
    } else {
        lv_obj_set_style_text_color(softkey_center_, hex(kColorTextMuted), 0);
        lv_obj_set_style_bg_color(softkey_center_slot_, hex(kColorBgSoftkeyAccent), 0);
        lv_obj_set_style_bg_opa(softkey_center_slot_, LV_OPA_40, 0);
    }
    request_render();
}

void ZephyrLvglShellUi::present_home(const shell::ShellPresentationFrame& frame) {
    update_top_bar("", "");
    const auto softkeys = softkeys_for_surface(shell::ShellSurface::Home);
    update_softkeys(softkeys[0], softkeys[1], softkeys[2]);
    render_system_menu(frame);
}

void ZephyrLvglShellUi::present_launcher(const shell::ShellPresentationFrame& frame) {
    update_top_bar("", "");
    const auto softkeys = softkeys_for_surface(shell::ShellSurface::Launcher);
    update_softkeys(softkeys[0], softkeys[1], softkeys[2]);
    render_system_menu(frame);
}

void ZephyrLvglShellUi::present_files_surface(const shell::ShellPresentationFrame& frame) {
    update_top_bar("", "");
    const auto softkeys = softkeys_for_surface(shell::ShellSurface::Files);
    update_softkeys(softkeys[0], softkeys[1], softkeys[2]);
    clear_content();

    auto* panel = create_panel(content_);
    lv_obj_set_size(panel, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(panel, hex(kColorBgPanel), 0);
    lv_obj_set_style_pad_left(panel, 10, 0);
    lv_obj_set_style_pad_right(panel, 10, 0);
    lv_obj_set_style_pad_top(panel, 8, 0);
    lv_obj_set_style_pad_bottom(panel, 4, 0);
    lv_obj_center(panel);

    auto* header = lv_obj_create(panel);
    lv_obj_remove_style_all(header);
    lv_obj_set_size(header, lv_pct(100), 24);
    lv_obj_set_style_bg_opa(header, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_pad_all(header, 0, 0);
    lv_obj_align(header, LV_ALIGN_TOP_LEFT, 0, 0);

    auto* title = lv_label_create(header);
    lv_obj_add_style(title, &style_title_, 0);
    lv_label_set_text(title, "Files");
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 0, 0);

    auto* path = lv_label_create(header);
    lv_obj_add_style(path, &style_hint_, 0);
    lv_label_set_long_mode(path, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(path, 200);
    lv_label_set_text(path, frame.detail.c_str());
    lv_obj_align(path, LV_ALIGN_RIGHT_MID, 0, 0);

    auto* list = lv_obj_create(panel);
    lv_obj_remove_style_all(list);
    lv_obj_set_size(list, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(list, 0, 0);
    lv_obj_set_style_pad_top(list, 28, 0);
    lv_obj_set_style_pad_bottom(list, 0, 0);
    lv_obj_set_style_pad_left(list, 0, 0);
    lv_obj_set_style_pad_right(list, 0, 0);
    lv_obj_set_style_pad_row(list, 0, 0);
    lv_obj_set_layout(list, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_align(list, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_OFF);

    current_file_entries_ = parse_file_entries(frame);
    update_files_softkey_state(files_info_available());
    if (current_file_entries_.empty()) {
        auto* empty = lv_label_create(list);
        lv_obj_add_style(empty, &style_hint_, 0);
        lv_label_set_text(empty, "No entries");
        return;
    }

    for (std::size_t index = 0; index < current_file_entries_.size(); ++index) {
        const auto& entry = current_file_entries_[index];

        auto* row = lv_obj_create(list);
        lv_obj_remove_style_all(row);
        lv_obj_set_size(row, lv_pct(100), 24);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_pad_left(row, 6, 0);
        lv_obj_set_style_pad_right(row, 6, 0);
        lv_obj_set_style_pad_top(row, 3, 0);
        lv_obj_set_style_pad_bottom(row, 3, 0);
        lv_obj_set_style_radius(row, 4, 0);
        if (entry.focused) {
            lv_obj_set_style_bg_color(row, hex(kColorFocusFill), 0);
            lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
            lv_obj_set_style_border_color(row, hex(kColorFocusBorder), 0);
            lv_obj_set_style_border_width(row, 1, 0);
        } else {
            lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
        }

        auto* label = lv_label_create(row);
        lv_obj_add_style(label, entry.focused ? &style_title_ : &style_body_, 0);
        lv_label_set_text(label, entry.label.c_str());
        lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);

        if (!entry.meta.empty()) {
            const lv_image_dsc_t* icon = nullptr;
            if (entry.directory) {
                icon = entry.focused ? &g_folder_icon_18_image : &g_folder_icon_16_image;
            } else {
                icon = entry.focused ? &g_file_icon_18_image : &g_file_icon_16_image;
            }
            if (icon != nullptr) {
                auto* meta_icon = lv_image_create(row);
                lv_image_set_src(meta_icon, icon);
                lv_obj_align(meta_icon, LV_ALIGN_RIGHT_MID, -kFileRowIconPaddingRight, 0);
            }
        }

        if ((index + 1U) < current_file_entries_.size()) {
            auto* divider = lv_obj_create(list);
            lv_obj_remove_style_all(divider);
            lv_obj_set_size(divider, lv_pct(100), 1);
            lv_obj_set_style_bg_color(divider, hex(kColorPanelDivider), 0);
            lv_obj_set_style_bg_opa(divider, LV_OPA_COVER, 0);
            lv_obj_set_style_border_width(divider, 0, 0);
        }
    }
}

void ZephyrLvglShellUi::present_settings_surface(const shell::ShellPresentationFrame& frame) {
    update_top_bar("", "");
    const auto softkeys = softkeys_for_surface(shell::ShellSurface::Settings);
    update_softkeys(softkeys[0], softkeys[1], softkeys[2]);
    clear_content();

    auto* panel = create_panel(content_);
    lv_obj_set_size(panel, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(panel, hex(kColorBgPanel), 0);
    lv_obj_set_style_pad_left(panel, 10, 0);
    lv_obj_set_style_pad_right(panel, 10, 0);
    lv_obj_set_style_pad_top(panel, 8, 0);
    lv_obj_set_style_pad_bottom(panel, 4, 0);
    lv_obj_center(panel);

    auto* header = lv_obj_create(panel);
    lv_obj_remove_style_all(header);
    lv_obj_set_size(header, lv_pct(100), 24);
    lv_obj_set_style_bg_opa(header, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_pad_all(header, 0, 0);
    lv_obj_align(header, LV_ALIGN_TOP_LEFT, 0, 0);

    auto* title = lv_label_create(header);
    lv_obj_add_style(title, &style_title_, 0);
    lv_label_set_text(title, "Settings");
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 0, 0);

    auto* list = lv_obj_create(panel);
    lv_obj_remove_style_all(list);
    lv_obj_set_size(list, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(list, 0, 0);
    lv_obj_set_style_pad_top(list, 28, 0);
    lv_obj_set_style_pad_bottom(list, 0, 0);
    lv_obj_set_style_pad_left(list, 0, 0);
    lv_obj_set_style_pad_right(list, 0, 0);
    lv_obj_set_style_pad_row(list, 0, 0);
    lv_obj_set_layout(list, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_align(list, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_OFF);

    const auto entries = parse_settings_entries(frame);
    for (std::size_t index = 0; index < entries.size(); ++index) {
        const auto& entry = entries[index];

        auto* row = lv_obj_create(list);
        lv_obj_remove_style_all(row);
        lv_obj_set_size(row, lv_pct(100), 24);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_pad_left(row, 6, 0);
        lv_obj_set_style_pad_right(row, 6, 0);
        lv_obj_set_style_pad_top(row, 3, 0);
        lv_obj_set_style_pad_bottom(row, 3, 0);
        lv_obj_set_style_radius(row, 4, 0);
        if (entry.focused) {
            lv_obj_set_style_bg_color(row, hex(kColorFocusFill), 0);
            lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
            lv_obj_set_style_border_color(row, hex(kColorFocusBorder), 0);
            lv_obj_set_style_border_width(row, 1, 0);
        } else {
            lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
        }

        auto* label = lv_label_create(row);
        lv_obj_add_style(label, entry.focused ? &style_title_ : &style_body_, 0);
        lv_label_set_text(label, entry.label.c_str());
        lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);

        auto* value = lv_label_create(row);
        lv_obj_add_style(value, entry.focused ? &style_title_ : &style_body_, 0);
        lv_obj_set_style_text_color(value,
                                    hex(entry.dirty ? kColorWarning : kColorTextSecondary),
                                    0);
        const std::string suffix = entry.dirty ? " *" : "";
        const std::string text = entry.value + suffix;
        lv_label_set_text(value, text.c_str());
        lv_obj_align(value, LV_ALIGN_RIGHT_MID, 0, 0);

        if ((index + 1U) < entries.size()) {
            auto* divider = lv_obj_create(list);
            lv_obj_remove_style_all(divider);
            lv_obj_set_size(divider, lv_pct(100), 1);
            lv_obj_set_style_bg_color(divider, hex(kColorPanelDivider), 0);
            lv_obj_set_style_bg_opa(divider, LV_OPA_COVER, 0);
            lv_obj_set_style_border_width(divider, 0, 0);
        }
    }

}

void ZephyrLvglShellUi::present_generic_surface(const shell::ShellPresentationFrame& frame) {
    update_top_bar(to_route_name(frame.surface),
                   frame.detail.empty() ? frame.context : frame.detail);
    const auto softkeys = softkeys_for_surface(frame.surface);
    update_softkeys(softkeys[0], softkeys[1], softkeys[2]);
    clear_content();

    auto* panel = create_panel(content_);
    lv_obj_set_size(panel, lv_pct(100), lv_pct(100));
    lv_obj_center(panel);

    auto* title = lv_label_create(panel);
    lv_obj_add_style(title, &style_title_, 0);
    lv_label_set_text(title, to_route_name(frame.surface).c_str());
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 0, 0);

    if (!frame.context.empty()) {
        auto* context = lv_label_create(panel);
        lv_obj_add_style(context, &style_hint_, 0);
        lv_label_set_text(context, frame.context.c_str());
        lv_obj_align(context, LV_ALIGN_TOP_LEFT, 0, 22);
    }

    auto* body = lv_obj_create(panel);
    lv_obj_remove_style_all(body);
    lv_obj_set_style_bg_opa(body, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(body, 0, 0);
    lv_obj_set_style_pad_top(body, 8, 0);
    lv_obj_set_style_pad_row(body, 6, 0);
    lv_obj_set_size(body, lv_pct(100), lv_pct(100));
    lv_obj_align(body, LV_ALIGN_TOP_LEFT, 0, 40);
    lv_obj_set_layout(body, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(body, LV_FLEX_FLOW_COLUMN);

    if (frame.lines.empty()) {
        auto* empty = lv_label_create(body);
        lv_obj_add_style(empty, &style_hint_, 0);
        lv_label_set_text(empty, "No data yet");
        return;
    }

    for (const auto& line : frame.lines) {
        auto* row = lv_obj_create(body);
        lv_obj_remove_style_all(row);
        lv_obj_add_style(row, &style_tile_, 0);
        lv_obj_set_width(row, lv_pct(100));

        auto* label = lv_label_create(row);
        lv_obj_add_style(label, line.emphasized ? &style_title_ : &style_body_, 0);
        lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(label, lv_pct(100));
        lv_label_set_text(label, line.text.c_str());
    }
}

std::array<std::string_view, 3> ZephyrLvglShellUi::softkeys_for_surface(shell::ShellSurface surface) const {
    switch (surface) {
        case shell::ShellSurface::Home:
            return {"Open", "Select", "Back"};
        case shell::ShellSurface::Launcher:
            return {"Open", "Menu", "Back"};
        case shell::ShellSurface::Files:
            return {"Open", "Info", "Back"};
        case shell::ShellSurface::Settings:
            return {"Apply", "Select", "Back"};
        case shell::ShellSurface::Notifications:
            return {"Open", "Clear", "Back"};
        case shell::ShellSurface::AppForeground:
            return {"Action", "Select", "Back"};
        case shell::ShellSurface::Recovery:
            return {"Retry", "Select", "Back"};
    }

    return {"Open", "Select", "Back"};
}

shell::ShellNavigationAction ZephyrLvglShellUi::action_for_softkey(SoftkeySlot slot) const {
    switch (slot) {
        case SoftkeySlot::Left:
            if (active_surface_ == shell::ShellSurface::Files) {
                return shell::ShellNavigationAction::Select;
            }
            if (active_surface_ == shell::ShellSurface::Home) {
                return shell::ShellNavigationAction::Select;
            }
            if (active_surface_ == shell::ShellSurface::Launcher ||
                active_surface_ == shell::ShellSurface::Settings ||
                active_surface_ == shell::ShellSurface::Notifications ||
                active_surface_ == shell::ShellSurface::Recovery) {
                return shell::ShellNavigationAction::PrimaryAction;
            }
            return shell::ShellNavigationAction::PrimaryAction;
        case SoftkeySlot::Center:
            if (active_surface_ == shell::ShellSurface::Files) {
                return shell::ShellNavigationAction::PrimaryAction;
            }
            if (active_surface_ == shell::ShellSurface::Launcher ||
                active_surface_ == shell::ShellSurface::Home ||
                active_surface_ == shell::ShellSurface::Settings ||
                active_surface_ == shell::ShellSurface::AppForeground ||
                active_surface_ == shell::ShellSurface::Recovery) {
                return shell::ShellNavigationAction::Select;
            }
            return shell::ShellNavigationAction::OpenNotifications;
        case SoftkeySlot::Right:
            return shell::ShellNavigationAction::Back;
    }

    return shell::ShellNavigationAction::Back;
}

std::vector<ZephyrLvglShellUi::LauncherEntryView> ZephyrLvglShellUi::parse_launcher_entries(
    const shell::ShellPresentationFrame& frame) const {
    std::vector<LauncherEntryView> entries;
    entries.reserve(frame.lines.size());

    for (const auto& line : frame.lines) {
        auto text = trim(line.text);
        if (text.rfind("about:", 0) == 0 || text == "No launcher entries") {
            continue;
        }

        LauncherEntryView entry;
        entry.focused = line.emphasized || text.rfind("[*]", 0) == 0;

        if (text.rfind("[*]", 0) == 0 || text.rfind("[ ]", 0) == 0) {
            text = trim(text.substr(3));
        }
        while (!text.empty() && std::isdigit(static_cast<unsigned char>(text.front()))) {
            text.erase(text.begin());
        }
        text = trim(text);
        if (!text.empty() && text.back() == '*') {
            text.pop_back();
            text = trim(text);
        }

        const auto bracket = text.find(" [");
        if (bracket != std::string::npos) {
            entry.secondary = trim(text.substr(bracket + 1));
            entry.label = trim(text.substr(0, bracket));
        } else {
            entry.label = trim(text);
        }

        if (entry.secondary.find("permission denied") != std::string::npos) {
            entry.blocked = true;
            entry.warning = true;
        } else if (!entry.secondary.empty()) {
            entry.warning = true;
        }

        if (!entry.label.empty()) {
            entries.push_back(std::move(entry));
        }
    }

    return entries;
}

std::vector<ZephyrLvglShellUi::SettingsEntryView> ZephyrLvglShellUi::parse_settings_entries(
    const shell::ShellPresentationFrame& frame) const {
    std::vector<SettingsEntryView> entries;
    entries.reserve(frame.lines.size());

    for (const auto& line : frame.lines) {
        auto text = trim(line.text);
        if (text.empty() || text.rfind("Section:", 0) == 0) {
            continue;
        }

        SettingsEntryView entry;
        entry.focused = line.emphasized || text.rfind("[*]", 0) == 0;
        if (text.rfind("[*]", 0) == 0 || text.rfind("[ ]", 0) == 0) {
            text = trim(text.substr(3));
        }
        entry.dirty = !text.empty() && text.back() == '*';
        if (entry.dirty) {
            text.pop_back();
            text = trim(text);
        }

        const auto separator = text.find(':');
        if (separator == std::string::npos) {
            continue;
        }
        entry.label = trim(text.substr(0, separator));
        entry.value = trim(text.substr(separator + 1));
        if (!entry.label.empty()) {
            entries.push_back(std::move(entry));
        }
    }

    return entries;
}

std::vector<ZephyrLvglShellUi::FileEntryView> ZephyrLvglShellUi::parse_file_entries(
    const shell::ShellPresentationFrame& frame) const {
    std::vector<FileEntryView> entries;
    entries.reserve(frame.lines.size());

    for (const auto& line : frame.lines) {
        auto text = trim(line.text);
        if (text.empty()) {
            continue;
        }
        if (text == "No SD card inserted" || text == "Insert a card to browse storage" ||
            text == "Storage unavailable" || text == "The SD card mount is not ready" ||
            text == "Folder is empty") {
            continue;
        }

        FileEntryView entry;
        entry.focused = line.emphasized || text.rfind("[*]", 0) == 0;
        if (text.rfind("[*]", 0) == 0 || text.rfind("[ ]", 0) == 0) {
            text = trim(text.substr(3));
        }

        const auto bracket = text.find(" [");
        if (bracket != std::string::npos) {
            entry.label = trim(text.substr(0, bracket));
            entry.meta = trim(text.substr(bracket + 1));
            if (!entry.meta.empty() && entry.meta.back() == ']') {
                entry.meta.pop_back();
            }
            entry.directory = entry.meta == "DIR";
        } else {
            entry.label = trim(text);
        }
        if (!entry.label.empty()) {
            entries.push_back(std::move(entry));
        }
    }

    return entries;
}

std::string ZephyrLvglShellUi::trim(std::string_view value) {
    std::size_t start = 0;
    while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start]))) {
        ++start;
    }

    std::size_t end = value.size();
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
        --end;
    }

    return std::string(value.substr(start, end - start));
}

std::string ZephyrLvglShellUi::to_route_name(shell::ShellSurface surface) {
    switch (surface) {
        case shell::ShellSurface::Home:
            return "HOME";
        case shell::ShellSurface::Launcher:
            return "MAIN MENU";
        case shell::ShellSurface::Files:
            return "FILES";
        case shell::ShellSurface::Settings:
            return "SETTINGS";
        case shell::ShellSurface::Notifications:
            return "NOTIFICATIONS";
        case shell::ShellSurface::AppForeground:
            return "APP";
        case shell::ShellSurface::Recovery:
            return "RECOVERY";
    }

    return "AEGIS";
}

lv_color_t ZephyrLvglShellUi::hex(uint32_t rgb) const {
    return lv_color_hex(rgb);
}

lv_obj_t* ZephyrLvglShellUi::create_panel(lv_obj_t* parent) const {
    auto* panel = lv_obj_create(parent);
    lv_obj_remove_style_all(panel);
    lv_obj_add_style(panel, &style_panel_, 0);
    return panel;
}

std::vector<ZephyrLvglShellUi::BuiltinMenuEntry> ZephyrLvglShellUi::builtin_menu_entries() const {
    return {
        {.id = "apps", .label = "Apps", .color = 0xD7E4EC},
        {.id = "files", .label = "Files", .color = 0x4E8FBE, .available = true, .use_files_icon = true, .action = shell::ShellNavigationAction::OpenFiles},
        {.id = "settings", .label = "Settings", .color = 0xE9EEF1, .available = true, .use_settings_icon = true, .action = shell::ShellNavigationAction::OpenSettings},
        {.id = "radio", .label = "Radio", .color = 0xDCE8D6},
        {.id = "messages", .label = "Messages", .color = 0xE5DDD2},
        {.id = "device", .label = "Device", .color = 0xDDD9E8},
        {.id = "updates", .label = "Updates", .color = 0xE7E2D0},
        {.id = "tools", .label = "Tools", .color = 0xD8E1E6},
        {.id = "help", .label = "Help", .color = 0xE5E2D7},
    };
}

void ZephyrLvglShellUi::render_system_menu(const shell::ShellPresentationFrame& frame) {
    clear_content();

    auto* grid = lv_obj_create(content_);
    lv_obj_remove_style_all(grid);
    lv_obj_set_style_bg_color(grid, hex(kColorBgRoot), 0);
    lv_obj_set_style_bg_opa(grid, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(grid, 0, 0);
    lv_obj_set_style_pad_top(grid, 10, 0);
    lv_obj_set_style_pad_bottom(grid, 8, 0);
    lv_obj_set_style_pad_left(grid, 6, 0);
    lv_obj_set_style_pad_right(grid, 6, 0);
    lv_obj_set_style_pad_row(grid, 16, 0);
    lv_obj_set_style_pad_column(grid, 4, 0);
    lv_obj_set_size(grid, lv_pct(100), lv_pct(100));
    lv_obj_align(grid, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_layout(grid, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(grid, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_scrollbar_mode(grid, LV_SCROLLBAR_MODE_ACTIVE);
    lv_obj_set_style_width(grid, 4, LV_PART_SCROLLBAR);
    lv_obj_set_style_radius(grid, 2, LV_PART_SCROLLBAR);
    lv_obj_set_style_bg_color(grid, hex(kColorFocusBorder), LV_PART_SCROLLBAR);
    lv_obj_set_style_bg_opa(grid, LV_OPA_70, LV_PART_SCROLLBAR);

    const auto entries = builtin_menu_entries();
    const auto launcher_entries = parse_launcher_entries(frame);
    for (std::size_t index = 0; index < entries.size(); ++index) {
        bool focused = false;
        for (const auto& launcher_entry : launcher_entries) {
            if (launcher_entry.label == entries[index].label) {
                focused = launcher_entry.focused;
                break;
            }
        }
        create_builtin_tile(grid, entries[index], focused);
    }
}

void ZephyrLvglShellUi::create_builtin_tile(lv_obj_t* parent,
                                            const BuiltinMenuEntry& entry,
                                            bool focused) {
    auto* tile = lv_obj_create(parent);
    lv_obj_remove_style_all(tile);
    lv_obj_set_size(tile, kTileWidth, kTileHeight);
    lv_obj_set_style_bg_opa(tile, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(tile, 0, 0);
    lv_obj_set_style_outline_width(tile, 0, 0);
    lv_obj_set_style_shadow_width(tile, 0, 0);
    lv_obj_set_style_radius(tile, 10, 0);
    if (focused) {
        lv_obj_set_style_bg_color(tile, hex(kColorFocusFill), 0);
        lv_obj_set_style_bg_opa(tile, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(tile, hex(kColorFocusBorder), 0);
        lv_obj_set_style_border_width(tile, 1, 0);
    }
    lv_obj_clear_flag(tile, LV_OBJ_FLAG_SCROLLABLE);

    auto* icon_wrap = lv_obj_create(tile);
    lv_obj_remove_style_all(icon_wrap);
    lv_obj_set_size(icon_wrap, kIconWrapSize, kIconWrapSize);
    lv_obj_align(icon_wrap, LV_ALIGN_TOP_MID, 0, 2);
    lv_obj_set_style_bg_opa(icon_wrap, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(icon_wrap, 0, 0);
    lv_obj_set_style_outline_width(icon_wrap, 0, 0);
    lv_obj_set_style_radius(icon_wrap, 0, 0);
    lv_obj_set_style_shadow_width(icon_wrap, 0, 0);
    lv_obj_clear_flag(icon_wrap, LV_OBJ_FLAG_SCROLLABLE);

    if (entry.use_settings_icon) {
        auto* image = lv_image_create(icon_wrap);
        lv_image_set_src(image, &g_settings_icon_image);
        lv_obj_set_style_bg_opa(image, LV_OPA_TRANSP, 0);
        lv_obj_center(image);
    } else if (entry.use_files_icon) {
        auto* image = lv_image_create(icon_wrap);
        lv_image_set_src(image, &g_files_icon_image);
        lv_obj_set_style_bg_opa(image, LV_OPA_TRANSP, 0);
        lv_obj_center(image);
    } else {
        auto* badge = lv_label_create(icon_wrap);
        lv_obj_add_style(badge, &style_icon_label_, 0);
        const char* symbol = "";
        if (entry.id == "apps") {
            symbol = LV_SYMBOL_LIST;
        } else if (entry.id == "files") {
            symbol = LV_SYMBOL_DIRECTORY;
        } else if (entry.id == "radio") {
            symbol = LV_SYMBOL_WIFI;
        } else if (entry.id == "tools") {
            symbol = LV_SYMBOL_EDIT;
        } else if (entry.id == "device") {
            symbol = LV_SYMBOL_BATTERY_FULL;
        }
        lv_label_set_text(badge, symbol);
        lv_obj_set_style_text_color(badge,
                                    hex(entry.id == "files" ? kColorTextInverse : kColorFocusBorder),
                                    0);
        lv_obj_center(badge);
    }

    auto* label = lv_label_create(tile);
    lv_obj_add_style(label, focused ? &style_title_ : &style_body_, 0);
    lv_label_set_text(label, entry.label.c_str());
    lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, -2);
    lv_obj_set_style_text_color(label, hex(kColorTextPrimary), 0);

    if (entry.available) {
        tile_actions_.push_back(entry.action);
        register_touch_target(tile, tile_actions_.back(), false, true);
        lv_obj_add_flag(tile, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_flag(icon_wrap, LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_add_flag(label, LV_OBJ_FLAG_EVENT_BUBBLE);
          lv_obj_add_event_cb(
              tile, builtin_tile_event_bridge, LV_EVENT_CLICKED, &tile_actions_.back());
        lv_obj_add_event_cb(
            tile, builtin_tile_event_bridge, LV_EVENT_SHORT_CLICKED, &tile_actions_.back());
    }
}

void ZephyrLvglShellUi::update_status_clock() {
    if (clock_label_ == nullptr) {
        return;
    }

    const uint32_t now_ms = lv_tick_get();
    if (last_clock_update_ms_ != 0 && (now_ms - last_clock_update_ms_) < kClockRefreshMs) {
        return;
    }
    last_clock_update_ms_ = now_ms;

    std::string text;
    if (clock_text_callback_) {
        text = clock_text_callback_();
    }
    if (text.empty()) {
        char buffer[6] {};
        const uint32_t seconds = now_ms / 1000U;
        const uint32_t hours = (seconds / 3600U) % 24U;
        const uint32_t minutes = (seconds / 60U) % 60U;
        std::snprintf(buffer, sizeof(buffer), "%02u:%02u", hours, minutes);
        text = buffer;
    }

    lv_label_set_text(clock_label_, text.c_str());
    lv_obj_align(clock_label_, LV_ALIGN_CENTER, 0, 0);
    request_render();
}

void ZephyrLvglShellUi::update_touch_feedback() {
    if (touch_feedback_target_ == nullptr || touch_feedback_deadline_ms_ == 0) {
        return;
    }
    const uint32_t now_ms = lv_tick_get();
    if (now_ms < touch_feedback_deadline_ms_) {
        return;
    }
    if (touch_feedback_target_ != touch_pressed_target_) {
        set_touch_target_pressed(touch_feedback_target_, false);
    }
    touch_feedback_target_ = nullptr;
    touch_feedback_deadline_ms_ = 0;
}

void ZephyrLvglShellUi::update_pending_action() {
    if (!pending_action_active_ || pending_action_deadline_ms_ == 0) {
        return;
    }
    const uint32_t now_ms = lv_tick_get();
    if (now_ms < pending_action_deadline_ms_) {
        return;
    }
    pending_action_active_ = false;
    pending_action_deadline_ms_ = 0;
      dispatch_action(pending_action_);
}

void ZephyrLvglShellUi::register_touch_target(lv_obj_t* object,
                                              shell::ShellNavigationAction action,
                                              bool is_softkey,
                                              bool supports_pressed_feedback) {
    if (object == nullptr) {
        return;
    }
    touch_targets_.push_back({.object = object,
                              .action = action,
                              .is_softkey = is_softkey,
                              .supports_pressed_feedback = supports_pressed_feedback});
}

ZephyrLvglShellUi::TouchTarget* ZephyrLvglShellUi::find_touch_target(int16_t x, int16_t y) {
    for (auto& target : touch_targets_) {
        if (target.object == nullptr) {
            continue;
        }
        lv_area_t coords {};
        lv_obj_get_coords(target.object, &coords);
        if (x >= coords.x1 && x <= coords.x2 && y >= coords.y1 && y <= coords.y2) {
            return &target;
        }
    }
    return nullptr;
}

void ZephyrLvglShellUi::set_softkey_pressed(SoftkeySlot slot, bool pressed) {
    lv_obj_t* target = nullptr;
    lv_obj_t* label = nullptr;
    switch (slot) {
        case SoftkeySlot::Left:
            target = softkey_left_slot_;
            label = softkey_left_;
            break;
        case SoftkeySlot::Center:
            target = softkey_center_slot_;
            label = softkey_center_;
            break;
        case SoftkeySlot::Right:
            target = softkey_right_slot_;
            label = softkey_right_;
            break;
    }
    if (target == nullptr) {
        return;
    }
    if (pressed) {
        lv_obj_set_style_bg_color(target, hex(0x6D8395), 0);
        lv_obj_set_style_bg_opa(target, LV_OPA_70, 0);
        lv_obj_set_style_radius(target, 0, 0);
        if (label != nullptr) {
            lv_obj_set_style_text_color(label, hex(kColorTextInverse), 0);
        }
    } else {
        lv_obj_set_style_bg_opa(target, LV_OPA_TRANSP, 0);
        if (label != nullptr) {
            lv_obj_set_style_text_color(label, hex(kColorTextInverse), 0);
        }
    }
    request_render();
}

void ZephyrLvglShellUi::set_touch_target_pressed(TouchTarget* target, bool pressed) {
    if (target == nullptr || target->object == nullptr || !target->supports_pressed_feedback) {
        return;
    }
    if (target->is_softkey) {
        if (target->object == softkey_left_slot_) {
            set_softkey_pressed(SoftkeySlot::Left, pressed);
        } else if (target->object == softkey_center_slot_) {
            set_softkey_pressed(SoftkeySlot::Center, pressed);
        } else if (target->object == softkey_right_slot_) {
            set_softkey_pressed(SoftkeySlot::Right, pressed);
        }
        return;
    }

    if (pressed) {
          lv_area_t coords {};
        lv_obj_get_coords(target->object, &coords);
        if (touch_feedback_overlay_ != nullptr) {
            if (target->is_softkey) {
                lv_obj_set_style_bg_color(touch_feedback_overlay_, hex(0xD7E6F5), 0);
                lv_obj_set_style_bg_opa(touch_feedback_overlay_, LV_OPA_20, 0);
                lv_obj_set_style_border_color(touch_feedback_overlay_, hex(kColorTextInverse), 0);
                lv_obj_set_style_radius(touch_feedback_overlay_, 0, 0);
            } else {
                lv_obj_set_style_bg_color(touch_feedback_overlay_, hex(kColorFocusFill), 0);
                lv_obj_set_style_bg_opa(touch_feedback_overlay_, LV_OPA_30, 0);
                lv_obj_set_style_border_color(touch_feedback_overlay_, hex(kColorFocusBorder), 0);
                lv_obj_set_style_radius(touch_feedback_overlay_, 8, 0);
            }
            lv_obj_set_pos(touch_feedback_overlay_, coords.x1, coords.y1);
            lv_obj_set_size(touch_feedback_overlay_,
                            (coords.x2 - coords.x1) + 1,
                            (coords.y2 - coords.y1) + 1);
            lv_obj_move_foreground(touch_feedback_overlay_);
            lv_obj_clear_flag(touch_feedback_overlay_, LV_OBJ_FLAG_HIDDEN);
        }
    } else {
          if (touch_feedback_overlay_ != nullptr && touch_feedback_target_ == target) {
              lv_obj_add_flag(touch_feedback_overlay_, LV_OBJ_FLAG_HIDDEN);
        }
    }
    request_render();
}

void ZephyrLvglShellUi::clear_softkey_pressed_state() {
    set_softkey_pressed(SoftkeySlot::Left, false);
    set_softkey_pressed(SoftkeySlot::Center, false);
    set_softkey_pressed(SoftkeySlot::Right, false);
    softkey_pressed_active_ = false;
}

bool ZephyrLvglShellUi::files_info_available() const {
    if (active_surface_ != shell::ShellSurface::Files || current_file_entries_.empty()) {
        return false;
    }
    for (const auto& entry : current_file_entries_) {
        if (entry.focused) {
            return !entry.directory;
        }
    }
    return false;
}

void ZephyrLvglShellUi::show_files_info_popup() {
    if (!files_info_available() || files_info_popup_ == nullptr ||
        files_info_popup_title_ == nullptr || files_info_popup_body_ == nullptr) {
        return;
    }
    for (const auto& entry : current_file_entries_) {
        if (!entry.focused || entry.directory) {
            continue;
        }
        lv_label_set_text(files_info_popup_title_, "File Info");
        std::string body = "Name: " + entry.label;
        if (!entry.meta.empty()) {
            body += "\nSize: ";
            body += entry.meta;
        }
        lv_label_set_text(files_info_popup_body_, body.c_str());
        lv_obj_clear_flag(files_info_popup_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(files_info_popup_);
        files_info_popup_visible_ = true;
        request_render();
        return;
    }
}

void ZephyrLvglShellUi::hide_files_info_popup() {
    if (files_info_popup_ == nullptr) {
        return;
    }
    lv_obj_add_flag(files_info_popup_, LV_OBJ_FLAG_HIDDEN);
    files_info_popup_visible_ = false;
    request_render();
}

void ZephyrLvglShellUi::poll_touch_actions() {
    if (!cached_touch_valid_ || touch_targets_.empty()) {
        return;
    }

    const int16_t x = cached_touch_x_;
    const int16_t y = cached_touch_y_;
    const bool pressed = cached_touch_pressed_;

    if (pressed) {
        auto* target = find_touch_target(x, y);
        if (!touch_tracking_active_) {
            touch_tracking_active_ = true;
            touch_pressed_target_ = target;
            if (target != nullptr) {
                if (target->is_softkey) {
                    if (target->object == softkey_left_slot_) {
                        active_pressed_softkey_ = SoftkeySlot::Left;
                    } else if (target->object == softkey_center_slot_) {
                        active_pressed_softkey_ = SoftkeySlot::Center;
                    } else {
                        active_pressed_softkey_ = SoftkeySlot::Right;
                    }
                    softkey_pressed_active_ = true;
                }
                set_touch_target_pressed(target, true);
                logger_.info("lvgl",
                             "touch target press x=" + std::to_string(x) + " y=" +
                                 std::to_string(y) + " action=" +
                                 std::string(shell::to_string(target->action)) +
                                 " softkey=" + std::string(target->is_softkey ? "yes" : "no"));
            }
            return;
        }

        if (target != touch_pressed_target_) {
            if (touch_pressed_target_ != nullptr) {
                set_touch_target_pressed(touch_pressed_target_, false);
            }
            touch_pressed_target_ = target;
            if (target != nullptr) {
                if (target->is_softkey) {
                    if (target->object == softkey_left_slot_) {
                        active_pressed_softkey_ = SoftkeySlot::Left;
                    } else if (target->object == softkey_center_slot_) {
                        active_pressed_softkey_ = SoftkeySlot::Center;
                    } else {
                        active_pressed_softkey_ = SoftkeySlot::Right;
                    }
                    softkey_pressed_active_ = true;
                }
                set_touch_target_pressed(target, true);
                logger_.info("lvgl",
                             "touch target move x=" + std::to_string(x) + " y=" +
                                 std::to_string(y) + " action=" +
                                 std::string(shell::to_string(target->action)) +
                                 " softkey=" + std::string(target->is_softkey ? "yes" : "no"));
            }
        }
        return;
    }

    if (!touch_tracking_active_) {
        return;
    }

    auto* release_target = find_touch_target(x, y);
    auto* pressed_target = touch_pressed_target_;
    if (pressed_target != nullptr) {
        set_touch_target_pressed(pressed_target, false);
    }
    if (softkey_pressed_active_) {
        softkey_pressed_active_ = false;
    }
    touch_tracking_active_ = false;
    touch_pressed_target_ = nullptr;

    if (pressed_target == nullptr || release_target != pressed_target) {
        logger_.info("lvgl",
                     "touch release missed x=" + std::to_string(x) + " y=" + std::to_string(y));
        return;
    }
    if (pressed_target->object == nullptr || !lv_obj_is_valid(pressed_target->object)) {
        return;
    }

    logger_.info("lvgl",
                 "touch release activate x=" + std::to_string(x) + " y=" + std::to_string(y) +
                     " action=" + std::string(shell::to_string(pressed_target->action)));
    dispatch_action(pressed_target->action);
}

void ZephyrLvglShellUi::dispatch_action(shell::ShellNavigationAction action) {
    if (active_surface_ == shell::ShellSurface::Files && files_info_popup_visible_) {
        if (action == shell::ShellNavigationAction::Back ||
            action == shell::ShellNavigationAction::PrimaryAction ||
            action == shell::ShellNavigationAction::Select) {
            hide_files_info_popup();
            if (action == shell::ShellNavigationAction::Back) {
                return;
            }
        }
    }
    if (active_surface_ == shell::ShellSurface::Files &&
        action == shell::ShellNavigationAction::PrimaryAction) {
        if (!files_info_available()) {
            logger_.info("lvgl", "files info disabled for focused directory");
            return;
        }
        show_files_info_popup();
        return;
    }
    logger_.info("lvgl", "dispatch action " + std::string(shell::to_string(action)));
    if (action_callback_) {
        action_callback_(action);
    }
}

void ZephyrLvglShellUi::pump_once() {
    render_pending_ = false;
    (void)lv_timer_handler();
}

void ZephyrLvglShellUi::request_render() {
    render_pending_ = true;
    lv_obj_t* target = lv_screen_active();
    if (target == nullptr) {
        target = screen_ != nullptr ? screen_ : splash_screen_;
    }
    if (target != nullptr) {
        lv_obj_invalidate(target);
    }
}

}  // namespace aegis::ports::zephyr
