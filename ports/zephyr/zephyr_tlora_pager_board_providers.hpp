#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>

#include <zephyr/kernel.h>

#include "platform/logging/logger.hpp"
#include "ports/zephyr/zephyr_board_backend_config.hpp"

struct device;

namespace aegis::ports::zephyr {

enum class TloraPagerTransferParticipant : uint8_t {
    Unknown,
    Display,
    Radio,
    Storage,
    Nfc,
};

enum class TloraPagerBoardControlParticipant : uint8_t {
    Unknown,
    Expander,
    Keyboard,
    Power,
    StorageDetect,
};

enum class TloraPagerPowerChannel : uint8_t {
    Radio,
    HapticDriver,
    Gps,
    Nfc,
    SdCard,
    SpeakerAmp,
    Keyboard,
    GpioDomain,
};

class ZephyrTloraPagerTransferCoordinator {
public:
    ZephyrTloraPagerTransferCoordinator(platform::Logger& logger, ZephyrBoardBackendConfig config);

    void bind_gpio_device(const struct device* gpio_device);
    [[nodiscard]] bool prepare();
    [[nodiscard]] bool ready() const;
    [[nodiscard]] TloraPagerTransferParticipant owner() const;
    [[nodiscard]] std::string owner_name() const;
    [[nodiscard]] static std::string coordinator_name();
    [[nodiscard]] int with_participant(TloraPagerTransferParticipant participant,
                                       k_timeout_t timeout,
                                       std::string_view operation,
                                       const std::function<int()>& action) const;

private:
    [[nodiscard]] bool lock_for(TloraPagerTransferParticipant participant, k_timeout_t timeout) const;
    void unlock_for(TloraPagerTransferParticipant participant) const;

    platform::Logger& logger_;
    ZephyrBoardBackendConfig config_;
    const struct device* gpio_device_ {nullptr};
    mutable k_mutex mutex_ {};
    mutable TloraPagerTransferParticipant owner_ {TloraPagerTransferParticipant::Unknown};
    bool ready_ {false};
};

class ZephyrTloraPagerBoardControlProvider {
public:
    ZephyrTloraPagerBoardControlProvider(platform::Logger& logger, ZephyrBoardBackendConfig config);

    void bind_i2c_device(const struct device* i2c_device);
    [[nodiscard]] bool ready() const;
    [[nodiscard]] bool expander_ready() const;
    [[nodiscard]] bool keyboard_ready() const;
    [[nodiscard]] TloraPagerBoardControlParticipant owner() const;
    [[nodiscard]] std::string owner_name() const;
    [[nodiscard]] static std::string coordinator_name();
    [[nodiscard]] int with_participant(TloraPagerBoardControlParticipant participant,
                                       k_timeout_t timeout,
                                       std::string_view operation,
                                       const std::function<int()>& action) const;
    [[nodiscard]] int read_i2c_reg(uint16_t address, uint8_t reg, uint8_t* value) const;
    [[nodiscard]] int write_i2c_reg(uint16_t address, uint8_t reg, uint8_t value) const;
    [[nodiscard]] bool initialize_expander();
    [[nodiscard]] bool keyboard_pending_event_count(uint8_t& pending) const;
    [[nodiscard]] bool keyboard_read_event(uint8_t& raw_event) const;
    [[nodiscard]] bool probe_keyboard_controller();
    [[nodiscard]] bool set_power_enabled(TloraPagerPowerChannel channel, bool enabled) const;
    [[nodiscard]] bool peripheral_power_enabled(TloraPagerPowerChannel channel) const;
    void log_expander_state() const;
    [[nodiscard]] bool read_sd_card_detect_raw(bool& present) const;

private:
    [[nodiscard]] bool lock_for(TloraPagerBoardControlParticipant participant, k_timeout_t timeout) const;
    void unlock_for(TloraPagerBoardControlParticipant participant) const;

    platform::Logger& logger_;
    ZephyrBoardBackendConfig config_;
    const struct device* i2c_device_ {nullptr};
    mutable k_mutex mutex_ {};
    mutable TloraPagerBoardControlParticipant owner_ {TloraPagerBoardControlParticipant::Unknown};
    bool expander_ready_ {false};
    bool keyboard_ready_ {false};
    mutable std::array<bool, 8> power_channel_state_ {};
};

}  // namespace aegis::ports::zephyr
