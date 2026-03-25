#include "ports/zephyr/zephyr_lvgl_shell_ui.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <string_view>
#include <utility>

#include "ports/zephyr/zephyr_lvgl_assets.hpp"

namespace aegis::ports::zephyr {

namespace {

constexpr uint32_t kColorBgRoot = 0xECE6D8;
constexpr uint32_t kColorBgPanel = 0xF6F1E6;
constexpr uint32_t kColorBgStatus = 0xD8CFBE;
constexpr uint32_t kColorBgSoftkey = 0xD4CAB8;
constexpr uint32_t kColorTextPrimary = 0x26231F;
constexpr uint32_t kColorTextSecondary = 0x4A453D;
constexpr uint32_t kColorTextMuted = 0x6B655C;
constexpr uint32_t kColorTextInverse = 0xF8F4EC;
constexpr uint32_t kColorBorderStrong = 0x5A554D;
constexpr uint32_t kColorBorderDefault = 0x7B746A;
constexpr uint32_t kColorFocusFill = 0x2F5B6A;
constexpr uint32_t kColorFocusBorder = 0x21424D;
constexpr uint32_t kColorWarning = 0xA45A2A;
constexpr uint32_t kColorPolicy = 0x6C557A;
constexpr uint32_t kColorReady = 0x4E6B50;

constexpr int kStatusBarHeight = 28;
constexpr int kSoftkeyBarHeight = 24;
constexpr int kContentInset = 8;
constexpr uint32_t kSplashDelayMs = 0;
}  // namespace

ZephyrLvglShellUi::ZephyrLvglShellUi(platform::Logger& logger,
                                     ZephyrBoardBackendConfig config,
                                     FlushCallback flush_callback)
    : logger_(logger), config_(std::move(config)), flush_callback_(std::move(flush_callback)) {}

bool ZephyrLvglShellUi::initialize() {
    lv_init();

    const std::size_t width = static_cast<std::size_t>(std::max(1, config_.display_width));
    const std::size_t band_height = std::min<std::size_t>(40, static_cast<std::size_t>(std::max(16, config_.display_height / 4)));
    const std::size_t pixel_count = width * band_height;
    draw_buffer_a_.assign(pixel_count, 0U);
    draw_buffer_b_.assign(pixel_count, 0U);

    display_ = lv_display_create(config_.display_width, config_.display_height);
    if (display_ == nullptr) {
        logger_.info("lvgl", "display create failed");
        return false;
    }

    lv_display_set_user_data(display_, this);
    lv_display_set_color_format(display_, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display_,
                           draw_buffer_a_.data(),
                           draw_buffer_b_.data(),
                           static_cast<uint32_t>(pixel_count * sizeof(uint16_t)),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display_, display_flush_bridge);
    lv_display_set_default(display_);

    ensure_theme();
    build_shell_chrome();
    show_startup_splash("Starting system");
    return true;
}

void ZephyrLvglShellUi::tick(uint32_t elapsed_ms) {
    lv_tick_inc(elapsed_ms == 0 ? 1U : elapsed_ms);
    if (render_pending_) {
        pump_once();
    }
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

    logger_.info("lvgl", "show startup splash stage=" + trim(stage));
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

    switch (frame.surface) {
        case shell::ShellSurface::Home:
            present_home(frame);
            break;
        case shell::ShellSurface::Launcher:
            present_launcher(frame);
            break;
        case shell::ShellSurface::Settings:
        case shell::ShellSurface::Notifications:
        case shell::ShellSurface::AppForeground:
        case shell::ShellSurface::Recovery:
            present_generic_surface(frame);
            break;
    }

    logger_.info("lvgl",
                 "present surface=" + to_route_name(frame.surface) +
                     " lines=" + std::to_string(frame.lines.size()));
    request_render();
    pump_once();
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
    if (flush_count_ <= 12U || (flush_count_ % 25U) == 0U) {
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
    lv_style_set_bg_opa(&style_status_bar_, LV_OPA_COVER);
    lv_style_set_border_color(&style_status_bar_, hex(kColorBorderStrong));
    lv_style_set_border_width(&style_status_bar_, 1);
    lv_style_set_pad_left(&style_status_bar_, 8);
    lv_style_set_pad_right(&style_status_bar_, 8);
    lv_style_set_pad_top(&style_status_bar_, 4);
    lv_style_set_pad_bottom(&style_status_bar_, 4);

    lv_style_init(&style_softkey_bar_);
    lv_style_set_bg_color(&style_softkey_bar_, hex(kColorBgSoftkey));
    lv_style_set_bg_opa(&style_softkey_bar_, LV_OPA_COVER);
    lv_style_set_border_color(&style_softkey_bar_, hex(kColorBorderStrong));
    lv_style_set_border_width(&style_softkey_bar_, 1);
    lv_style_set_pad_left(&style_softkey_bar_, 8);
    lv_style_set_pad_right(&style_softkey_bar_, 8);
    lv_style_set_pad_top(&style_softkey_bar_, 3);
    lv_style_set_pad_bottom(&style_softkey_bar_, 3);

    lv_style_init(&style_panel_);
    lv_style_set_bg_color(&style_panel_, hex(kColorBgPanel));
    lv_style_set_bg_opa(&style_panel_, LV_OPA_COVER);
    lv_style_set_border_color(&style_panel_, hex(kColorBorderStrong));
    lv_style_set_border_width(&style_panel_, 2);
    lv_style_set_radius(&style_panel_, 2);
    lv_style_set_pad_all(&style_panel_, 8);

    lv_style_init(&style_tile_);
    lv_style_set_bg_color(&style_tile_, hex(kColorBgPanel));
    lv_style_set_bg_opa(&style_tile_, LV_OPA_COVER);
    lv_style_set_border_color(&style_tile_, hex(kColorBorderDefault));
    lv_style_set_border_width(&style_tile_, 1);
    lv_style_set_radius(&style_tile_, 2);
    lv_style_set_pad_left(&style_tile_, 6);
    lv_style_set_pad_right(&style_tile_, 6);
    lv_style_set_pad_top(&style_tile_, 6);
    lv_style_set_pad_bottom(&style_tile_, 6);

    lv_style_init(&style_tile_focus_);
    lv_style_set_bg_color(&style_tile_focus_, hex(kColorFocusFill));
    lv_style_set_bg_opa(&style_tile_focus_, LV_OPA_COVER);
    lv_style_set_border_color(&style_tile_focus_, hex(kColorFocusBorder));
    lv_style_set_border_width(&style_tile_focus_, 2);

    lv_style_init(&style_tile_warning_);
    lv_style_set_border_color(&style_tile_warning_, hex(kColorWarning));
    lv_style_set_border_width(&style_tile_warning_, 2);

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

    theme_ready_ = true;
}

void ZephyrLvglShellUi::build_shell_chrome() {
    screen_ = lv_obj_create(nullptr);
    lv_obj_remove_style_all(screen_);
    lv_obj_add_style(screen_, &style_screen_, 0);
    lv_obj_set_size(screen_, config_.display_width, config_.display_height);
    lv_obj_set_style_pad_all(screen_, 0, 0);

    status_bar_ = lv_obj_create(screen_);
    lv_obj_remove_style_all(status_bar_);
    lv_obj_add_style(status_bar_, &style_status_bar_, 0);
    lv_obj_set_size(status_bar_, lv_pct(100), kStatusBarHeight);
    lv_obj_align(status_bar_, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_layout(status_bar_, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(status_bar_, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(status_bar_, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    title_label_ = lv_label_create(status_bar_);
    lv_obj_add_style(title_label_, &style_title_, 0);

    detail_label_ = lv_label_create(status_bar_);
    lv_obj_add_style(detail_label_, &style_hint_, 0);

    content_ = lv_obj_create(screen_);
    lv_obj_remove_style_all(content_);
    lv_obj_set_style_bg_opa(content_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content_, 0, 0);
    lv_obj_set_style_pad_all(content_, kContentInset, 0);
    lv_obj_set_size(content_,
                    lv_pct(100),
                    config_.display_height - kStatusBarHeight - kSoftkeyBarHeight);
    lv_obj_align(content_, LV_ALIGN_TOP_LEFT, 0, kStatusBarHeight);

    softkey_bar_ = lv_obj_create(screen_);
    lv_obj_remove_style_all(softkey_bar_);
    lv_obj_add_style(softkey_bar_, &style_softkey_bar_, 0);
    lv_obj_set_size(softkey_bar_, lv_pct(100), kSoftkeyBarHeight);
    lv_obj_align(softkey_bar_, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_layout(softkey_bar_, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(softkey_bar_, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(softkey_bar_, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    softkey_left_ = lv_label_create(softkey_bar_);
    lv_obj_add_style(softkey_left_, &style_hint_, 0);
    softkey_center_ = lv_label_create(softkey_bar_);
    lv_obj_add_style(softkey_center_, &style_hint_, 0);
    softkey_right_ = lv_label_create(softkey_bar_);
    lv_obj_add_style(softkey_right_, &style_hint_, 0);
}

void ZephyrLvglShellUi::clear_content() {
    if (content_ != nullptr) {
        lv_obj_clean(content_);
    }
}

void ZephyrLvglShellUi::update_top_bar(std::string_view route, std::string_view detail) {
    if (title_label_ == nullptr || detail_label_ == nullptr) {
        return;
    }

    const auto route_text = std::string("AEGIS / ") + std::string(route);
    lv_label_set_text(title_label_, route_text.c_str());
    lv_label_set_text(detail_label_, trim(detail).c_str());
}

void ZephyrLvglShellUi::update_softkeys(std::string_view left,
                                        std::string_view center,
                                        std::string_view right) {
    lv_label_set_text(softkey_left_, trim(left).c_str());
    lv_label_set_text(softkey_center_, trim(center).c_str());
    lv_label_set_text(softkey_right_, trim(right).c_str());
}

void ZephyrLvglShellUi::present_home(const shell::ShellPresentationFrame& frame) {
    update_top_bar("HOME", "Select opens launcher");
    update_softkeys("Open", "Select", "Back");
    clear_content();

    auto* panel = create_panel(content_);
    lv_obj_center(panel);
    lv_obj_set_size(panel, lv_pct(100), lv_pct(100));

    auto* hero = lv_image_create(panel);
    lv_image_set_src(hero, &g_aegis_logo_image);
    lv_obj_align(hero, LV_ALIGN_TOP_MID, 0, 8);

    auto* summary = lv_label_create(panel);
    lv_obj_add_style(summary, &style_body_, 0);
    lv_label_set_text(summary, "Quiet, governed, ready.");
    lv_obj_align(summary, LV_ALIGN_TOP_MID, 0, 98);

    auto* notes = lv_label_create(panel);
    lv_obj_add_style(notes, &style_hint_, 0);
    std::string body;
    for (const auto& line : frame.lines) {
        if (!body.empty()) {
            body += "\n";
        }
        body += line.text;
    }
    lv_label_set_text(notes, body.c_str());
    lv_obj_set_width(notes, lv_pct(100));
    lv_label_set_long_mode(notes, LV_LABEL_LONG_WRAP);
    lv_obj_align(notes, LV_ALIGN_TOP_LEFT, 8, 130);
}

void ZephyrLvglShellUi::present_launcher(const shell::ShellPresentationFrame& frame) {
    update_top_bar("MAIN MENU", frame.detail.empty() ? "Launcher" : frame.detail);
    update_softkeys("Launch", "Select", "Back");
    clear_content();

    auto* panel = create_panel(content_);
    lv_obj_set_size(panel, lv_pct(100), lv_pct(100));
    lv_obj_center(panel);

    auto* header = lv_label_create(panel);
    lv_obj_add_style(header, &style_title_, 0);
    lv_label_set_text(header, "Main Menu");
    lv_obj_align(header, LV_ALIGN_TOP_LEFT, 0, 0);

    auto* summary = lv_label_create(panel);
    lv_obj_add_style(summary, &style_hint_, 0);
    lv_label_set_text(summary, frame.context.empty() ? "System catalog" : frame.context.c_str());
    lv_obj_align(summary, LV_ALIGN_TOP_RIGHT, 0, 2);

    auto* grid = lv_obj_create(panel);
    lv_obj_remove_style_all(grid);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(grid, 0, 0);
    lv_obj_set_style_pad_top(grid, 10, 0);
    lv_obj_set_style_pad_bottom(grid, 0, 0);
    lv_obj_set_style_pad_row(grid, 8, 0);
    lv_obj_set_style_pad_column(grid, 8, 0);
    lv_obj_set_size(grid, lv_pct(100), lv_pct(100));
    lv_obj_align(grid, LV_ALIGN_TOP_LEFT, 0, 20);
    lv_obj_set_layout(grid, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(grid, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    const auto entries = parse_launcher_entries(frame);
    const lv_coord_t tile_width = config_.display_width >= 400 ? lv_pct(31) : lv_pct(48);
    const lv_coord_t tile_height = config_.display_height >= 240 ? 68 : 58;

    for (const auto& entry : entries) {
        auto* tile = lv_obj_create(grid);
        lv_obj_remove_style_all(tile);
        lv_obj_add_style(tile, &style_tile_, 0);
        if (entry.focused) {
            lv_obj_add_style(tile, &style_tile_focus_, 0);
        } else if (entry.warning || entry.blocked) {
            lv_obj_add_style(tile, &style_tile_warning_, 0);
        }
        lv_obj_set_size(tile, tile_width, tile_height);

        auto* title = lv_label_create(tile);
        lv_obj_add_style(title, entry.focused ? &style_focus_text_ : &style_body_, 0);
        lv_label_set_text(title, entry.label.c_str());
        lv_obj_align(title, LV_ALIGN_TOP_LEFT, 0, 0);

        auto* secondary = lv_label_create(tile);
        if (entry.focused) {
            lv_obj_add_style(secondary, &style_focus_text_, 0);
        } else {
            lv_obj_add_style(secondary, &style_hint_, 0);
            if (entry.warning) {
                lv_obj_set_style_text_color(secondary, hex(entry.blocked ? kColorPolicy : kColorWarning), 0);
            } else if (!entry.secondary.empty()) {
                lv_obj_set_style_text_color(secondary, hex(kColorReady), 0);
            }
        }
        lv_label_set_long_mode(secondary, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(secondary, lv_pct(100));
        lv_label_set_text(secondary,
                          entry.secondary.empty() ? (entry.focused ? "Ready to open" : "Ready") :
                                                    entry.secondary.c_str());
        lv_obj_align(secondary, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    }
}

void ZephyrLvglShellUi::present_generic_surface(const shell::ShellPresentationFrame& frame) {
    update_top_bar(to_route_name(frame.surface),
                   frame.detail.empty() ? frame.context : frame.detail);
    update_softkeys("Open", "Select", "Back");
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

void ZephyrLvglShellUi::pump_once() {
    render_pending_ = false;
    (void)lv_timer_handler();
}

void ZephyrLvglShellUi::request_render() {
    render_pending_ = true;
    if (screen_ != nullptr) {
        lv_obj_invalidate(screen_);
    }
}

}  // namespace aegis::ports::zephyr
