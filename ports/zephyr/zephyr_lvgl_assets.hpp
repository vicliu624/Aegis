#pragma once

#include <string_view>

#include <lvgl.h>

namespace aegis::ports::zephyr {

extern const lv_image_dsc_t g_aegis_logo_image;
extern const lv_image_dsc_t g_settings_icon_image;
extern const lv_image_dsc_t g_gps_topbar_image;
extern const lv_image_dsc_t g_ble_topbar_image;
const lv_image_dsc_t* find_builtin_icon_image(std::string_view key);

}  // namespace aegis::ports::zephyr
