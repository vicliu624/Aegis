#pragma once

#include "services/common/service_interfaces.hpp"

namespace aegis::services {

class InMemoryTextInputService : public ITextInputService {
public:
    explicit InMemoryTextInputService(TextInputState state);

    [[nodiscard]] TextInputState state() const override;
    [[nodiscard]] TextInputFocusState focus_state() const override;
    bool request_focus_for_session(const std::string& session_id) override;
    bool release_focus_for_session(const std::string& session_id) override;
    void assign_shell_focus() override;
    [[nodiscard]] std::string describe_backend() const override;

private:
    TextInputState state_;
    TextInputFocusState focus_;
};

}  // namespace aegis::services
