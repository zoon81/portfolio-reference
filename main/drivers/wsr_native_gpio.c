#include "wsr_native_gpio.h"

#include "assert.h"
#include "driver/ledc.h"
#include "esp_err.h"

#include "board_description.h"

// ─── LCD backlight PWM (LEDC) ───────────────────────────────────────────────
// The backlight LED is switched by an N-channel MOSFET on an active-HIGH gate
// (BOARD_LCD_BACKLIGHT_NATIVE_GPIO_PIN) with an external pull-down. Driving it
// from an LEDC channel gives software brightness control: duty 0 = off,
// duty max = full brightness. Frequency is kept well above the flicker-fusion
// threshold; tune freq/resolution here if you observe whine or banding.
#define WSR_BL_LEDC_MODE LEDC_LOW_SPEED_MODE
#define WSR_BL_LEDC_TIMER LEDC_TIMER_0
#define WSR_BL_LEDC_CHANNEL LEDC_CHANNEL_0
#define WSR_BL_LEDC_RES LEDC_TIMER_8_BIT
#define WSR_BL_LEDC_FREQ_HZ 5000
#define WSR_BL_LEDC_DUTY_MAX ((1u << 8) - 1u) // 8-bit resolution -> 255

void wsr_native_gpio_init(void) {
    ledc_timer_config_t bl_timer = {
        .speed_mode = WSR_BL_LEDC_MODE,
        .timer_num = WSR_BL_LEDC_TIMER,
        .duty_resolution = WSR_BL_LEDC_RES,
        .freq_hz = WSR_BL_LEDC_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    esp_err_t err = ledc_timer_config(&bl_timer);
    assert(err == ESP_OK);

    ledc_channel_config_t bl_channel = {
        .gpio_num = BOARD_LCD_BACKLIGHT_NATIVE_GPIO_PIN,
        .speed_mode = WSR_BL_LEDC_MODE,
        .channel = WSR_BL_LEDC_CHANNEL,
        .timer_sel = WSR_BL_LEDC_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .duty = 0, // start off; the stored brightness is applied at boot
        .hpoint = 0,
    };
    err = ledc_channel_config(&bl_channel);
    assert(err == ESP_OK);
}

void wsr_native_gpio_set_lcd_backlight(uint8_t percent) {
    if (percent > 100) {
        percent = 100;
    }
    const uint32_t duty = ((uint32_t)percent * WSR_BL_LEDC_DUTY_MAX) / 100u;
    esp_err_t err = ledc_set_duty(WSR_BL_LEDC_MODE, WSR_BL_LEDC_CHANNEL, duty);
    assert(err == ESP_OK);
    err = ledc_update_duty(WSR_BL_LEDC_MODE, WSR_BL_LEDC_CHANNEL);
    assert(err == ESP_OK);
}
