# Extend the GUI

The [shared UI](../main/ui/handheld_ui.c) creates LVGL objects and manages page
and focus state. Board key events and SDL keyboard events both arrive as
`enum handheld_key` values. SDL pointer clicks and keypad selection invoke
the same row action, so a feature is reachable with either input method.

## Add a page

Use the DMX page as a reference for a screen with live data.

1. Add a page ID in [handheld_ui.h](../main/ui/handheld_ui.h), then give it a
   title and a `case` in `show()`. Add an action that opens the page and a
   menu row that invokes it.
2. Build its controls with `add_row()` or `add_control()`. The shared
   navigation assumes at least one focusable row and a valid starting
   focus index. Each page can register up to `MAX_ROWS` actions.
3. Wire the Back button and `KEY_BACK` to the same parent for a nested
   screen. The default Back behavior uses the current order of the three
   top-level page IDs to restore System settings focus. A nested page needs
   an explicit route, as `PAGE_DMX` has. If adding a top-level menu item,
   update the corresponding focus mapping.
4. If the page needs periodic refresh, start an LVGL timer after creating
   its widgets. Delete that timer before `show()` cleans the old screen.
   For data pushed by the platform, store a copy and only update widgets
   while the page owns them.
5. Open the page in the simulator. Try keyboard and pointer navigation,
   then leave and reopen it while data is changing. Check text width,
   focus visibility, high contrast, and the Back path at 240 × 320.

## Keep the board and simulator independent

The GUI can include LVGL and portable feature types. It receives domain
data through [handheld_ui.h](../main/ui/handheld_ui.h). GPIO, I²C, UART,
NVS, and FreeRTOS calls belong in the board adapter and its drivers. The
simulator supplies its own data and invokes the same UI entry points.

On the board, the display task calls `lv_timer_handler()` under the LCD
driver's recursive mutex. If another task needs to call a shared UI
function, take `wsr_lcd_lvgl_lock()` before touching LVGL state and release
it afterwards. LVGL callbacks and timers invoked inside the handler already
run under that lock. Keep device reads outside it. On the desktop, input,
refresh, and rendering all run on the SDL event loop.

For a complete example, add a temperature label using the
[I²C sensor tutorial](add-i2c-sensor.md). For a new feature interface, see
[system features](system-features.md).
