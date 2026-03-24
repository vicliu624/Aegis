/*
 * Board-backed TCA8418 keypad input driver for the Aegis Zephyr port.
 *
 * This keeps the keypad controller in the explicit device adaptation layer
 * while reporting generic Zephyr input events upward.
 */

#define DT_DRV_COMPAT ti_tca8418_keypad

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(input_tca8418_keypad, CONFIG_INPUT_LOG_LEVEL);

#define TCA8418_REG_CFG 0x01
#define TCA8418_REG_INT_STAT 0x02
#define TCA8418_REG_KEY_LCK_EC 0x03
#define TCA8418_REG_KEY_EVENT_A 0x04
#define TCA8418_REG_GPIO_INT_STAT_1 0x11
#define TCA8418_REG_GPIO_INT_STAT_2 0x12
#define TCA8418_REG_GPIO_INT_STAT_3 0x13
#define TCA8418_REG_KP_GPIO_1 0x1D
#define TCA8418_REG_KP_GPIO_2 0x1E
#define TCA8418_REG_KP_GPIO_3 0x1F

#define TCA8418_CFG_KE_IEN BIT(0)
#define TCA8418_CFG_GPI_IEN BIT(1)
#define TCA8418_INT_STAT_K_INT BIT(0)
#define TCA8418_INT_STAT_GPI_INT BIT(1)
#define TCA8418_EVENT_COUNT_MASK 0x0F
#define TCA8418_RELEASE_FLAG BIT(7)
#define TCA8418_GPIO_EVENT_BASE 97U

struct tca8418_keypad_config {
	struct i2c_dt_spec bus;
	struct gpio_dt_spec irq;
	uint8_t rows;
	uint8_t cols;
	uint16_t poll_interval_ms;
	bool has_irq;
};

struct tca8418_keypad_data {
	const struct device *dev;
	struct k_work_delayable poll_work;
	struct gpio_callback irq_cb;
	bool irq_configured;
};

static uint8_t tca8418_matrix_mask(uint8_t count)
{
	if (count == 0U) {
		return 0U;
	}

	if (count >= 8U) {
		return 0xFFU;
	}

	return (uint8_t)(BIT(count) - 1U);
}

static int tca8418_read_reg(const struct tca8418_keypad_config *cfg, uint8_t reg, uint8_t *value)
{
	return i2c_reg_read_byte_dt(&cfg->bus, reg, value);
}

static int tca8418_write_reg(const struct tca8418_keypad_config *cfg, uint8_t reg, uint8_t value)
{
	return i2c_reg_write_byte_dt(&cfg->bus, reg, value);
}

static int tca8418_pending_event_count(const struct tca8418_keypad_config *cfg)
{
	uint8_t status = 0U;
	int ret = tca8418_read_reg(cfg, TCA8418_REG_KEY_LCK_EC, &status);

	if (ret != 0) {
		return ret;
	}

	return status & TCA8418_EVENT_COUNT_MASK;
}

static void tca8418_report_matrix_event(const struct device *dev, uint8_t raw_event)
{
	const struct tca8418_keypad_config *cfg = dev->config;
	const bool pressed = (raw_event & TCA8418_RELEASE_FLAG) == 0U;
	const uint8_t code = raw_event & 0x7FU;

	if (code == 0U || code >= TCA8418_GPIO_EVENT_BASE) {
		return;
	}

	const uint8_t index = code - 1U;
	const uint8_t row = index / cfg->cols;
	const uint8_t col = index % cfg->cols;

	if (row >= cfg->rows || col >= cfg->cols) {
		LOG_WRN("%s event out of range raw=%u row=%u col=%u", dev->name, code, row, col);
		return;
	}

	input_report_abs(dev, INPUT_ABS_X, col, false, K_FOREVER);
	input_report_abs(dev, INPUT_ABS_Y, row, false, K_FOREVER);
	input_report_key(dev, INPUT_BTN_TOUCH, pressed, true, K_FOREVER);
}

static void tca8418_clear_gpio_interrupts(const struct tca8418_keypad_config *cfg, uint8_t int_stat)
{
	if ((int_stat & TCA8418_INT_STAT_GPI_INT) == 0U) {
		return;
	}

	uint8_t value;
	(void)tca8418_read_reg(cfg, TCA8418_REG_GPIO_INT_STAT_1, &value);
	(void)tca8418_read_reg(cfg, TCA8418_REG_GPIO_INT_STAT_2, &value);
	(void)tca8418_read_reg(cfg, TCA8418_REG_GPIO_INT_STAT_3, &value);
	(void)tca8418_write_reg(cfg, TCA8418_REG_INT_STAT, TCA8418_INT_STAT_GPI_INT);
}

static void tca8418_poll_once(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct tca8418_keypad_data *data =
		CONTAINER_OF(dwork, struct tca8418_keypad_data, poll_work);
	const struct device *dev = data->dev;
	const struct tca8418_keypad_config *cfg = dev->config;

	while (true) {
		int pending = tca8418_pending_event_count(cfg);

		if (pending < 0) {
			LOG_ERR("%s failed reading event count: %d", dev->name, pending);
			break;
		}

		if (pending == 0) {
			break;
		}

		uint8_t raw_event = 0U;
		int ret = tca8418_read_reg(cfg, TCA8418_REG_KEY_EVENT_A, &raw_event);

		if (ret != 0) {
			LOG_ERR("%s failed reading key event: %d", dev->name, ret);
			break;
		}

		tca8418_report_matrix_event(dev, raw_event);
	}

	uint8_t int_stat = 0U;
	if (tca8418_read_reg(cfg, TCA8418_REG_INT_STAT, &int_stat) == 0) {
		tca8418_clear_gpio_interrupts(cfg, int_stat);
		(void)tca8418_write_reg(cfg, TCA8418_REG_INT_STAT,
					TCA8418_INT_STAT_GPI_INT | TCA8418_INT_STAT_K_INT);
	}

	if (!data->irq_configured) {
		(void)k_work_reschedule(&data->poll_work, K_MSEC(cfg->poll_interval_ms));
	}
}

