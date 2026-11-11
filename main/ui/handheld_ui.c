#include "handheld_ui.h"

#include <stdio.h>
#include <string.h>

#define WIDTH 240
#define MAX_ROWS 4

struct row {
    lv_obj_t *button;
    void (*action)(void);
};

static struct {
    const struct handheld_platform *platform;
    struct handheld_preferences prefs;
    enum handheld_page page;
    lv_obj_t *screen, *footer, *key_label, *rtc_label, *keypad_label;
    struct row rows[MAX_ROWS];
    unsigned count, focus, key_count;
    char last_key[32], rtc[32];
    bool keypad;
} ui;

static void show(enum handheld_page page, unsigned focus);
static void home(void) { show(PAGE_SETTINGS, 0); }
static void device(void) { show(PAGE_DEVICE, 0); }
static void dev(void) { show(PAGE_DEV, 0); }
static void about(void) { show(PAGE_ABOUT, 0); }

static lv_color_t color(uint32_t normal, uint32_t contrast) {
    return lv_color_hex(ui.prefs.contrast ? contrast : normal);
}

static lv_obj_t *label(lv_obj_t *parent, const char *value, const lv_font_t *font,
                       lv_color_t text_color, int x, int y) {
    lv_obj_t *object = lv_label_create(parent);
    lv_label_set_text(object, value);
    lv_obj_set_style_text_font(object, font, 0);
    lv_obj_set_style_text_color(object, text_color, 0);
    lv_obj_set_pos(object, x, y);
    return object;
}

static void focus_row(unsigned focus) {
    ui.focus = focus;
    for (unsigned i = 0; i < ui.count; i++) {
        const bool selected = i == focus;
        lv_obj_set_style_border_width(ui.rows[i].button, selected ? 2 : 1, 0);
        lv_obj_set_style_border_color(
            ui.rows[i].button, selected ? color(0x73e4bc, 0xffff00) : color(0x34495c, 0xffffff), 0);
        lv_obj_set_style_bg_color(
            ui.rows[i].button, selected ? color(0x193d3e, 0x252500) : color(0x172836, 0x101010), 0);
    }
}

static void click(lv_event_t *event) {
    struct row *row = lv_event_get_user_data(event);
    focus_row((unsigned)(row - ui.rows));
    row->action();
}

static void add_row(unsigned y, const char *title, const char *detail, void (*action)(void)) {
    struct row *row = &ui.rows[ui.count++];
    row->button = lv_btn_create(ui.screen);
    row->action = action;
    lv_obj_remove_style_all(row->button);
    lv_obj_set_pos(row->button, 12, y);
    lv_obj_set_size(row->button, 216, 50);
    lv_obj_set_style_radius(row->button, 7, 0);
    lv_obj_set_style_bg_opa(row->button, LV_OPA_COVER, 0);
    lv_obj_set_style_border_post(row->button, true, 0);
    label(row->button, title, &lv_font_montserrat_14, color(0xf2f7fb, 0xffffff), 11, 7);
    label(row->button, detail, &lv_font_montserrat_12, color(0xaabecd, 0xffffff), 11, 27);
    label(row->button, ">", &lv_font_montserrat_14, color(0x73e4bc, 0xffff00), 193, 16);
    lv_obj_add_event_cb(row->button, click, LV_EVENT_CLICKED, row);
}

static void save(struct handheld_preferences prefs) {
    if (!ui.platform->save(prefs)) {
        lv_label_set_text(ui.footer, "Settings could not be saved");
        return;
    }
    ui.prefs = prefs;
    show(PAGE_DEVICE, ui.focus);
}

static void brightness(void) {
    struct handheld_preferences prefs = ui.prefs;
    prefs.brightness = prefs.brightness == 100 ? 25 : prefs.brightness + 25;
    save(prefs);
}

static void contrast(void) {
    struct handheld_preferences prefs = ui.prefs;
    prefs.contrast = !prefs.contrast;
    save(prefs);
}

static void rule(unsigned y) {
    lv_obj_t *line = lv_obj_create(ui.screen);
    lv_obj_remove_style_all(line);
    lv_obj_set_pos(line, 12, y);
    lv_obj_set_size(line, WIDTH - 24, 1);
    lv_obj_set_style_bg_color(line, color(0x34495c, 0xffffff), 0);
    lv_obj_set_style_bg_opa(line, LV_OPA_COVER, 0);
}

static void update_diagnostics(void) {
    if (ui.page != PAGE_DEV) return;
    lv_label_set_text_fmt(ui.key_label, "%s  |  %u %s", ui.last_key, ui.key_count,
                          ui.key_count == 1 ? "press" : "presses");
    lv_label_set_text_fmt(ui.keypad_label, "Keypad: %s", ui.keypad ? "ready" : "unavailable");
    lv_label_set_text_fmt(ui.rtc_label, "RTC: %s", ui.rtc);
}

