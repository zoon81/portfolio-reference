#include "dmx_uart.h"

#include "board_description.h"
#include "driver/uart.h"
#include "esp_intr_alloc.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "ext_io.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#define DMX_UART UART_NUM_2
enum { RX_BUFFER_BYTES = 1024, EVENT_QUEUE_DEPTH = 48 };
static QueueHandle_t events;
static QueueHandle_t snapshots;

static uint32_t now_ms(void) { return (uint32_t)(esp_timer_get_time() / 1000); }

static void recover(struct dmx_monitor *monitor) {
    // Pause FIFO data interrupts while clearing the ring and event queue.
    uart_disable_rx_intr(DMX_UART);
    uart_flush_input(DMX_UART);
    xQueueReset(events);
    uart_enable_rx_intr(DMX_UART);
    dmx_on_overrun(monitor);
}

static void receive(void *arg) {
    (void)arg;
    struct dmx_monitor monitor = {0};
    uint8_t chunk[RX_BUFFER_BYTES];
    for (;;) {
        uart_event_t event;
        const TickType_t wait = monitor.break_pending ? pdMS_TO_TICKS(4) : portMAX_DELAY;
        bool changed = false;
        if (!xQueueReceive(events, &event, wait)) {
            changed = dmx_finish_break(&monitor, now_ms());
        } else if (event.type == UART_DATA) {
            if (event.size > sizeof(chunk) ||
                uart_read_bytes(DMX_UART, chunk, event.size, 0) != (int)event.size) {
                recover(&monitor);
                changed = true;
            } else {
                changed = dmx_on_data(&monitor, chunk, event.size, event.timeout_flag, now_ms());
            }
        } else if (event.type == UART_BREAK) {
            changed = dmx_on_break(&monitor, now_ms());
        } else if (event.type == UART_FIFO_OVF || event.type == UART_BUFFER_FULL) {
            recover(&monitor);
            changed = true;
        }
        if (changed) xQueueOverwrite(snapshots, &monitor.latest);
    }
}

bool dmx_uart_start(void) {
    const uart_config_t config = {
        .baud_rate = 250000,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_2,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    esp_err_t error = uart_param_config(DMX_UART, &config);
    if (error == ESP_OK) {
        error = uart_set_pin(DMX_UART, UART_PIN_NO_CHANGE, BOARD_RS485_UART_RXD_NATIVE_GPIO_PIN,
                             UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    }
    if (error == ESP_OK) {
        error = uart_driver_install(DMX_UART, RX_BUFFER_BYTES, 0, EVENT_QUEUE_DEPTH, &events,
                                    ESP_INTR_FLAG_IRAM);
    }
    if (error != ESP_OK) {
        ESP_LOGW("dmx", "UART initialization failed: %s", esp_err_to_name(error));
        return false;
    }

    snapshots = xQueueCreate(1, sizeof(struct dmx_snapshot));
    if (!snapshots || !ext_io_rs485_receive_enable() ||
        xTaskCreate(receive, "dmx_rx", 4096, NULL, 5, NULL) != pdPASS) {
        uart_driver_delete(DMX_UART);
        if (snapshots) vQueueDelete(snapshots);
        snapshots = NULL;
        ESP_LOGW("dmx", "Receiver could not be started");
        return false;
    }
    return true;
}

enum dmx_status dmx_uart_read(struct dmx_snapshot *snapshot) {
    if (!snapshots) return DMX_UNAVAILABLE;
    if (xQueuePeek(snapshots, snapshot, 0) != pdTRUE) return DMX_WAITING;
    return dmx_snapshot_status(snapshot, now_ms());
}
