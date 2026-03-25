#include "ports/zephyr/boards/tlora_pager/tlora_pager_board_providers.hpp"

#include <array>
#include <cerrno>
#include <optional>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>

#include "ports/zephyr/zephyr_gpio_pin.hpp"

namespace aegis::ports::zephyr {

namespace {

constexpr uint8_t kXl9555Address = 0x20;
constexpr uint8_t kXl9555RegInput0 = 0x00;
constexpr uint8_t kXl9555RegInput1 = 0x01;
constexpr uint8_t kXl9555RegOutput0 = 0x02;
constexpr uint8_t kXl9555RegOutput1 = 0x03;
constexpr uint8_t kXl9555RegPolarity0 = 0x04;
constexpr uint8_t kXl9555RegPolarity1 = 0x05;
constexpr uint8_t kXl9555RegConfig0 = 0x06;
constexpr uint8_t kXl9555RegConfig1 = 0x07;

constexpr uint8_t kTca8418Address = 0x34;
constexpr uint8_t kTca8418RegKeyLockEventCount = 0x03;
constexpr uint8_t kTca8418RegKeyEventA = 0x04;

constexpr uint8_t kExpandDrvEn = 0;
constexpr uint8_t kExpandAmpEn = 1;
constexpr uint8_t kExpandKbRst = 2;
constexpr uint8_t kExpandLoraEn = 3;
constexpr uint8_t kExpandGpsEn = 4;
constexpr uint8_t kExpandNfcEn = 5;
constexpr uint8_t kExpandGpsRst = 7;
constexpr uint8_t kExpandKbEn = 8;
constexpr uint8_t kExpandGpioEn = 9;
constexpr uint8_t kExpandSdDet = 10;
constexpr uint8_t kExpandSdPullen = 11;
constexpr uint8_t kExpandSdEn = 12;

std::string hex_byte(uint8_t value) {
    static constexpr char kHex[] = "0123456789ABCDEF";
    std::string result = "0x";
    result.push_back(kHex[(value >> 4) & 0x0F]);
    result.push_back(kHex[value & 0x0F]);
    return result;
}

std::string_view transfer_participant_name(TloraPagerTransferParticipant participant) {
    switch (participant) {
        case TloraPagerTransferParticipant::Unknown:
            return "unknown";
        case TloraPagerTransferParticipant::Display:
            return "display";
        case TloraPagerTransferParticipant::Radio:
            return "radio";
        case TloraPagerTransferParticipant::Storage:
            return "storage";
        case TloraPagerTransferParticipant::Nfc:
            return "nfc";
    }
    return "unknown";
}

std::string_view board_control_participant_name(TloraPagerBoardControlParticipant participant) {
    switch (participant) {
        case TloraPagerBoardControlParticipant::Unknown:
            return "unknown";
        case TloraPagerBoardControlParticipant::Expander:
            return "expander";
        case TloraPagerBoardControlParticipant::Keyboard:
            return "keyboard";
        case TloraPagerBoardControlParticipant::Power:
            return "power";
        case TloraPagerBoardControlParticipant::StorageDetect:
            return "storage-detect";
    }
    return "unknown";
}

std::optional<uint8_t> expander_pin_for_channel(TloraPagerPowerChannel channel) {
    switch (channel) {
        case TloraPagerPowerChannel::Radio:
            return kExpandLoraEn;
        case TloraPagerPowerChannel::HapticDriver:
            return kExpandDrvEn;
        case TloraPagerPowerChannel::Gps:
            return kExpandGpsEn;
        case TloraPagerPowerChannel::Nfc:
            return kExpandNfcEn;
        case TloraPagerPowerChannel::SdCard:
            return kExpandSdEn;
        case TloraPagerPowerChannel::SpeakerAmp:
            return kExpandAmpEn;
        case TloraPagerPowerChannel::Keyboard:
            return kExpandKbEn;
        case TloraPagerPowerChannel::GpioDomain:
            return kExpandGpioEn;
    }
    return std::nullopt;
}

constexpr std::size_t power_channel_index(TloraPagerPowerChannel channel) {
    return static_cast<std::size_t>(channel);
}

class TloraPagerXl9555 {
public:
    TloraPagerXl9555(const struct device* bus, uint16_t address)
        : bus_(bus), address_(address) {}

    [[nodiscard]] bool begin() const {
        uint8_t value = 0;
        return read_reg(kXl9555RegInput0, value);
    }

