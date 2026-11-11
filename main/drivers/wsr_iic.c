#include "wsr_iic.h"

#include "board_description.h"

#include "assert.h"
#include "driver/i2c_master.h"
#include "esp_err.h"

static i2c_device_config_t wsr_dev_cfg[BOARD_IIC_NODE_COUNT];

static i2c_master_bus_handle_t wsr_iic_bus_handle;
static i2c_master_dev_handle_t wsr_dev_handle[BOARD_IIC_NODE_COUNT];
static bool is_initialized = false;

bool wsr_iic_0_init(void) {
    if (is_initialized) {
        return true;
    }

    i2c_master_bus_config_t i2c_bus_config = {.clk_source = I2C_CLK_SRC_DEFAULT,
                                              .i2c_port = 0,
                                              .scl_io_num = BOARD_IIC_SCL_NATIVE_GPIO_PIN,
                                              .sda_io_num = BOARD_IIC_SDA_NATIVE_GPIO_PIN,
                                              .glitch_ignore_cnt = 7,
                                              .intr_priority = 0};

    esp_err_t err;

    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_config, &wsr_iic_bus_handle));
    // clang-format off
	wsr_dev_cfg[BOARD_IIC_NODE_EXT_IO] = (i2c_device_config_t) {
			.dev_addr_length = I2C_ADDR_BIT_LEN_7,
			.device_address = BOARD_EXT_IO_IIC_ADDRESS,
			.scl_speed_hz = WSR_IIC_CLOCK_SPEED_HZ
	};
	wsr_dev_cfg[BOARD_IIC_NODE_KB_IO] = (i2c_device_config_t) {
		.dev_addr_length = I2C_ADDR_BIT_LEN_7,
		.device_address = BOARD_KB_IO_IIC_ADDRESS,
		.scl_speed_hz = WSR_IIC_CLOCK_SPEED_HZ
	};
	wsr_dev_cfg[BOARD_IIC_NODE_RTC] = (i2c_device_config_t) {
		.dev_addr_length = I2C_ADDR_BIT_LEN_7,
		.device_address = BOARD_RTC_IIC_ADDRESS,
		.scl_speed_hz = WSR_IIC_CLOCK_SPEED_HZ
	};
    // clang-format on
    err = i2c_master_bus_add_device(wsr_iic_bus_handle, &wsr_dev_cfg[BOARD_IIC_NODE_EXT_IO],
                                    &wsr_dev_handle[BOARD_IIC_NODE_EXT_IO]);
    if (err != ESP_OK) {
        ESP_ERROR_CHECK(err);
        return false;
    }

    err = i2c_master_bus_add_device(wsr_iic_bus_handle, &wsr_dev_cfg[BOARD_IIC_NODE_KB_IO],
                                    &wsr_dev_handle[BOARD_IIC_NODE_KB_IO]);
    if (err != ESP_OK) {
        ESP_ERROR_CHECK(err);
        return false;
    }

    err = i2c_master_bus_add_device(wsr_iic_bus_handle, &wsr_dev_cfg[BOARD_IIC_NODE_RTC],
                                    &wsr_dev_handle[BOARD_IIC_NODE_RTC]);
    if (err != ESP_OK) {
        ESP_ERROR_CHECK(err);
        return false;
    }

    is_initialized = true;
    return true;
}

static bool wsr_iic_0_transfer(enum board_iic_device_list node, const uint8_t *tx_data,
                               size_t tx_len, uint8_t *rx_data, size_t rx_len) {
    assert(node < BOARD_IIC_NODE_COUNT && node >= 0);
    assert(tx_len || rx_len);
    assert(tx_data != NULL || rx_data != NULL);

    esp_err_t err = ESP_FAIL;
    // TX and RX
    if (tx_len > 0 && rx_len > 0) {
        err = i2c_master_transmit_receive(wsr_dev_handle[node], tx_data, tx_len, rx_data, rx_len,
                                          WSR_IIC_TRANSFER_TIMEOUT_MS);

        if (err == ESP_ERR_INVALID_ARG) {
            ESP_ERROR_CHECK(err);
        }
        // TX only
    } else if (tx_len > 0) {
        err =
            i2c_master_transmit(wsr_dev_handle[node], tx_data, tx_len, WSR_IIC_TRANSFER_TIMEOUT_MS);

        if (err == ESP_ERR_INVALID_ARG) {
            ESP_ERROR_CHECK(err);
        }
        // RX only
    } else if (rx_len > 0) {
        err =
            i2c_master_receive(wsr_dev_handle[node], rx_data, rx_len, WSR_IIC_TRANSFER_TIMEOUT_MS);

        if (err == ESP_ERR_INVALID_ARG) {
            ESP_ERROR_CHECK(err);
        }
    }
    if (err != ESP_OK) {
        return false;
    } else {
        return true;
    }
}

static bool wsr_iic_0_probe(enum board_iic_device_list node) {
    assert(node < BOARD_IIC_NODE_COUNT && node >= 0);
    esp_err_t err = i2c_master_probe(wsr_iic_bus_handle, wsr_dev_cfg[node].device_address,
                                     WSR_IIC_TRANSFER_TIMEOUT_MS);

    return err == ESP_OK;
}

bool wsr_iic_0_ext_io_transfer(const uint8_t *tx_data, size_t tx_len, uint8_t *rx_data,
                               size_t rx_len) {
    return wsr_iic_0_transfer(BOARD_IIC_NODE_EXT_IO, tx_data, tx_len, rx_data, rx_len);
}

bool wsr_iic_0_kb_io_transfer(const uint8_t *tx_data, size_t tx_len, uint8_t *rx_data,
                              size_t rx_len) {
    return wsr_iic_0_transfer(BOARD_IIC_NODE_KB_IO, tx_data, tx_len, rx_data, rx_len);
}

bool wsr_iic_0_rtc_transfer(const uint8_t *tx_data, size_t tx_len, uint8_t *rx_data,
                            size_t rx_len) {
    return wsr_iic_0_transfer(BOARD_IIC_NODE_RTC, tx_data, tx_len, rx_data, rx_len);
}

bool wsr_iic_0_probe_ext_io(void) { return wsr_iic_0_probe(BOARD_IIC_NODE_EXT_IO); }

bool wsr_iic_0_probe_kb_io(void) { return wsr_iic_0_probe(BOARD_IIC_NODE_KB_IO); }

bool wsr_iic_0_probe_rtc(void) { return wsr_iic_0_probe(BOARD_IIC_NODE_RTC); }
