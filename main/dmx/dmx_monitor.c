#include "dmx_monitor.h"

#include <string.h>

static void append(struct dmx_monitor *monitor, const uint8_t *data, size_t size) {
    if (!monitor->synchronized || monitor->invalid) return;
    if (size > sizeof(monitor->frame) - monitor->length) {
        monitor->invalid = true;
        return;
    }
    memcpy(monitor->frame + monitor->length, data, size);
    monitor->length += size;
}

bool dmx_finish_break(struct dmx_monitor *monitor, uint32_t now_ms) {
    if (!monitor->break_pending) return false;

    bool changed = false;
    if (monitor->synchronized && (monitor->length || monitor->invalid)) {
        if (monitor->invalid || monitor->length < 2 || monitor->frame[0] != 0) {
            monitor->latest.dropped++;
        } else {
            monitor->latest.slot_count = (uint16_t)(monitor->length - 1);
            memcpy(monitor->latest.slots, monitor->frame + 1, monitor->latest.slot_count);
            monitor->latest.frames++;
            monitor->latest.last_frame_ms = now_ms;
        }
        changed = true;
    }
    monitor->length = 0;
    monitor->invalid = false;
    monitor->break_pending = false;
    monitor->synchronized = true;
    return changed;
}

bool dmx_on_break(struct dmx_monitor *monitor, uint32_t now_ms) {
    const bool changed = dmx_finish_break(monitor, now_ms);
    monitor->break_pending = true;
    return changed;
}

bool dmx_on_data(struct dmx_monitor *monitor, const uint8_t *data, size_t size, bool timeout_chunk,
                 uint32_t now_ms) {
    if (!monitor->break_pending) {
        append(monitor, data, size);
        return false;
    }

    // The ESP32-S3 driver can report BREAK before it drains the old FIFO tail.
    // A timeout chunk immediately after BREAK can finish that packet.
    if (timeout_chunk) {
        append(monitor, data, size);
        return dmx_finish_break(monitor, now_ms);
    }

    const bool changed = dmx_finish_break(monitor, now_ms);
    append(monitor, data, size);
    return changed;
}

void dmx_on_overrun(struct dmx_monitor *monitor) {
    monitor->latest.dropped++;
    monitor->synchronized = false;
    monitor->break_pending = false;
    monitor->invalid = false;
    monitor->length = 0;
}

enum dmx_status dmx_snapshot_status(const struct dmx_snapshot *snapshot, uint32_t now_ms) {
    if (!snapshot->slot_count) return DMX_WAITING;
    return now_ms - snapshot->last_frame_ms < 1000 ? DMX_ACTIVE : DMX_IDLE;
}
