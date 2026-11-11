#pragma once

#include "lvgl.h"

enum handheld_key { KEY_UP, KEY_DOWN, KEY_ENTER, KEY_BACK, KEY_OTHER };
enum handheld_page { PAGE_SETTINGS, PAGE_DEVICE, PAGE_DEV, PAGE_ABOUT };

struct handheld_preferences {
    uint8_t brightness;
    bool contrast;
};

struct handheld_platform {
    const char *target;
    const char *version;
    // Return false if the requested settings could not be stored.
    bool (*save)(struct handheld_preferences prefs);
};

void handheld_ui_start(lv_disp_t *display, const struct handheld_platform *platform,
                       struct handheld_preferences prefs);
void handheld_ui_key(enum handheld_key key, const char *label);
void handheld_ui_status(bool keypad, const char *rtc);
enum handheld_page handheld_ui_page(void);