static void show(enum handheld_page page, unsigned focus) {
    static const char *const titles[] = {"System settings", "Device", "Dev Menu", "About"};
    static const char *const subtitles[] = {
        "Local controls and board tools", "Display and accessibility",
        "Keypad events and clock status", "An ESP32-S3 reference build"};
    lv_obj_clean(ui.screen);
    ui.page = page;
    ui.count = 0;
    lv_obj_set_style_bg_color(ui.screen, color(0x0e1a24, 0x000000), 0);
    lv_obj_set_style_bg_opa(ui.screen, LV_OPA_COVER, 0);
    const lv_color_t accent = color(0x73e4bc, 0xffff00);
    const lv_color_t bright = color(0xf2f7fb, 0xffffff);
    const lv_color_t muted = color(0xaabecd, 0xffffff);

    label(ui.screen, "HANDHELD / REFERENCE", &lv_font_montserrat_12, accent, 12, 14);
    label(ui.screen, titles[page], &lv_font_montserrat_24, bright, 12, 36);
    label(ui.screen, subtitles[page], &lv_font_montserrat_12, muted, 12, 68);
    rule(90);

    char detail[32];
    switch (page) {
    case PAGE_SETTINGS:
        add_row(105, "Device", "Brightness and contrast", device);
        add_row(163, "Dev Menu", "Keypad and RTC diagnostics", dev);
        add_row(221, "About", "Target and build version", about);
        break;
    case PAGE_DEVICE:
        snprintf(detail, sizeof(detail), "%u%%  |  Select to cycle", ui.prefs.brightness);
        add_row(105, "Backlight", detail, brightness);
        add_row(163, "High contrast", ui.prefs.contrast ? "On" : "Off", contrast);
        add_row(221, "Back", "System settings", home);
        break;
    case PAGE_DEV:
        label(ui.screen, "LAST KEY", &lv_font_montserrat_12, accent, 12, 109);
        ui.key_label = label(ui.screen, "", &lv_font_montserrat_14, bright, 12, 129);
        ui.keypad_label = label(ui.screen, "", &lv_font_montserrat_14, muted, 12, 165);
        ui.rtc_label = label(ui.screen, "", &lv_font_montserrat_14, muted, 12, 191);
        update_diagnostics();
        add_row(221, "Back", "System settings", home);
        break;
    case PAGE_ABOUT:
        label(ui.screen, "BUILD TARGET", &lv_font_montserrat_12, accent, 12, 109);
        label(ui.screen, ui.platform->target, &lv_font_montserrat_14, bright, 12, 129);
        snprintf(detail, sizeof(detail), "Firmware %s", ui.platform->version);
        label(ui.screen, detail, &lv_font_montserrat_14, muted, 12, 158);
        label(ui.screen, "LVGL 8  /  240 x 320", &lv_font_montserrat_14, muted, 12, 183);
        add_row(221, "Back", "System settings", home);
        break;
    }
    rule(285);
    ui.footer = label(ui.screen, "UP/DOWN   SELECT   BACK", &lv_font_montserrat_12, muted, 12, 297);
    focus_row(focus);
}

void handheld_ui_start(lv_disp_t *display, const struct handheld_platform *platform,
                       struct handheld_preferences prefs) {
    ui.screen = lv_disp_get_scr_act(display);
    ui.platform = platform;
    ui.prefs = prefs;
    lv_obj_clear_flag(ui.screen, LV_OBJ_FLAG_SCROLLABLE);
    strcpy(ui.last_key, "Waiting for input");
    strcpy(ui.rtc, "checking...");
    home();
}

void handheld_ui_key(enum handheld_key key, const char *name) {
    if (ui.page == PAGE_DEV) {
        ui.key_count++;
        snprintf(ui.last_key, sizeof(ui.last_key), "%s", name);
        update_diagnostics();
    }
    switch (key) {
    case KEY_UP:
        focus_row((ui.focus + ui.count - 1) % ui.count);
        break;
    case KEY_DOWN:
        focus_row((ui.focus + 1) % ui.count);
        break;
    case KEY_ENTER:
        ui.rows[ui.focus].action();
        break;
    case KEY_BACK:
        if (ui.page != PAGE_SETTINGS) show(PAGE_SETTINGS, ui.page - PAGE_DEVICE);
        break;
    case KEY_OTHER:
        break;
    }
}

void handheld_ui_status(bool keypad, const char *rtc) {
    if (ui.keypad == keypad && strcmp(ui.rtc, rtc) == 0) return;
    ui.keypad = keypad;
    snprintf(ui.rtc, sizeof(ui.rtc), "%s", rtc);
    update_diagnostics();
}

enum handheld_page handheld_ui_page(void) { return ui.page; }
