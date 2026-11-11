#pragma once

#include "pcf85063a.h"

bool wsr_rtc_init(void);
bool wsr_rtc_get_time(struct pcf85063a_time *t);