static void tca8418_irq_callback(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	struct tca8418_keypad_data *data =
		CONTAINER_OF(cb, struct tca8418_keypad_data, irq_cb);

	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	(void)k_work_reschedule(&data->poll_work, K_NO_WAIT);
}

static int tca8418_configure_matrix(const struct tca8418_keypad_config *cfg)
{
	const uint8_t kp_gpio_1 = tca8418_matrix_mask(cfg->rows);
	const uint8_t kp_gpio_2 = tca8418_matrix_mask((uint8_t)MIN(cfg->cols, 8U));
	const uint8_t kp_gpio_3 = cfg->cols > 8U ? tca8418_matrix_mask(cfg->cols - 8U) : 0U;
	int ret;

	ret = tca8418_write_reg(cfg, TCA8418_REG_CFG, TCA8418_CFG_KE_IEN | TCA8418_CFG_GPI_IEN);
	if (ret != 0) {
		return ret;
	}

	ret = tca8418_write_reg(cfg, TCA8418_REG_KP_GPIO_1, kp_gpio_1);
	if (ret != 0) {
		return ret;
	}

	ret = tca8418_write_reg(cfg, TCA8418_REG_KP_GPIO_2, kp_gpio_2);
	if (ret != 0) {
		return ret;
	}

	ret = tca8418_write_reg(cfg, TCA8418_REG_KP_GPIO_3, kp_gpio_3);
	if (ret != 0) {
		return ret;
	}

	return tca8418_write_reg(cfg, TCA8418_REG_INT_STAT,
				 TCA8418_INT_STAT_GPI_INT | TCA8418_INT_STAT_K_INT);
}

static int tca8418_keypad_init(const struct device *dev)
{
	const struct tca8418_keypad_config *cfg = dev->config;
	struct tca8418_keypad_data *data = dev->data;
	uint8_t probe = 0U;
	int ret;

	if (!device_is_ready(cfg->bus.bus)) {
		LOG_ERR("%s i2c bus not ready", dev->name);
		return -ENODEV;
	}

	ret = tca8418_read_reg(cfg, TCA8418_REG_KEY_LCK_EC, &probe);
	if (ret != 0) {
		LOG_ERR("%s probe failed: %d", dev->name, ret);
		return ret;
	}

	ret = tca8418_configure_matrix(cfg);
	if (ret != 0) {
		LOG_ERR("%s matrix configuration failed: %d", dev->name, ret);
		return ret;
	}

	data->dev = dev;
	k_work_init_delayable(&data->poll_work, tca8418_poll_once);

	if (cfg->has_irq) {
		if (!gpio_is_ready_dt(&cfg->irq)) {
			LOG_WRN("%s irq gpio not ready, falling back to polling", dev->name);
		} else {
			ret = gpio_pin_configure_dt(&cfg->irq, GPIO_INPUT);
			if (ret == 0) {
				gpio_init_callback(&data->irq_cb, tca8418_irq_callback, BIT(cfg->irq.pin));
				ret = gpio_add_callback_dt(&cfg->irq, &data->irq_cb);
			}
			if (ret == 0) {
				ret = gpio_pin_interrupt_configure_dt(&cfg->irq, GPIO_INT_EDGE_TO_ACTIVE);
			}
			if (ret == 0) {
				data->irq_configured = true;
			} else {
				LOG_WRN("%s irq setup failed rc=%d, falling back to polling", dev->name, ret);
			}
		}
	}

	(void)k_work_reschedule(&data->poll_work,
				data->irq_configured ? K_NO_WAIT : K_MSEC(cfg->poll_interval_ms));

	LOG_INF("%s ready rows=%u cols=%u irq=%s", dev->name, cfg->rows, cfg->cols,
		data->irq_configured ? "enabled" : "polling");
	return 0;
}

#define TCA8418_POLL_INTERVAL(inst) DT_INST_PROP_OR(inst, polling_interval_ms, 20)

#define TCA8418_CFG_INIT(inst)                                                                  \
	static const struct tca8418_keypad_config tca8418_keypad_cfg_##inst = {                \
		.bus = I2C_DT_SPEC_INST_GET(inst),                                              \
		.irq = GPIO_DT_SPEC_INST_GET_OR(inst, interrupt_gpios, {}),                     \
		.rows = DT_INST_PROP(inst, keypad_rows),                                        \
		.cols = DT_INST_PROP(inst, keypad_columns),                                     \
		.poll_interval_ms = TCA8418_POLL_INTERVAL(inst),                                \
		.has_irq = DT_INST_NODE_HAS_PROP(inst, interrupt_gpios),                        \
	};                                                                                     \
                                                                                               \
	static struct tca8418_keypad_data tca8418_keypad_data_##inst;                           \
                                                                                               \
	DEVICE_DT_INST_DEFINE(inst, tca8418_keypad_init, NULL,                                  \
			      &tca8418_keypad_data_##inst, &tca8418_keypad_cfg_##inst,       \
			      POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(TCA8418_CFG_INIT)