    [[nodiscard]] bool pin_mode(uint8_t pin, bool output) const {
        const uint8_t reg = pin >= 8 ? kXl9555RegConfig1 : kXl9555RegConfig0;
        const uint8_t bit = pin >= 8 ? static_cast<uint8_t>(pin - 8) : pin;
        return update_bit(reg, bit, output ? 0U : 1U);
    }

    [[nodiscard]] bool digital_write(uint8_t pin, bool high) const {
        const uint8_t reg = pin >= 8 ? kXl9555RegOutput1 : kXl9555RegOutput0;
        const uint8_t bit = pin >= 8 ? static_cast<uint8_t>(pin - 8) : pin;
        return update_bit(reg, bit, high ? 1U : 0U);
    }

    [[nodiscard]] bool digital_read(uint8_t pin, uint8_t& value) const {
        const uint8_t reg = pin >= 8 ? kXl9555RegInput1 : kXl9555RegInput0;
        const uint8_t bit = pin >= 8 ? static_cast<uint8_t>(pin - 8) : pin;
        uint8_t raw = 0;
        if (!read_reg(reg, raw)) {
            return false;
        }
        value = static_cast<uint8_t>((raw >> bit) & 0x01U);
        return true;
    }

    [[nodiscard]] bool read_reg(uint8_t reg, uint8_t& value) const {
        if (bus_ == nullptr || !device_is_ready(bus_)) {
            return false;
        }
        return i2c_reg_read_byte(bus_, address_, reg, &value) == 0;
    }

private:
    [[nodiscard]] bool write_reg(uint8_t reg, uint8_t value) const {
        if (bus_ == nullptr || !device_is_ready(bus_)) {
            return false;
        }
        return i2c_reg_write_byte(bus_, address_, reg, value) == 0;
    }

    [[nodiscard]] bool update_bit(uint8_t reg, uint8_t bit, uint8_t value) const {
        uint8_t current = 0;
        if (!read_reg(reg, current)) {
            return false;
        }
        const uint8_t mask = static_cast<uint8_t>(1U << bit);
        const uint8_t next = value != 0 ? static_cast<uint8_t>(current | mask)
                                        : static_cast<uint8_t>(current & ~mask);
        return write_reg(reg, next);
    }

