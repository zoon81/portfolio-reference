# Build and verification

## Toolchain and pinout

The firmware is pinned to ESP-IDF 5.3.1 and LVGL 8.3.11. Activate your installed ESP-IDF environment before running `./scripts/build.sh build`. The helper also accepts `IDF_PATH`, `IDF_PYTHON_ENV_PATH`, and `IDF_TOOLCHAIN_BIN` for installations with a separately managed environment. On first build, the ESP-IDF component manager fetches the LVGL version in `dependencies.lock`.

The board wiring lives in [board_description.h](../main/drivers/board_description.h).

| Peripheral | Rev 2 connection |
| --- | --- |
| ST7789 data and control | GPIO 7–14 (D0–D7), GPIO 4 (CS), 5 (DC), 6 (WR) |
| LCD reset and read strobe | Main I²C expander at 0x20, port 1 bits 1 and 2 |
| Backlight | GPIO 21, LEDC PWM |
| I²C bus | GPIO 17 (SDA), 18 (SCL) |
| Keypad | I²C expander at 0x22 |
| PCF85063A clock | I²C address 0x51 |

## Desktop smoke check

With SDL2 installed, run the same local checks from a clean checkout:

```sh
./scripts/check.sh
```

Or run `ctest --test-dir simulator/build --output-on-failure` after a simulator-only build. The test navigates the actual shared LVGL UI, cycles brightness, toggles contrast, injects a failed save, and checks the diagnostics and About pages. The dummy SDL video driver lets it run without a window. It does not exercise the board's GPIO, LCD bus, NVS, or RTC.

## Hardware status

This edition was compiled for the rev 2 board and exercised in the simulator. It has not been flashed or tested on a physical board. The firmware leaves the board's charger disabled and does not implement the original power manager.

The reference uses the ESP-IDF default single-app layout (partition table at `0x8000`, app at `0x10000`). Use a dedicated development board if you want to flash it. Check the installed partition table first; an app-only flash does not migrate the table, and a full flash can replace the existing layout. See the generated `build/flash_args` for this build's exact offsets.
