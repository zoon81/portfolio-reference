# System feature interface

The public reference carries the System Feature Interface (SFI) idea in
[handheld_ui.h](../main/ui/handheld_ui.h). `struct handheld_platform` gives
the GUI operations it can call, and functions such as `handheld_ui_status()`
give the platform a way to publish data. The application and simulator both
implement this boundary. The public reference uses feature-specific tasks
for background work such as DMX reception.

| When the feature needs to… | Follow this example |
| --- | --- |
| Apply a small action and return success or failure | `platform.save()` saves preferences on the board and stores them in memory in the simulator. The UI applies the new values after success and reports a failed save. |
| Publish status at a steady interval | `app_main()` samples the RTC and keypad and passes status to `handheld_ui_status()`. The UI caches it for the next visit to Dev Menu. |
| Receive data independently of the GUI | `dmx_uart_read()` copies a snapshot from a one-element queue without waiting. The receiver publishes from its own task and the screen refreshes at its own pace. |

## Add an operation or a snapshot

1. Define the smallest input and result the feature needs. Use domain values
   such as a temperature in hundredths of a degree and a status enum. Document
   which fields are valid in each state, who owns the data, and how long it
   stays current.
2. Add a function to the platform callback table for a UI request or a
   nonblocking snapshot read. For periodic data the application already
   samples, add a status setter and copy the values into the UI's state.
3. Implement the board adapter in `main/app.c` or in the feature's own
   module, then wire it up during startup. The driver owns transactions
   and the feature owns timing and policy. The UI owns presentation.
4. Implement the same contract in the simulator. Pick fixtures that make
   normal and failure paths easy to recognize. Exercise both through the
   same screen and navigation path.
5. Verify the user-visible result and the error path. Make sure it still
   works after leaving and reopening the screen.

## Choose where work runs

An LVGL event handler or timer runs in the GUI context. Keep callbacks
bounded and copy already available data there. The existing settings save
commits a small NVS record synchronously in that context. For a slower
operation, such as polling a device or writing a large file, enqueue a request
and report completion separately. Define what the GUI shows when the queue
is full or a request fails.

For streaming or periodic data, the worker can publish a complete snapshot.
A one-element overwrite queue suits a display that only needs the latest
reading. If each event matters, use a queue sized for the rate and define
how callers observe dropped events. Keep hardware access and waiting outside
the LVGL lock. Hand off copies so the producer can continue while the GUI
renders.

See the [temperature sensor tutorial](add-i2c-sensor.md) for a periodic
status update and [DMX over RS‑485](dmx-rs485.md) for a streaming feature.
