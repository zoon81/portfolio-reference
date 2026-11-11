#include "wsr_lcd.h"

#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "lvgl.h"
#include <stdio.h>

#include "board_description.h"
#define WSR_LVGL_TASK_PRIORITY 12

#define WSR_LVGL_TICK_PERIOD_MS 2
#define WSR_LVGL_TASK_MAX_DELAY_MS 25
#define WSR_LVGL_TASK_STACK_SIZE (4 * 1024)

// One partial RGB565 buffer in internal DMA-capable memory.
#define WSR_LCD_DRAW_BUF_LINES 80

static SemaphoreHandle_t wsr_lcd_lvgl_sem;
static lv_disp_draw_buf_t disp_buf;
static lv_disp_drv_t disp_drv;
static lv_disp_t *disp = NULL;
static esp_lcd_panel_handle_t s_panel_handle = NULL;

static bool wsr_lvgl_notify_flush_ready(esp_lcd_panel_io_handle_t panel_io,
                                        esp_lcd_panel_io_event_data_t *edata, void *user_ctx) {
    lv_disp_drv_t *disp_driver = (lv_disp_drv_t *)user_ctx;
    lv_disp_flush_ready(disp_driver);
    return false;
}

static void wsr_lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map) {
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t)drv->user_data;
    esp_err_t err = esp_lcd_panel_draw_bitmap(panel_handle, area->x1, area->y1, area->x2 + 1,
                                              area->y2 + 1, color_map);
    if (err != ESP_OK) {
        lv_disp_flush_ready(drv);
        printf(
            "esp_lcd_panel_draw_bitmap() returned an error: area: x1:%d, x2:%d, y1:%d, y2:%d\r\n",
            area->x1, area->x2, area->y1, area->y2);
    }
}

// Expand dirty rectangles to 8 pixels so each RGB565 row is 16-byte aligned.
#define WSR_LCD_FLUSH_PX_ALIGN 8

static void wsr_lvgl_rounder_cb(lv_disp_drv_t *drv, lv_area_t *area) {
    (void)drv;
    area->x1 &= ~(WSR_LCD_FLUSH_PX_ALIGN - 1);
    area->x2 |= (WSR_LCD_FLUSH_PX_ALIGN - 1);
    // WSR_LCD_H_RES is itself a multiple of the alignment, so this clamp never
    // actually trims today; it keeps the callback correct if the panel width
    // ever changes to something that is not.
    if (area->x2 > WSR_LCD_H_RES - 1) {
        area->x2 = WSR_LCD_H_RES - 1;
    }
}

static void wsr_lcd_lvgl_increase_tick(void *arg) {
    /* Tell LVGL how many milliseconds has elapsed */
    lv_tick_inc(WSR_LVGL_TICK_PERIOD_MS);
}

static void wsr_lvgl_runner_task(void *arg) {
    while (1) {
        uint32_t delay_ms = WSR_LVGL_TASK_MAX_DELAY_MS;
        if (wsr_lcd_lvgl_lock(0)) {
            delay_ms = lv_timer_handler();
            wsr_lcd_lvgl_unlock();
        }
        if (delay_ms > WSR_LVGL_TASK_MAX_DELAY_MS) {
            delay_ms = WSR_LVGL_TASK_MAX_DELAY_MS;
        }
        const TickType_t ticks = pdMS_TO_TICKS(delay_ms);
        vTaskDelay(ticks ? ticks : 1);
    }
}

