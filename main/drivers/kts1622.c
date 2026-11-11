#include "kts1622.h"

#include "wsr_iic.h"

enum kts1622_register {
    REG_INPUT_PORT_0 = 0,
    REG_INPUT_PORT_1,
    REG_OUTPUT_PORT_0,
    REG_OUTPUT_PORT_1,
    REG_POLARITY_INV_PORT_0,
    REG_POLARITY_INV_PORT_1,
    REG_CONFIG_PORT_0,
    REG_CONFIG_PORT_1,
    OUTPUT_DRIVE_STRENGTH_PORT_0_A = 0x40,
    OUTPUT_DRIVE_STRENGTH_PORT_0_B,
    OUTPUT_DRIVE_STRENGTH_PORT_1_A,
    OUTPUT_DRIVE_STRENGTH_PORT_1_B,
    INPUT_LATCH_PORT_0,
    INPUT_LATCH_PORT_1,
    PU_PD_ENABLE_PORT_0,
    PU_PD_ENABLE_PORT_1,
    PU_PD_SELECTION_PORT_0,
    PU_PD_SELECTION_PORT_1,
    IRQ_MASK_PORT_0,
    IRQ_MASK_PORT_1,
    IRQ_STATUS_PORT_0,
    IRQ_STATUS_PORT_1,
    IRQ_EDGE_SELECT_0_PORT_0 = 0x50,
    IRQ_EDGE_SELECT_1_PORT_0,
    IRQ_EDGE_SELECT_0_PORT_1,
    IRQ_EDGE_SELECT_1_PORT_1,
    IRQ_CLEAR_PORT_0 = 0x54,
    IRQ_CLEAR_PORT_1,
    OUTPUT_PORT_CONFIG = 0x4F
};

static bool __read_port_direction(const struct kts1622_dev_hnd *dev_hnd, enum kts1622_port port,
                                  uint8_t *direction);

bool kts1622_write_port_direction(const struct kts1622_dev_hnd *dev_hnd, enum kts1622_port port,
                                  uint8_t direction) {
    if (port == KTS1622_PORT_0) {
        return dev_hnd->write8(REG_CONFIG_PORT_0, direction);
    } else if (port == KTS1622_PORT_1) {
        return dev_hnd->write8(REG_CONFIG_PORT_1, direction);
    } else {
        return false;
    }
}

bool kts1622_set_port_pin_direction(const struct kts1622_dev_hnd *dev_hnd, enum kts1622_port port,
                                    uint8_t pin, enum kts1622_direction pin_direction) {
    uint8_t port_reg_value;
    bool ret;
    ret = __read_port_direction(dev_hnd, port, &port_reg_value);
    if (ret == false) {
        return false;
    }
    if (pin_direction == KTS1622_DIRECTION_OUTPUT) {
        port_reg_value &= ~(1 << pin);
    } else {
        port_reg_value |= (1 << pin);
    }
    return kts1622_write_port_direction(dev_hnd, port, port_reg_value);
}

bool kts1622_write_port_value(const struct kts1622_dev_hnd *dev_hnd, enum kts1622_port port,
                              uint8_t value) {
    if (port == KTS1622_PORT_0) {
        return dev_hnd->write8(REG_OUTPUT_PORT_0, value);
    } else if (port == KTS1622_PORT_1) {
        return dev_hnd->write8(REG_OUTPUT_PORT_1, value);
    } else {
        return false;
    }
}

bool kts1622_set_port_pin_value(const struct kts1622_dev_hnd *dev_hnd, enum kts1622_port port,
                                uint8_t pin, bool value) {
    uint8_t port_reg_value;
    bool ret;
    ret = kts1622_read_output_port_value(dev_hnd, port, &port_reg_value);
    if (ret == false) {
        return false;
    }
    if (value) {
        port_reg_value |= (1 << pin);
    } else {
        port_reg_value &= ~(1 << pin);
    }
    return kts1622_write_port_value(dev_hnd, port, port_reg_value);
}

bool kts1622_read_output_port_value(const struct kts1622_dev_hnd *dev_hnd, enum kts1622_port port,
                                    uint8_t *value) {
    if (port == KTS1622_PORT_0) {
        return dev_hnd->read8(REG_OUTPUT_PORT_0, value);
    } else if (port == KTS1622_PORT_1) {
        return dev_hnd->read8(REG_OUTPUT_PORT_1, value);
    } else {
        return false;
    }
}

