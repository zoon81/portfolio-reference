#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum { DMX_SLOTS = 512, DMX_FRAME_BYTES = 1 + DMX_SLOTS };
enum dmx_status { DMX_UNAVAILABLE, DMX_WAITING, DMX_ACTIVE, DMX_IDLE };

struct dmx_snapshot {
    uint8_t slots[DMX_SLOTS];
    uint16_t slot_count;
    uint32_t frames;
    uint32_t dropped;
    uint32_t last_frame_ms;
};

struct dmx_monitor {
    struct dmx_snapshot latest;
    uint8_t frame[DMX_FRAME_BYTES];
    size_t length;
    bool synchronized;
    bool invalid;
    bool break_pending;
};

// Return true when the published counters or channel values change.
bool dmx_on_break(struct dmx_monitor *monitor, uint32_t now_ms);
bool dmx_on_data(struct dmx_monitor *monitor, const uint8_t *data, size_t size, bool timeout_chunk,
                 uint32_t now_ms);
bool dmx_finish_break(struct dmx_monitor *monitor, uint32_t now_ms);
void dmx_on_overrun(struct dmx_monitor *monitor);

enum dmx_status dmx_snapshot_status(const struct dmx_snapshot *snapshot, uint32_t now_ms);
