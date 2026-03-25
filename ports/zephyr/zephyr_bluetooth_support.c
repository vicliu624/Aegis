#include <zephyr/autoconf.h>

#if defined(CONFIG_BT) && !defined(CONFIG_BT_HOST_CRYPTO)

#include <errno.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/crypto.h>
#include <esp_random.h>

int bt_rand(void *buf, size_t len)
{
    if (buf == NULL && len != 0U) {
        return -EINVAL;
    }

    if (len == 0U) {
        return 0;
    }

    uint8_t *bytes = buf;
    size_t offset = 0U;

    while (offset < len) {
        const uint32_t value = esp_random();
        const size_t chunk = (len - offset) < sizeof(value) ? (len - offset) : sizeof(value);

        for (size_t index = 0; index < chunk; ++index) {
            bytes[offset + index] = (uint8_t)((value >> (index * 8U)) & 0xFFU);
        }

        offset += chunk;
    }

    return 0;
}

#endif
