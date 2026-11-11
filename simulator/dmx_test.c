#include "dmx_monitor.h"

#include <stdio.h>

#define CHECK(expr)                                                                                \
    do {                                                                                           \
        if (!(expr)) {                                                                             \
            fprintf(stderr, "DMX check failed at line %d: %s\n", __LINE__, #expr);                 \
            return 1;                                                                              \
        }                                                                                          \
    } while (0)

int main(void) {
    struct dmx_monitor monitor = {0};
    uint8_t frame[DMX_FRAME_BYTES] = {0};
    frame[1] = 37;
    frame[122] = 219;
    frame[DMX_SLOTS] = 255;
    const uint8_t short_frame[] = {0, 89, 0};

    // Data before the first break is a partial packet.
    CHECK(!dmx_on_data(&monitor, short_frame, sizeof(short_frame), false, 0));
    CHECK(!dmx_on_break(&monitor, 1));
    CHECK(!dmx_on_data(&monitor, short_frame, sizeof(short_frame), true, 1));
    CHECK(dmx_snapshot_status(&monitor.latest, 1) == DMX_WAITING);

    // The FIFO tail arrives as a timeout chunk after BREAK.
    CHECK(!dmx_on_data(&monitor, frame, 122, false, 2));
    CHECK(!dmx_on_data(&monitor, frame + 122, 366, false, 3));
    CHECK(!dmx_on_break(&monitor, 4));
    CHECK(dmx_on_data(&monitor, frame + 488, sizeof(frame) - 488, true, 4));
    CHECK(monitor.latest.frames == 1 && monitor.latest.slot_count == DMX_SLOTS);
    CHECK(monitor.latest.slots[0] == 37 && monitor.latest.slots[121] == 219);
    CHECK(monitor.latest.slots[511] == 255);
    CHECK(dmx_snapshot_status(&monitor.latest, 1003) == DMX_ACTIVE);
    CHECK(dmx_snapshot_status(&monitor.latest, 1004) == DMX_IDLE);

    // No pending FIFO tail. The next FIFO-full chunk belongs to the next packet.
    CHECK(!dmx_on_data(&monitor, short_frame, sizeof(short_frame), true, 5));
    CHECK(!dmx_on_break(&monitor, 6));
    const uint8_t alternate[] = {0xcc, 16, 7};
    CHECK(dmx_on_data(&monitor, alternate, sizeof(alternate), false, 6));
    CHECK(monitor.latest.slot_count == 2 && monitor.latest.slots[0] == 89);
    CHECK(!dmx_on_break(&monitor, 7));
    CHECK(dmx_finish_break(&monitor, 11));
    CHECK(monitor.latest.frames == 2 && monitor.latest.dropped == 1);

    // Oversize packets and driver overruns leave the last good snapshot intact.
    CHECK(!dmx_on_data(&monitor, frame, sizeof(frame), false, 12));
    CHECK(!dmx_on_data(&monitor, short_frame, sizeof(short_frame), false, 13));
    CHECK(!dmx_on_break(&monitor, 14));
    CHECK(dmx_finish_break(&monitor, 18));
    CHECK(monitor.latest.dropped == 2);
    dmx_on_overrun(&monitor);
    CHECK(!dmx_on_data(&monitor, frame, sizeof(frame), false, 19));
    CHECK(!dmx_on_break(&monitor, 20));
    CHECK(!dmx_finish_break(&monitor, 24));
    CHECK(monitor.latest.frames == 2 && monitor.latest.dropped == 3);

    CHECK(!dmx_on_data(&monitor, short_frame, sizeof(short_frame), false, 25));
    CHECK(!dmx_on_break(&monitor, 26));
    CHECK(dmx_finish_break(&monitor, 30));
    CHECK(monitor.latest.frames == 3 && monitor.latest.slot_count == 2);
    return 0;
}
