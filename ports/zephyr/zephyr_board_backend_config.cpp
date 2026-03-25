#include "ports/zephyr/zephyr_board_backend_config.hpp"

#include <zephyr/device.h>
#include <zephyr/devicetree.h>

namespace aegis::ports::zephyr {

namespace {

#if DT_NODE_EXISTS(DT_ALIAS(lora0))
constexpr char kRadioDeviceName[] = DEVICE_DT_NAME(DT_ALIAS(lora0));
#else
constexpr char kRadioDeviceName[] = "lora0";
#endif

#if DT_NODE_EXISTS(DT_ALIAS(gps0))
constexpr char kGpsDeviceName[] = DEVICE_DT_NAME(DT_ALIAS(gps0));
#else
constexpr char kGpsDeviceName[] = "gps0";
#endif

#if DT_NODE_EXISTS(DT_ALIAS(audioout0))
constexpr char kAudioOutputDeviceName[] = DEVICE_DT_NAME(DT_ALIAS(audioout0));
#else
constexpr char kAudioOutputDeviceName[] = "speaker0";
#endif

#if DT_NODE_EXISTS(DT_ALIAS(audioin0))
constexpr char kAudioInputDeviceName[] = DEVICE_DT_NAME(DT_ALIAS(audioin0));
#else
constexpr char kAudioInputDeviceName[] = "microphone0";
#endif

#if DT_NODE_EXISTS(DT_ALIAS(hostlink0))
constexpr char kHostlinkDeviceName[] = DEVICE_DT_NAME(DT_ALIAS(hostlink0));
#else
constexpr char kHostlinkDeviceName[] = "CDC_ACM_0";
#endif

#if DT_NODE_EXISTS(DT_ALIAS(inputkbd0))
constexpr char kKeyboardInputDeviceName[] = DEVICE_DT_NAME(DT_ALIAS(inputkbd0));
#else
constexpr char kKeyboardInputDeviceName[] = "tca8418";
#endif

#if DT_NODE_EXISTS(DT_ALIAS(inputkbdmap0))
constexpr char kTextInputDeviceName[] = DEVICE_DT_NAME(DT_ALIAS(inputkbdmap0));
#else
constexpr char kTextInputDeviceName[] = "kbdmap0";
#endif

#if DT_NODE_EXISTS(DT_ALIAS(rotary0))
constexpr char kRotaryInputDeviceName[] = DEVICE_DT_NAME(DT_ALIAS(rotary0));
#else
constexpr char kRotaryInputDeviceName[] = "rotary";
#endif

#if DT_NODE_EXISTS(DT_ALIAS(display0))
constexpr char kDisplayDeviceName[] = DEVICE_DT_NAME(DT_ALIAS(display0));
#else
constexpr char kDisplayDeviceName[] = "st7796";
#endif

#if DT_NODE_EXISTS(DT_ALIAS(rtc0))
constexpr char kRtcDeviceName[] = DEVICE_DT_NAME(DT_ALIAS(rtc0));
#else
constexpr char kRtcDeviceName[] = "pcf85063";
#endif

#if DT_NODE_EXISTS(DT_ALIAS(battery0))
constexpr char kBatteryDeviceName[] = DEVICE_DT_NAME(DT_ALIAS(battery0));
#else
constexpr char kBatteryDeviceName[] = "bq27220";
#endif

#if DT_NODE_EXISTS(DT_ALIAS(nfc0))
constexpr char kNfcDeviceName[] = DEVICE_DT_NAME(DT_ALIAS(nfc0));
#else
constexpr char kNfcDeviceName[] = "st25r3916";
#endif

}  // namespace

ZephyrBoardBackendConfig make_device_a_backend_config() {
    return ZephyrBoardBackendConfig {
        .backend_id = "zephyr_device_a",
        .target_board = "bootstrap",
        .profile_device_id = "device-a-handheld",
        .board_family = "board_x",
        .runtime_family = ZephyrBoardRuntimeFamily::Generic,
        .display_backend_family = ZephyrBoardDisplayBackendFamily::Generic,
        .input_backend_family = ZephyrBoardInputBackendFamily::Generic,
        .display_width = 320,
        .display_height = 240,
        .display_touch = false,
        .display_prefer_manual_spi = false,
        .display_prefer_raw_mipi = false,
        .display_boot_log_mirror = false,
        .display_smoke_test = false,
        .display_swap_xy = false,
        .display_mirror_x = false,
        .display_mirror_y = false,
        .display_x_gap = 0,
        .display_y_gap = 0,
        .display_madctl = 0x28,
        .display_layout_class = "compact",
        .keyboard_input = false,
        .pointer_input = false,
        .joystick_input = true,
        .primary_input_name = "joystick",
        .removable_storage = false,
        .radio_device_name = kRadioDeviceName,
        .gps_device_name = kGpsDeviceName,
        .audio_output_device_name = kAudioOutputDeviceName,
        .audio_input_device_name = kAudioInputDeviceName,
        .hostlink_device_name = kHostlinkDeviceName,
        .hostlink_bridge_name = "no bridge",
        .keyboard_device_name = "",
        .text_input_device_name = "",
        .rotary_device_name = kRotaryInputDeviceName,
        .display_device_name = kDisplayDeviceName,
        .rtc_device_name = kRtcDeviceName,
        .battery_device_name = kBatteryDeviceName,
        .nfc_device_name = kNfcDeviceName,
        .display_backlight_pin = -1,
        .display_dc_pin = -1,
        .display_cs_pin = -1,
        .keyboard_backlight_pin = -1,
        .keyboard_irq_pin = -1,
        .keyboard_rows = 0,
        .keyboard_columns = 0,
        .keyboard_i2c_address = 0,
        .shell_select_row = -1,
        .shell_select_col = -1,
        .shell_back_row = -1,
        .shell_back_col = -1,
        .shell_menu_row = -1,
        .shell_menu_col = -1,
        .shell_settings_row = -1,
        .shell_settings_col = -1,
        .shell_notifications_row = -1,
        .shell_notifications_col = -1,
        .rotary_a_pin = -1,
        .rotary_b_pin = -1,
        .rotary_center_pin = -1,
        .touch_irq_pin = -1,
        .trackball_up_pin = -1,
        .trackball_down_pin = -1,
        .trackball_left_pin = -1,
        .trackball_right_pin = -1,
        .trackball_click_pin = -1,
        .coordination_quiesce_pins = {-1, -1, -1, -1, -1, -1},
        .notes = "bootstrap Zephyr device A profile with joystick-oriented shell surface",
    };
}

ZephyrBoardBackendConfig make_device_b_backend_config() {
    return ZephyrBoardBackendConfig {
        .backend_id = "zephyr_device_b",
        .target_board = "bootstrap",
        .profile_device_id = "device-b-messenger",
        .board_family = "board_y",
        .runtime_family = ZephyrBoardRuntimeFamily::Generic,
        .display_backend_family = ZephyrBoardDisplayBackendFamily::Generic,
        .input_backend_family = ZephyrBoardInputBackendFamily::Generic,
        .display_width = 480,
        .display_height = 320,
        .display_touch = false,
        .display_prefer_manual_spi = false,
        .display_prefer_raw_mipi = false,
        .display_boot_log_mirror = false,
        .display_smoke_test = false,
        .display_swap_xy = false,
        .display_mirror_x = false,
        .display_mirror_y = false,
        .display_x_gap = 0,
        .display_y_gap = 0,
        .display_madctl = 0x28,
        .display_layout_class = "wide",
        .keyboard_input = true,
        .pointer_input = false,
        .joystick_input = false,
        .primary_input_name = "keyboard",
        .removable_storage = true,
        .radio_device_name = kRadioDeviceName,
        .gps_device_name = kGpsDeviceName,
        .audio_output_device_name = kAudioOutputDeviceName,
        .audio_input_device_name = kAudioInputDeviceName,
        .hostlink_device_name = kHostlinkDeviceName,
        .hostlink_bridge_name = "zephyr-console-bridge",
        .keyboard_device_name = kKeyboardInputDeviceName,
        .text_input_device_name = kTextInputDeviceName,
        .rotary_device_name = "",
        .display_device_name = kDisplayDeviceName,
        .rtc_device_name = kRtcDeviceName,
        .battery_device_name = kBatteryDeviceName,
        .nfc_device_name = kNfcDeviceName,
        .display_backlight_pin = -1,
        .display_dc_pin = -1,
        .display_cs_pin = -1,
        .keyboard_backlight_pin = -1,
        .keyboard_irq_pin = -1,
        .keyboard_rows = 0,
        .keyboard_columns = 0,
        .keyboard_i2c_address = 0,
        .shell_select_row = -1,
        .shell_select_col = -1,
        .shell_back_row = -1,
        .shell_back_col = -1,
        .shell_menu_row = -1,
        .shell_menu_col = -1,
        .shell_settings_row = -1,
        .shell_settings_col = -1,
        .shell_notifications_row = -1,
        .shell_notifications_col = -1,
        .rotary_a_pin = -1,
        .rotary_b_pin = -1,
        .rotary_center_pin = -1,
        .touch_irq_pin = -1,
        .trackball_up_pin = -1,
        .trackball_down_pin = -1,
        .trackball_left_pin = -1,
        .trackball_right_pin = -1,
        .trackball_click_pin = -1,
        .coordination_quiesce_pins = {-1, -1, -1, -1, -1, -1},
        .notes = "bootstrap Zephyr device B profile with keyboard-dominant shell surface",
    };
}

ZephyrBoardBackendConfig make_tlora_pager_sx1262_backend_config() {
    return ZephyrBoardBackendConfig {
        .backend_id = "zephyr_tlora_pager_sx1262",
        .target_board = "esp32s3_devkitc/esp32s3/procpu + lilygo_tlora_pager_sx1262.overlay",
        .profile_device_id = "lilygo-tlora-pager-sx1262",
        .board_family = "lilygo_tlora_pager",
        .runtime_family = ZephyrBoardRuntimeFamily::TloraPager,
        .display_backend_family = ZephyrBoardDisplayBackendFamily::Pager,
        .input_backend_family = ZephyrBoardInputBackendFamily::PagerDirect,
        .display_width = 480,
        .display_height = 222,
        .display_touch = false,
        .display_prefer_manual_spi = true,
        .display_prefer_raw_mipi = true,
        .display_boot_log_mirror = false,
        .display_smoke_test = false,
        .display_swap_xy = false,
        .display_mirror_x = false,
        .display_mirror_y = false,
        .display_x_gap = 0,
        .display_y_gap = 49,
        .display_madctl = 0xE8,
        .display_layout_class = "pager-landscape",
        .keyboard_input = true,
        .pointer_input = false,
        .joystick_input = true,
        .primary_input_name = "keyboard+rotary",
        .removable_storage = true,
        .radio_device_name = kRadioDeviceName,
        .gps_device_name = kGpsDeviceName,
        .audio_output_device_name = kAudioOutputDeviceName,
        .audio_input_device_name = kAudioInputDeviceName,
        .hostlink_device_name = kHostlinkDeviceName,
        .hostlink_bridge_name = "usb-cdc-hostlink",
        .keyboard_device_name = kKeyboardInputDeviceName,
        .text_input_device_name = kTextInputDeviceName,
        .rotary_device_name = kRotaryInputDeviceName,
        .display_device_name = kDisplayDeviceName,
        .rtc_device_name = kRtcDeviceName,
        .battery_device_name = kBatteryDeviceName,
        .nfc_device_name = kNfcDeviceName,
        .display_backlight_pin = 42,
        .display_dc_pin = 37,
        .display_cs_pin = 38,
        .keyboard_backlight_pin = 46,
        .keyboard_irq_pin = 6,
        .keyboard_rows = 4,
        .keyboard_columns = 10,
        .keyboard_i2c_address = 0x34,
        .shell_select_row = 1,
        .shell_select_col = 9,
        .shell_back_row = 2,
        .shell_back_col = 9,
        .shell_menu_row = 2,
        .shell_menu_col = 0,
        .shell_settings_row = 2,
        .shell_settings_col = 8,
        .shell_notifications_row = 0,
        .shell_notifications_col = 9,
        .rotary_a_pin = 40,
        .rotary_b_pin = 41,
        .rotary_center_pin = 7,
        .touch_irq_pin = -1,
        .trackball_up_pin = -1,
        .trackball_down_pin = -1,
        .trackball_left_pin = -1,
        .trackball_right_pin = -1,
        .trackball_click_pin = -1,
        .coordination_quiesce_pins = {21, 36, 39, 47, -1, -1},
        .notes =
            "T-LoRa-Pager SX1262 profile mapped from hardware doc: ST7796 480x222 display, "
            "TCA8418 keyboard, rotary encoder, UBlox GPS, SX1262 LoRa, ST25R3916 NFC, "
            "ES8311 codec, XL9555 power gating, BQ25896/BQ27220 power path; "
            "display addressing follows trail-mate runtime rotation-0 contract (MADCTL 0xE8, y-gap 49)",
    };
}

ZephyrBoardBackendConfig make_tdeck_sx1262_backend_config() {
    return ZephyrBoardBackendConfig {
        .backend_id = "zephyr_tdeck_sx1262",
        .target_board = "esp32s3_devkitc/esp32s3/procpu + lilygo_tdeck.overlay",
        .profile_device_id = "lilygo-tdeck-sx1262",
        .board_family = "lilygo_tdeck",
        .runtime_family = ZephyrBoardRuntimeFamily::TDeck,
        .display_backend_family = ZephyrBoardDisplayBackendFamily::TDeck,
        .input_backend_family = ZephyrBoardInputBackendFamily::TDeckDirect,
        .display_width = 320,
        .display_height = 240,
        .display_touch = true,
        .display_prefer_manual_spi = true,
        .display_prefer_raw_mipi = false,
        .display_boot_log_mirror = true,
        .display_smoke_test = true,
        .display_swap_xy = false,
        .display_mirror_x = false,
        .display_mirror_y = false,
        .display_x_gap = 0,
        .display_y_gap = 0,
        .display_madctl = 0x60,
        .display_layout_class = "tdeck-landscape",
        .keyboard_input = true,
        .pointer_input = true,
        .joystick_input = true,
        .primary_input_name = "keyboard+trackball+touch",
        .removable_storage = true,
        .radio_device_name = kRadioDeviceName,
        .gps_device_name = kGpsDeviceName,
        .audio_output_device_name = kAudioOutputDeviceName,
        .audio_input_device_name = kAudioInputDeviceName,
        .hostlink_device_name = kHostlinkDeviceName,
        .hostlink_bridge_name = "usb-cdc-hostlink",
        .keyboard_device_name = "tdeck-keyboard",
        .text_input_device_name = "tdeck-keyboard",
        .rotary_device_name = "tdeck-trackball",
        .display_device_name = "st7789",
        .rtc_device_name = kRtcDeviceName,
        .battery_device_name = "axp2101",
        .nfc_device_name = "",
        .battery_adc_pin = 4,
        .display_backlight_pin = 42,
        .display_dc_pin = 11,
        .display_cs_pin = 12,
        .keyboard_backlight_pin = -1,
        .keyboard_irq_pin = -1,
        .keyboard_rows = 0,
        .keyboard_columns = 0,
        .keyboard_i2c_address = 0x55,
        .shell_select_row = -1,
        .shell_select_col = -1,
        .shell_back_row = -1,
        .shell_back_col = -1,
        .shell_menu_row = -1,
        .shell_menu_col = -1,
        .shell_settings_row = -1,
        .shell_settings_col = -1,
        .shell_notifications_row = -1,
        .shell_notifications_col = -1,
        .rotary_a_pin = -1,
        .rotary_b_pin = -1,
        .rotary_center_pin = -1,
        .touch_irq_pin = 16,
        .trackball_up_pin = 3,
        .trackball_down_pin = 15,
        .trackball_left_pin = 1,
        .trackball_right_pin = 2,
        .trackball_click_pin = 0,
        .coordination_quiesce_pins = {9, 12, 39, -1, -1, -1},
        .notes =
            "T-Deck SX1262 profile mapped from trail-mate and LilyGo references: ST7789 320x240 display, "
            "AXP2101 PMU on I2C, GT911 touch, I2C keyboard at 0x55, trackball GPIO navigation, "
            "SX1262 LoRa, GPS on UART1, shared SPI across display/radio/SD, early board power enable on GPIO10",
    };
}

}  // namespace aegis::ports::zephyr
