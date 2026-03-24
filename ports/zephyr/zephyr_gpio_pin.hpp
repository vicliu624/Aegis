#pragma once

#include <zephyr/device.h>
#include <zephyr/devicetree.h>

namespace aegis::ports::zephyr {

struct ZephyrGpioPinRef {
    const struct device* device {nullptr};
    int pin {-1};
};

inline ZephyrGpioPinRef resolve_gpio_pin(int global_pin) {
    if (global_pin < 0) {
        return {};
    }

#if DT_NODE_EXISTS(DT_NODELABEL(gpio1))
    if (global_pin >= 32) {
        return ZephyrGpioPinRef {
            .device = DEVICE_DT_GET(DT_NODELABEL(gpio1)),
            .pin = global_pin - 32,
        };
    }
#endif

    return ZephyrGpioPinRef {
        .device = DEVICE_DT_GET(DT_NODELABEL(gpio0)),
        .pin = global_pin,
    };
}

}  // namespace aegis::ports::zephyr
