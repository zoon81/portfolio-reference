#pragma once

#include "dmx_monitor.h"

bool dmx_uart_start(void);
enum dmx_status dmx_uart_read(struct dmx_snapshot *snapshot);
