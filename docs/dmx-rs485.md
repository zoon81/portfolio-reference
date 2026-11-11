# DMX over RS‑485

Open **System settings → Dev Menu → DMX monitor**. The screen shows signal
status, accepted frames, dropped packets or receive overruns, and the last
accepted slot values. **Prev** and **Next** step through 16 slots at a time.
Slots missing from a short packet appear as `--`. If no packet has arrived
for one second, the monitor marks the signal idle and keeps the last values
visible.

## From the wire to the screen

- **Board interface.** The rev 2 transceiver's receiver feeds GPIO 38 on
  UART2, configured for 250 kbit/s with 8 data bits, no parity, and 2 stop
  bits. The I²C expander holds the transmitter disabled and enables the
  receiver after the UART is ready.
- **Frame assembly.** The receiver waits for a break before collecting bytes.
  The ESP-IDF event queue can report the break before a timeout event drains
  the preceding frame's final FIFO chunk. The assembler defers publication
  so that chunk can be included. When the next event is a FIFO threshold
  chunk, it starts the next packet. A short wait completes a pending break
  if no data event follows. This is an adapter for the observed UART event
  ordering on this board and SDK, which should be checked on the bench with
  short packets and different inter-slot gaps.
- **Slot validation.** Byte zero is the start code. The monitor accepts a zero
  start code and 1–512 slots. Empty boundaries are skipped. Alternate start
  codes, oversize packets, and receive overruns increment the drop counter.
  The driver clears its receive buffers after an overrun and waits for a
  fresh break. The monitor reports slot values without interpreting them as
  application commands.
- **UI handoff.** The UART task overwrites a one-element queue with the latest
  snapshot. The UI reads it at most once per 125 ms refresh, plus immediate
  reads on page navigation. The UI can miss intermediate display samples
  while the frame counter still tracks every accepted packet.

The protocol reference is [ANSI E1.11, DMX512-A](https://tsp.esta.org/tsp/documents/published_docs.php).
The driver uses the [ESP-IDF UART event API](https://docs.espressif.com/projects/esp-idf/en/v5.3.1/esp32s3/api-reference/peripherals/uart.html).

## Try it in the simulator

The simulator generates a full synthetic packet roughly 40 times per second.
It splits the data into UART-style chunks and delivers the last chunk after a
break event. The first three slots read 32, 49, and 66. **Space** pauses and
resumes the synthetic input, so you can check the transition to idle and back
to receiving. The sample is generated locally from a fixed formula.

## Verify on the bench

On a dedicated rev 2 development board running this reference build, connect
a DMX source to the RS‑485 input using the board's connector pinout. Send a
repeating zero-start-code packet with distinct values at slots 1, 17, and 512.
Check each value on its corresponding bank and confirm the drop counter stays
steady. Repeat with a short packet and verify the absent slots read `--`.
Stop the source and check that the monitor reports idle within about one
second. Resume the stream and check that the display recovers.

The simulator checks assembly and display behavior. Wire timing, electrical
behavior, and sustained throughput still need this bench run.
