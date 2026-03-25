#pragma once

#include <array>
#include <string>

namespace aegis::ports::zephyr {

struct ZephyrBoardBackendConfig {
    std::string backend_id;
    std::string target_board;
    std::string profile_device_id;
    std::string board_family;
    int display_width {0};
    int display_height {0};
    bool display_touch {false};
    bool display_prefer_manual_spi {false};
    bool display_prefer_raw_mipi {false};
    bool display_boot_log_mirror {false};
    bool display_smoke_test {false};
    bool display_swap_xy {false};
    bool display_mirror_x {false};
    bool display_mirror_y {false};
    int display_x_gap {0};
    int display_y_gap {0};
    int display_madctl {0x28};
    std::string display_layout_class;
    bool keyboard_input {false};
    bool pointer_input {false};
    bool joystick_input {false};
    std::string primary_input_name;
    bool removable_storage {false};
    std::string radio_device_name;
    std::string gps_device_name;
    std::string audio_output_device_name;
    std::string audio_input_device_name;
    std::string hostlink_device_name;
    std::string hostlink_bridge_name;
    std::string keyboard_device_name;
    std::string text_input_device_name;
    std::string rotary_device_name;
    std::string display_device_name;
    std::string rtc_device_name;
    std::string battery_device_name;
    std::string nfc_device_name;
    int display_backlight_pin {-1};
    int display_dc_pin {-1};
    int display_cs_pin {-1};
    int keyboard_backlight_pin {-1};
    int keyboard_irq_pin {-1};
    int keyboard_rows {0};
    int keyboard_columns {0};
    int keyboard_i2c_address {0};
    int shell_select_row {-1};
    int shell_select_col {-1};
    int shell_back_row {-1};
    int shell_back_col {-1};
    int shell_menu_row {-1};
    int shell_menu_col {-1};
    int shell_settings_row {-1};
    int shell_settings_col {-1};
    int shell_notifications_row {-1};
    int shell_notifications_col {-1};
    int rotary_a_pin {-1};
    int rotary_b_pin {-1};
    int rotary_center_pin {-1};
    std::array<int, 6> coordination_quiesce_pins {-1, -1, -1, -1, -1, -1};
    std::string notes;
};

[[nodiscard]] ZephyrBoardBackendConfig make_device_a_backend_config();
[[nodiscard]] ZephyrBoardBackendConfig make_device_b_backend_config();
[[nodiscard]] ZephyrBoardBackendConfig make_tlora_pager_sx1262_backend_config();

}  // namespace aegis::ports::zephyr
