#include "handheld_ui.h"

#include <stdio.h>

#include "board_description.h"
#include "dmx_uart.h"
#include "driver/gpio.h"
#include "esp_app_desc.h"
#include "esp_log.h"
#include "ext_io.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "kb_board_description.h"
#include "kb_io.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "rtc.h"
#include "wsr_lcd.h"
#include "wsr_native_gpio.h"

static const char *TAG = "handheld";
static nvs_handle_t storage;
static bool storage_ready;

static bool save_preferences(struct handheld_preferences prefs) {
    uint16_t value = prefs.brightness | (prefs.contrast ? 0x100 : 0);
    if (!storage_ready || nvs_set_u16(storage, "display", value) != ESP_OK ||
        nvs_commit(storage) != ESP_OK) {
        return false;
    }
    wsr_native_gpio_set_lcd_backlight(prefs.brightness);
    return true;
}

static struct handheld_preferences load_preferences(void) {
    struct handheld_preferences prefs = {.brightness = 75};
    // Never erase the partition automatically if its layout or version differs.
    if (nvs_flash_init() != ESP_OK || nvs_open("handheld_ref", NVS_READWRITE, &storage) != ESP_OK) {
        ESP_LOGW(TAG, "Preferences unavailable; using defaults");
        return prefs;
    }
    storage_ready = true;
    uint16_t value;
    if (nvs_get_u16(storage, "display", &value) == ESP_OK && (value & 0xfe00) == 0) {
        const unsigned brightness = value & 0xff;
        if (brightness >= 25 && brightness <= 100 && brightness % 25 == 0) {
            prefs.brightness = brightness;
            prefs.contrast = (value & 0x100) != 0;
        }
    }
    return prefs;
}

static void report_keys(uint16_t presses) {
    static const struct {
        unsigned bit;
        enum handheld_key key;
        const char *name;
    } map[] = {
        {KB_BOARD_P0_BTN_L_SPEC, KEY_BACK, "Left special / Back"},
        {KB_BOARD_P0_BTN_L_SEL, KEY_OTHER, "Left select"},
        {KB_BOARD_P0_BTN_LEFT, KEY_OTHER, "Left"},
        {KB_BOARD_P0_BTN_DOWN, KEY_DOWN, "Down"},
        {KB_BOARD_P0_BTN_ENTER, KEY_ENTER, "Enter"},
        {KB_BOARD_P0_BTN_UP, KEY_UP, "Up"},
        {KB_BOARD_P0_BTN_RIGHT, KEY_OTHER, "Right"},
        {KB_BOARD_P0_BTN_R_SPEC, KEY_OTHER, "Right special"},
        {8 + KB_BOARD_P1_BTN_R_SEL, KEY_OTHER, "Right select"},
    };
    if (!presses || !wsr_lcd_lvgl_lock(-1)) return;
    for (unsigned i = 0; i < sizeof(map) / sizeof(map[0]); i++) {
        if (presses & (1u << map[i].bit)) handheld_ui_key(map[i].key, map[i].name);
    }
    wsr_lcd_lvgl_unlock();
}

void app_main(void) {
    const struct handheld_preferences prefs = load_preferences();
    wsr_native_gpio_init();
    if (!ext_io_init() || !ext_io_LCD_RST_set()) {
        ESP_LOGE(TAG, "LCD control expander unavailable");
        return;
    }
    vTaskDelay(pdMS_TO_TICKS(20));
    if (!ext_io_LCD_RST_clr()) {
        ESP_LOGE(TAG, "LCD reset could not be released");
        return;
    }
    vTaskDelay(pdMS_TO_TICKS(120));

    // Keep the keypad auxiliary open-drain line released.
    ESP_ERROR_CHECK(gpio_set_level(BOARD_KB_IO_1_NATIVE_GPIO_PIN, 1));
    ESP_ERROR_CHECK(gpio_set_direction(BOARD_KB_IO_1_NATIVE_GPIO_PIN, GPIO_MODE_OUTPUT_OD));
    const bool keypad = kb_io_init();
    const bool rtc = wsr_rtc_init();
    lv_disp_t *display = wsr_lcd_init();
    dmx_uart_start();
    static struct handheld_platform platform = {
        .target = "ESP32-S3 / Board rev 2",
        .save = save_preferences,
        .read_dmx = dmx_uart_read,
        .dmx_source = "UART2 / RS-485 input",
    };
    platform.version = esp_app_get_description()->version;
    if (wsr_lcd_lvgl_lock(-1)) {
        handheld_ui_start(display, &platform, prefs);
        wsr_lcd_lvgl_unlock();
    }
    wsr_native_gpio_set_lcd_backlight(prefs.brightness);

    uint16_t stable = 0, previous = 0;
    TickType_t next_status = xTaskGetTickCount();
    bool key_ok = keypad;
    for (;;) {
        uint16_t sample;
        if (keypad) {
            key_ok = kb_io_read_buttons(&sample);
            if (key_ok) {
                sample &= 0x01ff;
                // Accept a changed mask after two consecutive samples, then emit
                // only newly pressed edges. Held keys never repeat.
                if (sample == previous && sample != stable) {
                    report_keys(sample & ~stable);
                    stable = sample;
                }
                previous = sample;
            } else {
                previous = stable;
            }
        }
        const TickType_t now = xTaskGetTickCount();
        if ((int32_t)(now - next_status) >= 0) {
            char clock[32] = "unavailable or unset";
            struct pcf85063a_time t;
            if (rtc && wsr_rtc_get_time(&t)) {
                snprintf(clock, sizeof(clock), "%02u:%02u:%02u", t.hours, t.minutes, t.seconds);
            }
            if (wsr_lcd_lvgl_lock(-1)) {
                handheld_ui_status(key_ok, clock);
                wsr_lcd_lvgl_unlock();
            }
            next_status = now + pdMS_TO_TICKS(1000);
        }
        vTaskDelay(pdMS_TO_TICKS(15));
    }
}