    const struct device* bus_ {nullptr};
    uint16_t address_ {0};
};

}  // namespace

ZephyrTloraPagerTransferCoordinator::ZephyrTloraPagerTransferCoordinator(
    platform::Logger& logger,
    ZephyrBoardBackendConfig config)
    : logger_(logger), config_(std::move(config)) {
    k_mutex_init(&mutex_);
}

void ZephyrTloraPagerTransferCoordinator::bind_gpio_device(const struct device* gpio_device) {
    gpio_device_ = gpio_device;
}

bool ZephyrTloraPagerTransferCoordinator::prepare() {
    if (gpio_device_ == nullptr || !device_is_ready(gpio_device_)) {
        ready_ = false;
        return false;
    }

    for (const auto pin : config_.coordination_quiesce_pins) {
        if (pin < 0) {
            continue;
        }
        const auto pin_ref = resolve_gpio_pin(pin);
        if (pin_ref.device != nullptr && device_is_ready(pin_ref.device)) {
            const int cfg_rc = gpio_pin_configure(pin_ref.device, pin_ref.pin, GPIO_OUTPUT_ACTIVE);
            const int set_rc = gpio_pin_set(pin_ref.device, pin_ref.pin, 1);
            logger_.info("board",
                         "coordination quiesce pin=" + std::to_string(pin) +
                             " cfg_rc=" + std::to_string(cfg_rc) +
                             " set_rc=" + std::to_string(set_rc));
        }
    }

    if (config_.display_cs_pin >= 0) {
        const auto pin_ref = resolve_gpio_pin(config_.display_cs_pin);
        if (pin_ref.device != nullptr && device_is_ready(pin_ref.device)) {
            const int cfg_rc = gpio_pin_configure(pin_ref.device, pin_ref.pin, GPIO_OUTPUT_ACTIVE);
            const int set_rc = gpio_pin_set(pin_ref.device, pin_ref.pin, 1);
            logger_.info("board",
                         "display cs pin=" + std::to_string(config_.display_cs_pin) +
                             " cfg_rc=" + std::to_string(cfg_rc) +
                             " set_rc=" + std::to_string(set_rc));
        }
    }

    if (config_.display_dc_pin >= 0) {
        const auto pin_ref = resolve_gpio_pin(config_.display_dc_pin);
        if (pin_ref.device != nullptr && device_is_ready(pin_ref.device)) {
            const int cfg_rc = gpio_pin_configure(pin_ref.device, pin_ref.pin, GPIO_OUTPUT_ACTIVE);
            const int set_rc = gpio_pin_set(pin_ref.device, pin_ref.pin, 1);
            logger_.info("board",
                         "display dc pin=" + std::to_string(config_.display_dc_pin) +
                             " cfg_rc=" + std::to_string(cfg_rc) +
                             " set_rc=" + std::to_string(set_rc));
        }
    }

    k_sleep(K_MSEC(20));
    ready_ = true;
    return true;
}

bool ZephyrTloraPagerTransferCoordinator::ready() const {
    return ready_;
}

TloraPagerTransferParticipant ZephyrTloraPagerTransferCoordinator::owner() const {
    return owner_;
}

std::string ZephyrTloraPagerTransferCoordinator::owner_name() const {
    return std::string(transfer_participant_name(owner_));
}

std::string ZephyrTloraPagerTransferCoordinator::coordinator_name() {
    return "pager-transfer-coordinator";
}

int ZephyrTloraPagerTransferCoordinator::with_participant(TloraPagerTransferParticipant participant,
                                                          k_timeout_t timeout,
                                                          std::string_view operation,
                                                          const std::function<int()>& action) const {
    if (action == nullptr) {
        return -EINVAL;
    }
    if (!lock_for(participant, timeout)) {
        logger_.info("board",
                     "coordination denied coordinator=" + coordinator_name() +
                         " owner=" + std::string(transfer_participant_name(participant)) +
                         " op=" + std::string(operation));
        return -EAGAIN;
    }
    const int rc = action();
    logger_.info("board",
                 "coordination op coordinator=" + coordinator_name() +
                     " owner=" + std::string(transfer_participant_name(participant)) +
                     " op=" + std::string(operation) + " rc=" + std::to_string(rc));
    unlock_for(participant);
    return rc;
}

bool ZephyrTloraPagerTransferCoordinator::lock_for(TloraPagerTransferParticipant participant,
                                                   k_timeout_t timeout) const {
    if (!ready_) {
        return false;
    }
    if (k_mutex_lock(&mutex_, timeout) != 0) {
        return false;
    }
    owner_ = participant;
    logger_.info("board",
                 "coordination acquire coordinator=" + coordinator_name() +
                     " owner=" + std::string(transfer_participant_name(participant)));
    return true;
}

void ZephyrTloraPagerTransferCoordinator::unlock_for(TloraPagerTransferParticipant participant) const {
    logger_.info("board",
                 "coordination release coordinator=" + coordinator_name() +
                     " owner=" + std::string(transfer_participant_name(owner_)) +
                     " requested-by=" + std::string(transfer_participant_name(participant)));
    owner_ = TloraPagerTransferParticipant::Unknown;
    (void)k_mutex_unlock(&mutex_);
}

ZephyrTloraPagerBoardControlProvider::ZephyrTloraPagerBoardControlProvider(
    platform::Logger& logger,
    ZephyrBoardBackendConfig config)
    : logger_(logger), config_(std::move(config)) {
    k_mutex_init(&mutex_);
    power_channel_state_.fill(false);
}

void ZephyrTloraPagerBoardControlProvider::bind_i2c_device(const struct device* i2c_device) {
    i2c_device_ = i2c_device;
}

bool ZephyrTloraPagerBoardControlProvider::ready() const {
    return i2c_device_ != nullptr && device_is_ready(i2c_device_);
}

bool ZephyrTloraPagerBoardControlProvider::expander_ready() const {
    return expander_ready_;
}

bool ZephyrTloraPagerBoardControlProvider::keyboard_ready() const {
    return keyboard_ready_;
}

TloraPagerBoardControlParticipant ZephyrTloraPagerBoardControlProvider::owner() const {
    return owner_;
}

std::string ZephyrTloraPagerBoardControlProvider::owner_name() const {
    return std::string(board_control_participant_name(owner_));
}

std::string ZephyrTloraPagerBoardControlProvider::coordinator_name() {
    return "pager-board-control";
}

int ZephyrTloraPagerBoardControlProvider::with_participant(
    TloraPagerBoardControlParticipant participant,
    k_timeout_t timeout,
    std::string_view operation,
    const std::function<int()>& action) const {
    if (action == nullptr) {
        return -EINVAL;
    }
    if (!lock_for(participant, timeout)) {
        logger_.info("board",
                     "coordination denied coordinator=" + coordinator_name() +
                         " owner=" + std::string(board_control_participant_name(participant)) +
                         " op=" + std::string(operation));
        return -EAGAIN;
    }
    const int rc = action();
    logger_.info("board",
                 "coordination op coordinator=" + coordinator_name() +
                     " owner=" + std::string(board_control_participant_name(participant)) +
                     " op=" + std::string(operation) + " rc=" + std::to_string(rc));
    unlock_for(participant);
    return rc;
}

int ZephyrTloraPagerBoardControlProvider::read_i2c_reg(uint16_t address, uint8_t reg, uint8_t* value) const {
    if (value == nullptr || !ready()) {
        return -ENODEV;
    }
    int rc = -EAGAIN;
    (void)with_participant(TloraPagerBoardControlParticipant::Unknown,
                           K_MSEC(50),
                           "board-control.i2c.read",
                           [&]() {
                               rc = i2c_reg_read_byte(i2c_device_, address, reg, value);
                               return rc;
                           });
    return rc;
}

int ZephyrTloraPagerBoardControlProvider::write_i2c_reg(uint16_t address,
                                                        uint8_t reg,
                                                        uint8_t value) const {
    if (!ready()) {
        return -ENODEV;
    }
    int rc = -EAGAIN;
    (void)with_participant(TloraPagerBoardControlParticipant::Unknown,
                           K_MSEC(50),
                           "board-control.i2c.write",
                           [&]() {
                               rc = i2c_reg_write_byte(i2c_device_, address, reg, value);
                               return rc;
                           });
    return rc;
}

bool ZephyrTloraPagerBoardControlProvider::initialize_expander() {
    if (!ready()) {
        logger_.info("board", "xl9555 skipped: i2c unavailable");
        expander_ready_ = false;
        return false;
    }

    bool begin_ok = false;
    (void)with_participant(TloraPagerBoardControlParticipant::Expander,
                           K_MSEC(50),
                           "expander.begin",
                           [&]() {
                               const TloraPagerXl9555 expander(i2c_device_, kXl9555Address);
                               begin_ok = expander.begin();
                               return begin_ok ? 0 : -EIO;
                           });
    if (!begin_ok) {
        logger_.info("board", "xl9555 probe failed addr=" + hex_byte(kXl9555Address));
        expander_ready_ = false;
        return false;
    }

    (void)with_participant(TloraPagerBoardControlParticipant::Expander,
                           K_MSEC(50),
                           "expander.polarity_reset",
                           [&]() {
                               const TloraPagerXl9555 expander(i2c_device_, kXl9555Address);
                               uint8_t polarity = 0;
                               (void)expander.read_reg(kXl9555RegPolarity0, polarity);
                               (void)expander.read_reg(kXl9555RegPolarity1, polarity);
                               (void)i2c_reg_write_byte(i2c_device_, kXl9555Address, kXl9555RegPolarity0, 0x00);
                               (void)i2c_reg_write_byte(i2c_device_, kXl9555Address, kXl9555RegPolarity1, 0x00);
                               return 0;
                           });

    const std::array<uint8_t, 10> enable_lines {
        kExpandKbRst,  kExpandLoraEn, kExpandGpsEn, kExpandDrvEn, kExpandAmpEn,
        kExpandNfcEn,  kExpandGpsRst, kExpandKbEn,  kExpandGpioEn, kExpandSdEn,
    };

    bool enable_lines_ok = true;
    (void)with_participant(TloraPagerBoardControlParticipant::Expander,
                           K_MSEC(50),
                           "expander.enable_lines",
                           [&]() {
                               const TloraPagerXl9555 expander(i2c_device_, kXl9555Address);
                               for (const auto pin : enable_lines) {
                                   const bool mode_ok = expander.pin_mode(pin, true);
                                   const bool write_ok = expander.digital_write(pin, true);
                                   logger_.info("board",
                                                "xl9555 enable pin=" + std::to_string(pin) +
                                                    " mode=" +
                                                    (mode_ok ? std::string("ok") : std::string("failed")) +
                                                    " write=" +
                                                    (write_ok ? std::string("ok") : std::string("failed")));
                                   if (!mode_ok || !write_ok) {
                                       enable_lines_ok = false;
                                   }
                                   k_sleep(K_MSEC(1));
                               }
                               return enable_lines_ok ? 0 : -EIO;
                           });
    if (!enable_lines_ok) {
        expander_ready_ = false;
        logger_.info("board", "xl9555 enable lines failed addr=" + hex_byte(kXl9555Address));
        return false;
    }

    power_channel_state_.fill(false);
    power_channel_state_[power_channel_index(TloraPagerPowerChannel::HapticDriver)] = true;
    power_channel_state_[power_channel_index(TloraPagerPowerChannel::SpeakerAmp)] = true;
    power_channel_state_[power_channel_index(TloraPagerPowerChannel::Radio)] = true;
    power_channel_state_[power_channel_index(TloraPagerPowerChannel::Gps)] = true;
    power_channel_state_[power_channel_index(TloraPagerPowerChannel::Nfc)] = true;
    power_channel_state_[power_channel_index(TloraPagerPowerChannel::Keyboard)] = true;
    power_channel_state_[power_channel_index(TloraPagerPowerChannel::GpioDomain)] = true;
    power_channel_state_[power_channel_index(TloraPagerPowerChannel::SdCard)] = true;

    bool storage_detect_pins_ok = true;
    (void)with_participant(TloraPagerBoardControlParticipant::StorageDetect,
                           K_MSEC(50),
                           "storage.detect.configure",
                           [&]() {
                               const TloraPagerXl9555 expander(i2c_device_, kXl9555Address);
                               const bool pull_ok = expander.pin_mode(kExpandSdPullen, false);
                               const bool detect_ok = expander.pin_mode(kExpandSdDet, false);
                               storage_detect_pins_ok = pull_ok && detect_ok;
                               return storage_detect_pins_ok ? 0 : -EIO;
                           });
    if (!storage_detect_pins_ok) {
        expander_ready_ = false;
        logger_.info("board", "xl9555 storage detect configuration failed addr=" + hex_byte(kXl9555Address));
        return false;
    }

    expander_ready_ = true;
    logger_.info("board", "xl9555 ready addr=" + hex_byte(kXl9555Address));
    return true;
}

bool ZephyrTloraPagerBoardControlProvider::keyboard_pending_event_count(uint8_t& pending) const {
    pending = 0;
    if (!keyboard_ready_) {
        return false;
    }
    int rc = -EAGAIN;
    (void)with_participant(TloraPagerBoardControlParticipant::Keyboard,
                           K_MSEC(50),
                           "keyboard.pending_event_count",
                           [&]() {
                               rc = i2c_reg_read_byte(i2c_device_,
                                                      kTca8418Address,
                                                      kTca8418RegKeyLockEventCount,
                                                      &pending);
                               return rc;
                           });
    return rc == 0;
}

bool ZephyrTloraPagerBoardControlProvider::keyboard_read_event(uint8_t& raw_event) const {
    raw_event = 0;
    if (!keyboard_ready_) {
        return false;
    }
    int rc = -EAGAIN;
    (void)with_participant(TloraPagerBoardControlParticipant::Keyboard,
                           K_MSEC(50),
                           "keyboard.read_event",
                           [&]() {
                               rc = i2c_reg_read_byte(i2c_device_,
                                                      kTca8418Address,
                                                      kTca8418RegKeyEventA,
                                                      &raw_event);
                               return rc;
                           });
    return rc == 0;
}

bool ZephyrTloraPagerBoardControlProvider::probe_keyboard_controller() {
    if (!ready()) {
        keyboard_ready_ = false;
        return false;
    }
    uint8_t pending = 0;
    int rc = -EAGAIN;
    (void)with_participant(TloraPagerBoardControlParticipant::Keyboard,
                           K_MSEC(50),
                           "keyboard.probe",
                           [&]() {
                               rc = i2c_reg_read_byte(i2c_device_,
                                                      kTca8418Address,
                                                      kTca8418RegKeyLockEventCount,
                                                      &pending);
                               return rc;
                           });
    keyboard_ready_ = rc == 0;
    logger_.info("board",
                 "tca8418 probe " + std::string(keyboard_ready_ ? "ok" : "failed") +
                     " addr=" + hex_byte(kTca8418Address) +
                     (keyboard_ready_ ? " events=" + hex_byte(pending) : ""));
    return keyboard_ready_;
}

bool ZephyrTloraPagerBoardControlProvider::set_power_enabled(TloraPagerPowerChannel channel,
                                                             bool enabled) const {
    if (!expander_ready_ || !ready()) {
        return false;
    }
    const auto pin = expander_pin_for_channel(channel);
    if (!pin.has_value()) {
        return false;
    }
    bool ok = false;
    const int rc = with_participant(TloraPagerBoardControlParticipant::Power,
                                    K_MSEC(50),
                                    "power.set_channel",
                                    [&]() {
                                        const TloraPagerXl9555 expander(i2c_device_, kXl9555Address);
                                        ok = expander.digital_write(*pin, enabled);
                                        return ok ? 0 : -EIO;
                                    });
    if (rc != 0) {
        ok = false;
    }
    if (ok && power_channel_index(channel) < power_channel_state_.size()) {
        power_channel_state_[power_channel_index(channel)] = enabled;
    }
    logger_.info("board",
                 "power channel=" + std::to_string(static_cast<int>(channel)) +
                     " enabled=" + std::string(enabled ? "1" : "0") +
                     " rc=" + std::string(ok ? "ok" : "failed"));
    return ok;
}

bool ZephyrTloraPagerBoardControlProvider::peripheral_power_enabled(TloraPagerPowerChannel channel) const {
    const auto index = power_channel_index(channel);
    return index < power_channel_state_.size() ? power_channel_state_[index] : false;
}

void ZephyrTloraPagerBoardControlProvider::log_expander_state() const {
    if (!expander_ready_ || !ready()) {
        return;
    }
    const TloraPagerXl9555 expander(i2c_device_, kXl9555Address);
    uint8_t out0 = 0;
    uint8_t out1 = 0;
    uint8_t cfg0 = 0;
    uint8_t cfg1 = 0;
    uint8_t in0 = 0;
    uint8_t in1 = 0;
    (void)expander.read_reg(kXl9555RegOutput0, out0);
    (void)expander.read_reg(kXl9555RegOutput1, out1);
    (void)expander.read_reg(kXl9555RegConfig0, cfg0);
    (void)expander.read_reg(kXl9555RegConfig1, cfg1);
    (void)expander.read_reg(kXl9555RegInput0, in0);
    (void)expander.read_reg(kXl9555RegInput1, in1);
    logger_.info("board",
                 "xl9555 state out0=" + hex_byte(out0) + " out1=" + hex_byte(out1) +
                     " cfg0=" + hex_byte(cfg0) + " cfg1=" + hex_byte(cfg1) +
                     " in0=" + hex_byte(in0) + " in1=" + hex_byte(in1));
}

bool ZephyrTloraPagerBoardControlProvider::read_sd_card_detect_raw(bool& present) const {
    present = false;
    if (!expander_ready_ || !ready()) {
        return false;
    }
    uint8_t value = 1;
    bool ok = false;
    const int rc = with_participant(TloraPagerBoardControlParticipant::StorageDetect,
                                    K_MSEC(50),
                                    "storage.detect.read",
                                    [&]() {
                                        const TloraPagerXl9555 expander(i2c_device_, kXl9555Address);
                                        ok = expander.digital_read(kExpandSdDet, value);
                                        return ok ? 0 : -EIO;
                                    });
    if (rc != 0 || !ok) {
        return false;
    }
    present = value == 0;
    return true;
}

bool ZephyrTloraPagerBoardControlProvider::lock_for(TloraPagerBoardControlParticipant participant,
                                                    k_timeout_t timeout) const {
    if (!ready()) {
        return false;
    }
    if (k_mutex_lock(&mutex_, timeout) != 0) {
        return false;
    }
    owner_ = participant;
    logger_.info("board",
                 "coordination acquire coordinator=" + coordinator_name() +
                     " owner=" + std::string(board_control_participant_name(participant)));
    return true;
}

void ZephyrTloraPagerBoardControlProvider::unlock_for(TloraPagerBoardControlParticipant participant) const {
    logger_.info("board",
                 "coordination release coordinator=" + coordinator_name() +
                     " owner=" + std::string(board_control_participant_name(owner_)) +
                     " requested-by=" + std::string(board_control_participant_name(participant)));
    owner_ = TloraPagerBoardControlParticipant::Unknown;
    (void)k_mutex_unlock(&mutex_);
}

}  // namespace aegis::ports::zephyr
