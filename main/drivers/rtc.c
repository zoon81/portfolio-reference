#include "rtc.h"

#include <assert.h>

#include "pcf85063a.h"
#include "wsr_iic.h"

/* Fitted crystal: CM7V-T1A-32.768kHz-12.5pF-20PPM-TA-QC -> 12.5 pF load.
 * The CAP_SEL bit must match this or the 20 ppm accuracy does not hold. */
#define RTC_CRYSTAL_LOAD_CAP PCF85063A_CAP_SEL_12_5PF

/* --------------------- Transport bound to the I2C bus -------------------- */

static bool rtc_bus_write(uint8_t reg, const uint8_t *data, size_t len) {
    /* Largest write is the 7-byte calendar block plus the register address. */
    uint8_t buf[1 + 16];
    if (len > sizeof(buf) - 1) {
        return false;
    }
    buf[0] = reg;
    for (size_t i = 0; i < len; i++) {
        buf[1 + i] = data[i];
    }
    return wsr_iic_0_rtc_transfer(buf, len + 1, NULL, 0);
}

static bool rtc_bus_read(uint8_t reg, uint8_t *data, size_t len) {
    uint8_t tx = reg;
    return wsr_iic_0_rtc_transfer(&tx, 1, data, len);
}

static const struct pcf85063a_dev_hnd rtc_dev_hnd = {.write = rtc_bus_write, .read = rtc_bus_read};

/* ------------------------------- Public API ------------------------------ */

bool wsr_rtc_init(void) {
    bool res = wsr_iic_0_init();
    assert(res == true);

    if (!wsr_iic_0_probe_rtc()) {
        return false;
    }

    return pcf85063a_init(&rtc_dev_hnd, RTC_CRYSTAL_LOAD_CAP);
}

bool wsr_rtc_get_time(struct pcf85063a_time *t) { return pcf85063a_get_time(&rtc_dev_hnd, t); }
