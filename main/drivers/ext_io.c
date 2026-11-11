#include "ext_io.h"

#include "esp_err.h"
#include <assert.h>

#include "board_description.h"
#include "kts1622.h"
#include "wsr_iic.h"

static bool ext_io_reg_write_byte(uint8_t reg, uint8_t value) {
    uint8_t tx_buf[] = {reg, value};
    return wsr_iic_0_ext_io_transfer(tx_buf, sizeof(tx_buf), NULL, 0);
}

static bool ext_io_reg_write_word(uint8_t reg, uint16_t value) {
    const uint8_t value_msb = value >> 8;
    const uint8_t value_lsb = value & 0xFF;
    uint8_t tx_buf[] = {reg, value_msb, value_lsb};
    return wsr_iic_0_ext_io_transfer(tx_buf, sizeof(tx_buf), NULL, 0);
}

static bool ext_io_reg_read_byte(uint8_t reg, uint8_t *reg_value) {
    uint8_t tx_buf[] = {reg};
    return wsr_iic_0_ext_io_transfer(tx_buf, sizeof(tx_buf), reg_value, 1);
}

static bool ext_io_reg_read_word(uint8_t reg, uint16_t *reg_value) {
    uint8_t tx_buf[] = {reg};
    return wsr_iic_0_ext_io_transfer(tx_buf, sizeof(tx_buf), (uint8_t *)reg_value, 2);
}

const struct kts1622_dev_hnd ext_io_dev_hnd = {.write8 = ext_io_reg_write_byte,
                                               .write16 = ext_io_reg_write_word,
                                               .read8 = ext_io_reg_read_byte,
                                               .read16 = ext_io_reg_read_word};

bool ext_io_init(void) {
    bool res;
    res = wsr_iic_0_init();
    assert(res == true);
    res = wsr_iic_0_probe_ext_io();
    if (res == false) {
        return false;
    }

    // Preload output levels before changing pin directions.
    const uint8_t p0 = (1u << BOARD_CC_CHR_EN_EXT_GPIO_0_PIN);
    const uint8_t p1 = (1u << BOARD_LCD_RST_EXT_GPIO_1_PIN) | (1u << BOARD_LCD_RDX_EXT_GPIO_1_PIN);
    return kts1622_write_port_value(&ext_io_dev_hnd, KTS1622_PORT_0, p0) &&
           kts1622_write_port_value(&ext_io_dev_hnd, KTS1622_PORT_1, p1) &&
           kts1622_write_port_direction(&ext_io_dev_hnd, KTS1622_PORT_0,
                                        (uint8_t)~((1u << BOARD_CC_CHR_EN_EXT_GPIO_0_PIN) |
                                                   (1u << BOARD_KB_IO_3_EXT_GPIO_0_PIN))) &&
           kts1622_write_port_direction(&ext_io_dev_hnd, KTS1622_PORT_1, (uint8_t)~p1);
}

bool ext_io_LCD_RST_set(void) {
    return kts1622_set_port_pin_value(&ext_io_dev_hnd, KTS1622_PORT_1, BOARD_LCD_RST_EXT_GPIO_1_PIN,
                                      false);
}

bool ext_io_LCD_RST_clr(void) {
    return kts1622_set_port_pin_value(&ext_io_dev_hnd, KTS1622_PORT_1, BOARD_LCD_RST_EXT_GPIO_1_PIN,
                                      true);
}
