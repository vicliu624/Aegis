#pragma once

#include <string>

#include <zephyr/kernel.h>

#include "ports/zephyr/zephyr_board_backend_config.hpp"
#include "services/common/service_interfaces.hpp"

namespace aegis::services {

class ZephyrTextInputService : public ITextInputService {
public:
    explicit ZephyrTextInputService(ports::zephyr::ZephyrBoardBackendConfig config);

    [[nodiscard]] TextInputState state() const override;
    [[nodiscard]] TextInputFocusState focus_state() const override;
    bool request_focus_for_session(const std::string& session_id) override;
    bool release_focus_for_session(const std::string& session_id) override;
    void assign_shell_focus() override;
    [[nodiscard]] std::string describe_backend() const override;

    void handle_key_event(const std::string& source_name, uint16_t key_code, bool pressed);

private:
    [[nodiscard]] bool matches_source(const std::string& source_name) const;
    void update_modifier(uint16_t key_code, bool pressed);
    void update_text(uint16_t key_code, bool pressed);
    [[nodiscard]] char translate_plain(uint16_t key_code) const;
    [[nodiscard]] char translate_symbol(uint16_t key_code) const;

    ports::zephyr::ZephyrBoardBackendConfig config_;
    mutable k_mutex mutex_;
    TextInputState state_;
    TextInputFocusState focus_;
    bool shift_pressed_ {false};
    bool symbol_pressed_ {false};
    bool alt_pressed_ {false};
    bool caps_lock_ {false};
};

}  // namespace aegis::services
