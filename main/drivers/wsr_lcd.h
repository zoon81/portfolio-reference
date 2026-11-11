#pragma once

#include "lvgl.h"

#define WSR_LCD_H_RES 240
#define WSR_LCD_V_RES 320
#define WSR_LCD_PIXEL_CLOCK_HZ (10 * 1000 * 1000)

lv_disp_t *wsr_lcd_init(void);
bool wsr_lcd_lvgl_lock(int timeout_ms);
bool wsr_lcd_lvgl_unlock(void);
