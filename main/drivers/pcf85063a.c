#include "pcf85063a.h"

static inline uint8_t bcd2dec(uint8_t bcd) { return (uint8_t)((bcd >> 4) * 10u + (bcd & 0x0Fu)); }

static bool pcf85063a_read_reg(const struct pcf85063a_dev_hnd *hnd, uint8_t reg, uint8_t *value) {
    if (hnd == NULL || value == NULL) {
        return false;
    }
    return hnd->read(reg, value, 1);
}

static bool pcf85063a_write_reg(const struct pcf85063a_dev_hnd *hnd, uint8_t reg, uint8_t value) {
    if (hnd == NULL) {
        return false;
    }
    return hnd->write(reg, &value, 1);
}

/* ------------------------------- Control --------------------------------- */

bool pcf85063a_init(const struct pcf85063a_dev_hnd *hnd, enum pcf85063a_cap_sel cap_sel) {
    uint8_t ctrl1;
    if (!pcf85063a_read_reg(hnd, PCF85063A_REG_CONTROL_1, &ctrl1)) {
        return false;
    }
    /* Start the oscillator in 24-hour mode with the fitted crystal load.
     * Preserve the other control bits and the calendar. */
    ctrl1 &= (uint8_t)~(PCF85063A_CTRL1_STOP | PCF85063A_CTRL1_12_24 | PCF85063A_CTRL1_CAP_SEL);
    if (cap_sel == PCF85063A_CAP_SEL_12_5PF) {
        ctrl1 |= PCF85063A_CTRL1_CAP_SEL;
    }
    return pcf85063a_write_reg(hnd, PCF85063A_REG_CONTROL_1, ctrl1);
}

/* -------------------------- Oscillator-stop flag ------------------------- */

bool pcf85063a_get_time(const struct pcf85063a_dev_hnd *hnd, struct pcf85063a_time *t) {
    uint8_t raw[7];
    if (hnd == NULL || t == NULL) {
        return false;
    }
    if (!hnd->read(PCF85063A_REG_SECONDS, raw, sizeof(raw)) || (raw[0] & PCF85063A_SECONDS_OS)) {
        return false;
    }
    t->seconds = bcd2dec(raw[0] & 0x7Fu); /* strip OS */
    t->minutes = bcd2dec(raw[1] & 0x7Fu);
    t->hours = bcd2dec(raw[2] & 0x3Fu); /* 24h mode */
    t->day = bcd2dec(raw[3] & 0x3Fu);
    t->weekday = (uint8_t)(raw[4] & 0x07u);
    t->month = bcd2dec(raw[5] & 0x1Fu);
    t->year = bcd2dec(raw[6]);
    return t->hours < 24 && t->minutes < 60 && t->seconds < 60 && t->month >= 1 && t->month <= 12 &&
           t->day >= 1 && t->day <= 31;
}