static void wsr_lcd_bus_init(esp_lcd_panel_io_handle_t *io_handle, void *user_ctx) {
    esp_lcd_i80_bus_config_t bus_config = {
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .dc_gpio_num = BOARD_LCD_DCX_NATIVE_GPIO_PIN,
        .wr_gpio_num = BOARD_LCD_WRX_NATIVE_GPIO_PIN,
        .data_gpio_nums = {BOARD_LCD_DATA_0_NATIVE_GPIO_PIN, BOARD_LCD_DATA_1_NATIVE_GPIO_PIN,
                           BOARD_LCD_DATA_2_NATIVE_GPIO_PIN, BOARD_LCD_DATA_3_NATIVE_GPIO_PIN,
                           BOARD_LCD_DATA_4_NATIVE_GPIO_PIN, BOARD_LCD_DATA_5_NATIVE_GPIO_PIN,
                           BOARD_LCD_DATA_6_NATIVE_GPIO_PIN, BOARD_LCD_DATA_7_NATIVE_GPIO_PIN},
        .bus_width = 8,
        .max_transfer_bytes = WSR_LCD_H_RES * WSR_LCD_DRAW_BUF_LINES * sizeof(uint16_t),
        .dma_burst_size = 16,
    };
    esp_lcd_i80_bus_handle_t i80_bus = NULL;
    ESP_ERROR_CHECK(esp_lcd_new_i80_bus(&bus_config, &i80_bus));

    esp_lcd_panel_io_i80_config_t io_config = {
        .cs_gpio_num = BOARD_LCD_CSX_NATIVE_GPIO_PIN,
        .pclk_hz = WSR_LCD_PIXEL_CLOCK_HZ,
        .trans_queue_depth = 1,
        .dc_levels =
            {
                .dc_idle_level = 0,
                .dc_cmd_level = 0,
                .dc_dummy_level = 1,
                .dc_data_level = 1,
            },
        .flags = {.swap_color_bytes = !LV_COLOR_16_SWAP,
                  .cs_active_high = 0,
                  .pclk_idle_low = 0,
                  .pclk_active_neg = 0},
        .on_color_trans_done = wsr_lvgl_notify_flush_ready,
        .user_ctx = user_ctx,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i80(i80_bus, &io_config, io_handle));
}

static void init_lcd_panel(esp_lcd_panel_io_handle_t io_handle, esp_lcd_panel_handle_t *panel) {
    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = -1,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .data_endian = LCD_RGB_DATA_ENDIAN_BIG,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));

    esp_lcd_panel_reset(panel_handle);
    esp_lcd_panel_init(panel_handle);
    esp_lcd_panel_invert_color(panel_handle, true);
    esp_lcd_panel_set_gap(panel_handle, 0, 0);
    *panel = panel_handle;
}

lv_disp_t *wsr_lcd_init(void) {
    esp_lcd_panel_io_handle_t io_handle = NULL;
    wsr_lcd_bus_init(&io_handle, &disp_drv);

    init_lcd_panel(io_handle, &s_panel_handle);

    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(s_panel_handle, true));

    lv_init();
    uint32_t draw_buf_alloc_caps = 0;
    lv_color_t *buf1 = esp_lcd_i80_alloc_draw_buffer(
        io_handle, WSR_LCD_H_RES * WSR_LCD_DRAW_BUF_LINES * sizeof(lv_color_t),
        draw_buf_alloc_caps);
    assert(buf1 != NULL);
    lv_disp_draw_buf_init(&disp_buf, buf1, NULL, WSR_LCD_H_RES * WSR_LCD_DRAW_BUF_LINES);

    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = WSR_LCD_H_RES;
    disp_drv.ver_res = WSR_LCD_V_RES;
    disp_drv.flush_cb = wsr_lvgl_flush_cb;
    disp_drv.rounder_cb = wsr_lvgl_rounder_cb;
    disp_drv.draw_buf = &disp_buf;
    disp_drv.user_data = s_panel_handle;
    disp = lv_disp_drv_register(&disp_drv);

    const esp_timer_create_args_t lvgl_tick_timer_args = {.callback = &wsr_lcd_lvgl_increase_tick,
                                                          .name = "lvgl_tick"};
    esp_timer_handle_t lvgl_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, WSR_LVGL_TICK_PERIOD_MS * 1000));

    wsr_lcd_lvgl_sem = xSemaphoreCreateRecursiveMutex();
    assert(wsr_lcd_lvgl_sem);
    BaseType_t err = xTaskCreate(wsr_lvgl_runner_task, "LVGL_RUNNER", WSR_LVGL_TASK_STACK_SIZE,
                                 NULL, WSR_LVGL_TASK_PRIORITY, NULL);
    assert(err == pdPASS);
    return disp;
}

bool wsr_lcd_lvgl_lock(int timeout_ms) {
    // Convert timeout in milliseconds to FreeRTOS ticks
    const TickType_t timeout_ticks = (timeout_ms == -1) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTakeRecursive(wsr_lcd_lvgl_sem, timeout_ticks) == pdTRUE;
}

bool wsr_lcd_lvgl_unlock(void) { return xSemaphoreGiveRecursive(wsr_lcd_lvgl_sem) == pdTRUE; }
