# Add an I²C temperature sensor

This walkthrough adds an optional TMP117 and shows its reading in Dev Menu.
You can apply it on a branch and test the GUI in the simulator before you
connect hardware. The existing [RTC adapter](../main/drivers/rtc.c) shows
the same split between an I²C transaction and a device's register logic.

```text
I²C bus -> TMP117 register driver -> app_main() -> temperature status -> Dev Menu
                                                     ^
                                             SDL simulator fixture
```

## Register the sensor on the existing bus

The reference keeps physical addresses and logical device IDs in
[board_description.h](../main/drivers/board_description.h). The bus layer
in [wsr_iic.c](../main/drivers/wsr_iic.c) owns the ESP-IDF bus and device
handles. Each registered device has its own address and clock setting.
The existing wrappers use one shared transfer routine for writes, reads,
and combined write/read transactions.

1. In `board_description.h`, add `#define BOARD_TMP117_IIC_ADDRESS 0x48`
   and put `BOARD_IIC_NODE_TMP117,` immediately before
   `BOARD_IIC_NODE_COUNT`. This example assumes a TMP117 with ADD0 tied
   to ground. Check your board's address, logic voltage, and pull-ups
   before wiring. [TMP117 datasheet, sections 7.5 and 8.2](https://www.ti.com/lit/ds/symlink/tmp117.pdf)
2. In `wsr_iic_0_init()`, add the following after the RTC device is
   registered and before `is_initialized = true`. Registering a device
   allocates a device handle on the bus. The transaction later
   establishes whether the peripheral responds.

```c
wsr_dev_cfg[BOARD_IIC_NODE_TMP117] = (i2c_device_config_t){
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = BOARD_TMP117_IIC_ADDRESS,
    .scl_speed_hz = WSR_IIC_CLOCK_SPEED_HZ,
};
ESP_ERROR_CHECK(i2c_master_bus_add_device(wsr_iic_bus_handle,
                                         &wsr_dev_cfg[BOARD_IIC_NODE_TMP117],
                                         &wsr_dev_handle[BOARD_IIC_NODE_TMP117]));
```

Add this wrapper at the end of `wsr_iic.c` and its declaration to
[wsr_iic.h](../main/drivers/wsr_iic.h) before the closing `#endif`.
The register address goes out first, then the master reads the result
in the same transaction.

```c
bool wsr_iic_0_tmp117_read(uint8_t reg, uint8_t *data, size_t len) {
    return wsr_iic_0_transfer(BOARD_IIC_NODE_TMP117, &reg, 1, data, len);
}
```

## Decode the chip registers

Create `main/drivers/tmp117.h` and `main/drivers/tmp117.c` with the
following contents. Add `"drivers/tmp117.c"` to the `SRCS` list in
[main/CMakeLists.txt](../main/CMakeLists.txt).

```c
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool tmp117_read_centi_c(bool (*read_reg)(uint8_t, uint8_t *, size_t), int *centi_c);
```

```c
#include "tmp117.h"

bool tmp117_read_centi_c(bool (*read_reg)(uint8_t, uint8_t *, size_t), int *centi_c) {
    uint8_t bytes[2];
    if (!read_reg(0x0f, bytes, sizeof(bytes))) return false;

    const uint16_t device_id = ((uint16_t)bytes[0] << 8) | bytes[1];
    if ((device_id & 0x0fff) != 0x0117 || !read_reg(0x00, bytes, sizeof(bytes))) {
        return false;
    }

    const uint16_t raw = ((uint16_t)bytes[0] << 8) | bytes[1];
    if (raw == 0x8000) return false;

    const int32_t counts = raw < 0x8000 ? raw : (int32_t)raw - 65536;
    const int32_t scaled = counts * 100;
    *centi_c = (int)((scaled + (scaled < 0 ? -64 : 64)) / 128);
    return true;
}
```

The ID check masks off the silicon revision. The temperature register
holds signed counts at 128 counts per degree Celsius. The code rounds to
the nearest hundredth and treats the reset value as unavailable while the
first conversion is pending. The example uses the factory continuous-conversion mode. If your
module's EEPROM has been reconfigured, check the conversion mode
before using the example. The read callback keeps the
register logic independent of ESP-IDF and lets you test it with fixture
bytes. [TMP117 datasheet, sections 7.5.5, 7.6.2, and 7.6.11](https://www.ti.com/lit/ds/symlink/tmp117.pdf)

## Publish a status from the application

Add these declarations to [handheld_ui.h](../main/ui/handheld_ui.h). The
platform publishes a snapshot by value. Only a ready snapshot carries a
displayable temperature.

```c
enum handheld_temperature_status { TEMP_UNAVAILABLE, TEMP_READY, TEMP_LOST };

struct handheld_temperature {
    enum handheld_temperature_status status;
    int centi_c;
};

void handheld_ui_temperature(struct handheld_temperature temperature);
```

In [app.c](../main/app.c), include `tmp117.h` and `wsr_iic.h`.
Keep `bool had_temperature = false;` beside `key_ok`, before the main
loop. The app already starts the I²C bus through `ext_io_init()`. Use
the existing once-a-second status block. Insert this after the RTC read,
before taking the LVGL lock.

```c
struct handheld_temperature temperature = {
    .status = had_temperature ? TEMP_LOST : TEMP_UNAVAILABLE,
};
if (tmp117_read_centi_c(wsr_iic_0_tmp117_read, &temperature.centi_c)) {
    temperature.status = TEMP_READY;
    had_temperature = true;
}
```

Call `handheld_ui_temperature(temperature);` after
`handheld_ui_status(key_ok, clock);` inside the same locked block.
Each poll verifies the chip ID and tries a fresh reading. A sensor
that fails after supplying data is marked lost, and a later good read
restores it. A missing sensor can therefore leave the rest of Dev Menu
usable. Slow transactions still consume time in the application loop,
which also polls the keypad. Move sampling into its own task if that
delay is unacceptable. [System feature interface](system-features.md)

## Display it in Dev Menu

In [handheld_ui.c](../main/ui/handheld_ui.c), add
`struct handheld_temperature temperature;` and `lv_obj_t *temperature_label;`
to the static UI state. Define the following helper and setter before
`show()`. The snapshot is cached even while another screen is open.
Formatting belongs to the UI, which can render the same data from either
platform.

```c
static void update_temperature(void) {
    if (ui.page != PAGE_DEV) return;

    const struct handheld_temperature temperature = ui.temperature;
    if (temperature.status == TEMP_READY) {
        const int value = temperature.centi_c;
        const int magnitude = value < 0 ? -value : value;
        lv_label_set_text_fmt(ui.temperature_label, "Temp: %s%d.%02d C",
                              value < 0 ? "-" : "", magnitude / 100, magnitude % 100);
    } else {
        lv_label_set_text(ui.temperature_label,
                          temperature.status == TEMP_LOST ? "Temp: reading lost"
                                                          : "Temp: unavailable");
    }
}

void handheld_ui_temperature(struct handheld_temperature temperature) {
    ui.temperature = temperature;
    update_temperature();
}
```

In the `PAGE_DEV` branch of `show()`, move the keypad and RTC labels
to y=150 and y=172. Insert the following after `update_diagnostics()`
and move the DMX menu row from y=207 to y=210. Keep the Back button in
its existing place.

```c
ui.temperature_label = label(ui.screen, "", &lv_font_montserrat_12, muted, 12, 192);
update_temperature();
```

## Try all three states on the desktop

The status starts at `TEMP_UNAVAILABLE` through zero initialization.
In [simulator.c](../simulator/simulator.c), add this branch to the
`switch` in `key()`. It uses the same UI setter as the board.

```c
case SDLK_t: {
    static unsigned fixture;
    static const struct handheld_temperature samples[] = {
        {.status = TEMP_READY, .centi_c = 2344},
        {.status = TEMP_LOST},
        {.status = TEMP_UNAVAILABLE},
    };
    handheld_ui_temperature(samples[fixture]);
    fixture = (fixture + 1) % (sizeof(samples) / sizeof(samples[0]));
    break;
}
```

Build and launch the simulator. Enter Dev Menu and check for
`Temp: unavailable`. Press **T** to see `Temp: 23.44 C`, press it again
to see `Temp: reading lost`, then press twice more to return to a good
reading. Leave the menu and reopen it to verify the cached reading.
Try a fixture with `.centi_c = -25` to check the sign and padding.

Build the ESP32-S3 target before moving to the bench. On a compatible
development board, boot once with the sensor absent and once with the
sensor connected while powered down. Confirm the RTC, keypad, and DMX
menu still work. For a runtime read failure, inject a failed transaction
in a test build or use a fixture that can interrupt the sensor safely.
The simulator verifies the screen and state transitions. The bench
verifies the actual bus wiring, timing, and sensor behavior.
[Build and verification](build-and-verification.md)
