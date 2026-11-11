#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// PCF85063A calendar registers and control bits.
enum {
    PCF85063A_REG_CONTROL_1 = 0x00,
    PCF85063A_REG_SECONDS = 0x04,
    PCF85063A_CTRL1_STOP = 1u << 5,
    PCF85063A_CTRL1_12_24 = 1u << 1,
    PCF85063A_CTRL1_CAP_SEL = 1u,
    PCF85063A_SECONDS_OS = 1u << 7,
    PCF85063A_CMD_SOFTWARE_RESET = 0x58
};
enum pcf85063a_cap_sel { PCF85063A_CAP_SEL_7PF, PCF85063A_CAP_SEL_12_5PF };
struct pcf85063a_time {
    uint8_t seconds, minutes, hours, day, weekday, month, year;
};
struct pcf85063a_dev_hnd {
    bool (*write)(uint8_t reg, const uint8_t *data, size_t len);
    bool (*read)(uint8_t reg, uint8_t *data, size_t len);
};
bool pcf85063a_init(const struct pcf85063a_dev_hnd *hnd, enum pcf85063a_cap_sel cap_sel);
bool pcf85063a_get_time(const struct pcf85063a_dev_hnd *hnd, struct pcf85063a_time *t);
