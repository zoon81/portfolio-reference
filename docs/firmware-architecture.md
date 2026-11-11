# Firmware architecture

The reference has one UI and two platform adapters. It keeps the hardware details near the drivers while making it possible to test navigation on a desktop.

```text
Rev 2 board                      Desktop
KTS1622 keypad + PCF85063A RTC    SDL2 keys, pointer + sample clock
              |                         |
     main/app.c                 simulator/simulator.c
              |                         |
              +-- main/ui/handheld_ui.c-+
                  LVGL objects and menu state
              |                         |
      ST7789 over i80 DMA        SDL2 RGB565 texture
```

## Display: one buffer and an asynchronous transfer

The ST7789 is connected to the ESP32-S3's 8-bit i80 bus. LVGL renders a 240-pixel-wide RGB565 image using an 80-line partial buffer in internal DMA-capable memory (38,400 bytes). Its flush callback passes the dirty rectangle to the LCD driver, with inclusive LVGL endpoints converted to the exclusive endpoints the driver expects. LVGL can reuse the buffer only after the driver's transfer completion callback calls `lv_disp_flush_ready()`. A rejected transfer also releases the buffer so a single error does not wedge future draws.

The driver expands horizontal dirty rectangles to 8-pixel boundaries, keeping transfer lengths divisible by 16 bytes. The display task runs `lv_timer_handler()`; firmware input and status updates take the same LVGL lock before changing widgets. The simulator runs the equivalent UI in its SDL event loop and copies dirty rectangles into a texture.

## Input: keypad polling and press edges

The two KTS1622 expanders have separate jobs. One drives the LCD reset and read strobe. The second reads the nine active-low keypad inputs. The firmware samples the keypad outside the LVGL lock at roughly 15 ms intervals. It accepts a change after two matching samples and emits only newly pressed bits. Up and Down move focus, Enter selects, and the left special key goes back. The remaining keypad keys can be inspected in Dev Menu.

The PCF85063A adapter reads the calendar registers in one I²C transaction, checks the oscillator-stop bit, and decodes the clock. The application updates the UI about once a second. The reference shows the time already on the RTC; it does not set it. If the clock is unset or the read fails, Dev Menu shows that state.

## Preferences: one small NVS record

Brightness is represented as 25, 50, 75, or 100 percent. The firmware packs it and the contrast flag into one 16-bit value under `handheld_ref/display`. The UI applies the new settings after the platform accepts the save. Invalid or missing values fall back to 75% and the default theme. An NVS initialization failure leaves the existing partition alone and runs with defaults. The simulator exercises the same UI save path using an in-memory record.

## DMX diagnostics

UART2 feeds a portable packet assembler. The firmware publishes its latest
snapshot through a one-element queue. The UI refreshes eight times per second
while the monitor is visible. The simulator generates UART events in software
and uses the same assembler. See [DMX over RS‑485](dmx-rs485.md) for framing,
recovery, and bench steps.
