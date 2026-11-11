#include "kb_io.h"

#include "esp_err.h"
#include <assert.h>

#include "kb_board_description.h"
#include "kts1622.h"
#include "wsr_iic.h"

static bool kb_io_reg_write_byte(uint8_t reg, uint8_t value) {
    uint8_t tx_buf[] = {reg, value};
    return wsr_iic_0_kb_io_transfer(tx_buf, sizeof(tx_buf), NULL, 0);
}

static bool kb_io_reg_write_word(uint8_t reg, uint16_t value) {
    const uint8_t value_msb = value >> 8;
    const uint8_t value_lsb = value & 0xFF;
    uint8_t tx_buf[] = {reg, value_msb, value_lsb};
    return wsr_iic_0_kb_io_transfer(tx_buf, sizeof(tx_buf), NULL, 0);
}

static bool kb_io_reg_read_byte(uint8_t reg, uint8_t *reg_value) {
    uint8_t tx_buf[] = {reg};
    return wsr_iic_0_kb_io_transfer(tx_buf, sizeof(tx_buf), reg_value, 1);
}

static bool kb_io_reg_read_word(uint8_t reg, uint16_t *reg_value) {
    uint8_t tx_buf[] = {reg};
    return wsr_iic_0_kb_io_transfer(tx_buf, sizeof(tx_buf), (uint8_t *)reg_value, 2);
}

const struct kts1622_dev_hnd kb_iodev_hnd = {.write8 = kb_io_reg_write_byte,
                                             .write16 = kb_io_reg_write_word,
                                             .read8 = kb_io_reg_read_byte,
                                             .read16 = kb_io_reg_read_word};

