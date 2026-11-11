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
| RS‑485 receive | UART2 RX on GPIO 38, receiver enable and transmitter disable on expander port 1 |

## Schematics and PCB renders

| Board | Schematic PDF | PCB renders |
| --- | --- | --- |
| ESP32-S3 main board | [Main board](../hardware/main-board-schematic.pdf) | [Display side](../hardware/main-board-front.png) · [Component side](../hardware/main-board-back.png) |
| I²C keypad | [Keypad](../hardware/keypad-schematic.pdf) | [Button side](../hardware/keypad-front.png) · [Connector side](../hardware/keypad-back.png) |
| Optional DMX prefilter | [DMX prefilter](../hardware/dmx-prefilter-schematic.pdf) | [Assembled PCB](../hardware/dmx-prefilter.png) |

The main-board PDF shows the ESP32-S3 and LCD wiring on PDF page 1,
power on page 2, external connectors on page 3, the I²C expander and RTC
on page 4, the RS-485 transceiver on page 5, and the I²C address table
on page 6. The keypad schematic shows the switches and its separate
0x22 expander. The DMX prefilter is a standalone accessory.

The main-board exports use the layout dated 2025-03-13. The firmware's
rev 2 board description names its pin mapping. The display bus, I²C,
and RS-485 receive path were cross-checked against the exported
schematic. The older revision markings on the board and schematic
reflect their source files. The main-board PDF covers the circuits
used as firmware references. Its root sheet retains the RF connector
and sheet outline, and the dedicated RF sheet is outside the PDF.

## Desktop smoke check

With SDL2 installed, run the same local checks from a clean checkout:

```sh
./scripts/check.sh
```

Or run `ctest --test-dir simulator/build --output-on-failure` after a simulator-only build. The test navigates the actual shared LVGL UI, cycles brightness, toggles contrast, injects a failed save, and checks the diagnostics, DMX slot navigation, and About pages. A second test exercises DMX frame assembly and recovery from a receive overrun. The dummy SDL video driver lets it run without a window. It does not exercise the board's GPIO, LCD bus, NVS, RTC, or electrical DMX timing.

## Hardware status

This edition was compiled for the rev 2 board and exercised in the simulator. It has not been flashed or tested on a physical board. The firmware leaves the board's charger disabled and does not implement the original power manager.

The reference uses the ESP-IDF default single-app layout (partition table at `0x8000`, app at `0x10000`). Use a dedicated development board if you want to flash it. Check the installed partition table first; an app-only flash does not migrate the table, and a full flash can replace the existing layout. See the generated `build/flash_args` for this build's exact offsets.

## Verify your extension

Build the firmware and run the desktop checks after changing the shared
interface. In the simulator, exercise the success and failure paths through
the actual screen. The temperature sensor tutorial uses three fixtures
for startup without a reading, a valid reading, and a failed read after
success. Reopen the screen to check cached state and navigation.

On the board, verify electrical connections and the response under
representative load. For an I²C peripheral, check the address and device
identity on the populated board and boot with the optional part absent.
For sustained serial input, follow the [DMX bench procedure](dmx-rs485.md).

## Reproduce the README screenshots

The four GUI images come from the shared LVGL UI running in the SDL2
simulator. The capture mode runs the smoke scenario and saves the rendered
240 × 320 framebuffer for each page. From the repository root, with the
simulator already built:

```sh
sim="$PWD/simulator/build/handheld-sim"
capture_dir="$(mktemp -d)"
(cd "$capture_dir" && SDL_VIDEODRIVER=dummy "$sim" --capture)
```

The output directory contains `settings.bmp`, `device.bmp`,
`dev-menu.bmp`, and `dmx-monitor.bmp`. The README uses PNG conversions
of those files at the same resolution.
