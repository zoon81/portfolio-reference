idf_component_register(
    SRCS "app.c" "ui/handheld_ui.c" "dmx/dmx_monitor.c" "drivers/dmx_uart.c"
         "drivers/wsr_lcd.c" "drivers/wsr_iic.c" "drivers/wsr_native_gpio.c"
         "drivers/ext_io.c" "drivers/kts1622.c" "drivers/kb_io.c"
         "drivers/rtc.c" "drivers/pcf85063a.c"
    INCLUDE_DIRS "ui" "drivers" "dmx"
    PRIV_REQUIRES esp_app_format driver esp_driver_uart esp_lcd esp_timer nvs_flash
)