bool kb_io_init(void) {
    bool res;
    res = wsr_iic_0_init();
    assert(res == true);
    res = wsr_iic_0_probe_kb_io();
    if (res == false) {
        return false;
    }

    // Configure keypad inputs and their pull-ups.
    res &= kts1622_write_port_direction(&kb_iodev_hnd, KTS1622_PORT_0,
                                        KTS1622_DIRECTION_INPUT(KB_BOARD_P0_BTN_L_SPEC) |
                                            KTS1622_DIRECTION_INPUT(KB_BOARD_P0_BTN_L_SEL) |
                                            KTS1622_DIRECTION_INPUT(KB_BOARD_P0_BTN_LEFT) |
                                            KTS1622_DIRECTION_INPUT(KB_BOARD_P0_BTN_DOWN) |
                                            KTS1622_DIRECTION_INPUT(KB_BOARD_P0_BTN_ENTER) |
                                            KTS1622_DIRECTION_INPUT(KB_BOARD_P0_BTN_UP) |
                                            KTS1622_DIRECTION_INPUT(KB_BOARD_P0_BTN_RIGHT) |
                                            KTS1622_DIRECTION_INPUT(KB_BOARD_P0_BTN_R_SPEC));

    // Setting up port pull-up/pull-down on PORT0
    // All PORT0 pins are set to pull-up
    res &= kts1622_set_pd_pu_enable(
        &kb_iodev_hnd, KTS1622_PORT_0,
        KTS1622_PDPU_ENABLE(KB_BOARD_P0_BTN_L_SPEC) | KTS1622_PDPU_ENABLE(KB_BOARD_P0_BTN_L_SEL) |
            KTS1622_PDPU_ENABLE(KB_BOARD_P0_BTN_LEFT) | KTS1622_PDPU_ENABLE(KB_BOARD_P0_BTN_DOWN) |
            KTS1622_PDPU_ENABLE(KB_BOARD_P0_BTN_ENTER) | KTS1622_PDPU_ENABLE(KB_BOARD_P0_BTN_UP) |
            KTS1622_PDPU_ENABLE(KB_BOARD_P0_BTN_RIGHT) |
            KTS1622_PDPU_ENABLE(KB_BOARD_P0_BTN_R_SPEC));
    res &= kts1622_set_pd_pu_selection(&kb_iodev_hnd, KTS1622_PORT_0,
                                       KTS1622_PDPU_SELECTION_PULL_UP(KB_BOARD_P0_BTN_L_SPEC) |
                                           KTS1622_PDPU_SELECTION_PULL_UP(KB_BOARD_P0_BTN_L_SEL) |
                                           KTS1622_PDPU_SELECTION_PULL_UP(KB_BOARD_P0_BTN_LEFT) |
                                           KTS1622_PDPU_SELECTION_PULL_UP(KB_BOARD_P0_BTN_DOWN) |
                                           KTS1622_PDPU_SELECTION_PULL_UP(KB_BOARD_P0_BTN_ENTER) |
                                           KTS1622_PDPU_SELECTION_PULL_UP(KB_BOARD_P0_BTN_UP) |
                                           KTS1622_PDPU_SELECTION_PULL_UP(KB_BOARD_P0_BTN_RIGHT) |
                                           KTS1622_PDPU_SELECTION_PULL_UP(KB_BOARD_P0_BTN_R_SPEC));

    // Setting up port pin values on PORT1
    res &= kts1622_write_port_value(&kb_iodev_hnd, KTS1622_PORT_1,
                                    KTS1622_OUTPUT_PORT_PIN_VALUE_LOW(KB_BOARD_P1_LED_0) |
                                        KTS1622_OUTPUT_PORT_PIN_VALUE_LOW(KB_BOARD_P1_LED_1) |
                                        KTS1622_OUTPUT_PORT_PIN_VALUE_LOW(KB_BOARD_P1_LED_2));

    // Setting up port pin directions on PORT1
    res &= kts1622_write_port_direction(&kb_iodev_hnd, KTS1622_PORT_1,
                                        KTS1622_DIRECTION_INPUT(KB_BOARD_P1_BTN_R_SEL) |
                                            KTS1622_DIRECTION_OUTPUT(KB_BOARD_P1_LED_0) |
                                            KTS1622_DIRECTION_OUTPUT(KB_BOARD_P1_LED_1) |
                                            KTS1622_DIRECTION_OUTPUT(KB_BOARD_P1_LED_2) |
                                            KTS1622_DIRECTION_INPUT(KB_BOARD_P1_HW_ID_BIT_0) |
                                            KTS1622_DIRECTION_INPUT(KB_BOARD_P1_HW_ID_BIT_1) |
                                            KTS1622_DIRECTION_INPUT(KB_BOARD_P1_HW_ID_BIT_2) |
                                            KTS1622_DIRECTION_INPUT(KB_BOARD_P1_HW_ID_BIT_3));

    // Setting up port pull-up/pull-down on PORT1
    // HW-ID PORT1 pins are set to pull-up
    res &= kts1622_set_pd_pu_enable(&kb_iodev_hnd, KTS1622_PORT_1,
                                    KTS1622_PDPU_ENABLE(KB_BOARD_P1_BTN_R_SEL) |
                                        KTS1622_PDPU_ENABLE(KB_BOARD_P1_HW_ID_BIT_0) |
                                        KTS1622_PDPU_ENABLE(KB_BOARD_P1_HW_ID_BIT_1) |
                                        KTS1622_PDPU_ENABLE(KB_BOARD_P1_HW_ID_BIT_2) |
                                        KTS1622_PDPU_ENABLE(KB_BOARD_P1_HW_ID_BIT_3));
    res &= kts1622_set_pd_pu_selection(&kb_iodev_hnd, KTS1622_PORT_1,
                                       KTS1622_PDPU_SELECTION_PULL_UP(KB_BOARD_P1_BTN_R_SEL) |
                                           KTS1622_PDPU_SELECTION_PULL_UP(KB_BOARD_P1_HW_ID_BIT_0) |
                                           KTS1622_PDPU_SELECTION_PULL_UP(KB_BOARD_P1_HW_ID_BIT_1) |
                                           KTS1622_PDPU_SELECTION_PULL_UP(KB_BOARD_P1_HW_ID_BIT_2) |
                                           KTS1622_PDPU_SELECTION_PULL_UP(KB_BOARD_P1_HW_ID_BIT_3));

    res &= kts1622_set_irq_msk(&kb_iodev_hnd, KTS1622_PORT_0, 0xff);
    res &= kts1622_set_irq_msk(&kb_iodev_hnd, KTS1622_PORT_1, 0xff);
    res &= kts1622_clear_irq(&kb_iodev_hnd);

    return res;
}

bool kb_io_read_buttons(uint16_t *pressed_mask) {
    uint8_t p0 = 0;
    uint8_t p1 = 0;
    bool res = kts1622_read_input_port_value(&kb_iodev_hnd, KTS1622_PORT_0, &p0);
    res &= kts1622_read_input_port_value(&kb_iodev_hnd, KTS1622_PORT_1, &p1);
    if (res == false) {
        return false;
    }
    // Buttons are active-low (pressed pulls the line LOW), so invert: 1 = pressed.
    *pressed_mask = (uint16_t)((uint8_t)~p0) | ((uint16_t)((uint8_t)~p1) << 8);
    return true;
}