bool kts1622_read_input_port_value(const struct kts1622_dev_hnd *dev_hnd, enum kts1622_port port,
                                   uint8_t *value) {
    if (port == KTS1622_PORT_0) {
        return dev_hnd->read8(REG_INPUT_PORT_0, value);
    } else if (port == KTS1622_PORT_1) {
        return dev_hnd->read8(REG_INPUT_PORT_1, value);
    } else {
        return false;
    }
}

bool kts1622_get_output_port_pin_value(const struct kts1622_dev_hnd *dev_hnd,
                                       enum kts1622_port port, uint8_t pin, bool *value) {
    uint8_t port_reg_value;
    bool ret;
    ret = kts1622_read_output_port_value(dev_hnd, port, &port_reg_value);
    if (ret == false) {
        return false;
    }
    *value = (port_reg_value >> pin) & 0x01;
    return true;
}

bool kts1622_get_input_port_pin_value(const struct kts1622_dev_hnd *dev_hnd, enum kts1622_port port,
                                      uint8_t pin, bool *value) {
    uint8_t port_reg_value;
    bool ret;
    ret = kts1622_read_input_port_value(dev_hnd, port, &port_reg_value);
    if (ret == false) {
        return false;
    }
    *value = (port_reg_value >> pin) & 0x01;
    return true;
}

bool kts1622_set_pd_pu_enable(const struct kts1622_dev_hnd *dev_hnd, enum kts1622_port port,
                              uint8_t value) {
    if (port == KTS1622_PORT_0) {
        return dev_hnd->write8(PU_PD_ENABLE_PORT_0, value);
    } else if (port == KTS1622_PORT_1) {
        return dev_hnd->write8(PU_PD_ENABLE_PORT_1, value);
    } else {
        return false;
    }
}

bool kts1622_set_pd_pu_selection(const struct kts1622_dev_hnd *dev_hnd, enum kts1622_port port,
                                 uint8_t value) {
    if (port == KTS1622_PORT_0) {
        return dev_hnd->write8(PU_PD_SELECTION_PORT_0, value);
    } else if (port == KTS1622_PORT_1) {
        return dev_hnd->write8(PU_PD_SELECTION_PORT_1, value);
    } else {
        return false;
    }
}

bool kts1622_set_irq_msk(const struct kts1622_dev_hnd *dev_hnd, enum kts1622_port port,
                         uint8_t value) {
    if (port == KTS1622_PORT_0) {
        return dev_hnd->write8(IRQ_MASK_PORT_0, value);
    } else if (port == KTS1622_PORT_1) {
        return dev_hnd->write8(IRQ_MASK_PORT_1, value);
    } else {
        return false;
    }
}

bool kts1622_get_irq_status_by_port(const struct kts1622_dev_hnd *dev_hnd, enum kts1622_port port,
                                    uint8_t *value) {
    if (port == KTS1622_PORT_0) {
        return dev_hnd->read8(IRQ_STATUS_PORT_0, value);
    } else if (port == KTS1622_PORT_1) {
        return dev_hnd->read8(IRQ_STATUS_PORT_1, value);
    } else {
        return false;
    }
}

bool kts1622_get_irq_status(const struct kts1622_dev_hnd *dev_hnd, uint16_t *value) {
    return dev_hnd->read16(IRQ_STATUS_PORT_0, value);
}

bool kts1622_set_irq_edge_select(const struct kts1622_dev_hnd *dev_hnd, enum kts1622_port port,
                                 uint16_t value) {
    if (port == KTS1622_PORT_0) {
        return dev_hnd->write8(IRQ_EDGE_SELECT_0_PORT_0, value & 0xFF) &&
               dev_hnd->write8(IRQ_EDGE_SELECT_1_PORT_0, (value >> 8) & 0xFF);
    } else if (port == KTS1622_PORT_1) {
        return dev_hnd->write8(IRQ_EDGE_SELECT_0_PORT_1, value & 0xFF) &&
               dev_hnd->write8(IRQ_EDGE_SELECT_1_PORT_1, (value >> 8) & 0xFF);
    } else {
        return false;
    }
}

bool kts1622_clear_irq(const struct kts1622_dev_hnd *dev_hnd) {
    return dev_hnd->write16(IRQ_CLEAR_PORT_0, 0xFFFF);
}

static bool __read_port_direction(const struct kts1622_dev_hnd *dev_hnd, enum kts1622_port port,
                                  uint8_t *direction) {
    if (port == KTS1622_PORT_0) {
        return dev_hnd->read8(REG_CONFIG_PORT_0, direction);
    } else if (port == KTS1622_PORT_1) {
        return dev_hnd->read8(REG_CONFIG_PORT_1, direction);
    } else {
        return false;
    }
}
