#pragma once

// Rev 2 handheld board: GPIO numbers and 7-bit I2C addresses.
#define BOARD_LCD_CSX_NATIVE_GPIO_PIN 4        // LCD chip select, active low
#define BOARD_LCD_DCX_NATIVE_GPIO_PIN 5        // LCD data/command select, 1 = Data, 0 = Command
#define BOARD_LCD_WRX_NATIVE_GPIO_PIN 6        // LCD write enable, active HIGH
#define BOARD_LCD_DATA_0_NATIVE_GPIO_PIN 7     // LCD parallel bus BIT_0, connected to LCD D08
#define BOARD_LCD_DATA_1_NATIVE_GPIO_PIN 8     // LCD parallel bus BIT_1, connected to LCD D09
#define BOARD_LCD_DATA_2_NATIVE_GPIO_PIN 9     // LCD parallel bus BIT_2, connected to LCD D10
#define BOARD_LCD_DATA_3_NATIVE_GPIO_PIN 10    // LCD parallel bus BIT_3, connected to LCD D11
#define BOARD_LCD_DATA_4_NATIVE_GPIO_PIN 11    // LCD parallel bus BIT_4, connected to LCD D12
#define BOARD_LCD_DATA_5_NATIVE_GPIO_PIN 12    // LCD parallel bus BIT_5, connected to LCD D13
#define BOARD_LCD_DATA_6_NATIVE_GPIO_PIN 13    // LCD parallel bus BIT_6, connected to LCD D14
#define BOARD_LCD_DATA_7_NATIVE_GPIO_PIN 14    // LCD parallel bus BIT_7, connected to LCD D15
#define BOARD_LCD_BACKLIGHT_NATIVE_GPIO_PIN 21 // Backlight driver gate, active HIGH
#define BOARD_IIC_SDA_NATIVE_GPIO_PIN 17       // IIC Serial Data
#define BOARD_IIC_SCL_NATIVE_GPIO_PIN 18       // IIC Serial Clock
#define BOARD_EXT_IO_IIC_ADDRESS 0x20
#define BOARD_KB_IO_IIC_ADDRESS 0x22
#define BOARD_KB_IO_0_NATIVE_GPIO_PIN 37 // GPIO 0 Keyboard module interface
#define BOARD_KB_IO_1_NATIVE_GPIO_PIN                                                              \
    41 // GPIO 1 Keyboard module interface, can be multiplexed with JTAG
#define BOARD_KB_IO_2_NATIVE_GPIO_PIN                                                              \
    42 // GPIO 2 Keyboard module interface, can be multiplexed with JTAG
#define BOARD_KB_IO_3_EXT_GPIO_0_PIN 6 // GPIO 3 Keyboard module interface
#define BOARD_FUEL_GAUGE_IIC_ADDRESS 0x55
#define BOARD_FUEL_GAUGE_BACKUP_IIC_ADDRESS 0x0B
#define BOARD_LCD_RST_EXT_GPIO_1_PIN 1   // Reset LCD, active LOW
#define BOARD_LCD_RDX_EXT_GPIO_1_PIN 2   // Read strobe, idle HIGH
#define BOARD_CC_CHRG_EXT_GPIO_0_PIN 0   // Charge Controller Charge in progress PIN, active LOW
#define BOARD_CC_CHR_EN_EXT_GPIO_0_PIN 2 // Charge Controller charge enable, active LOW
#define BOARD_CC_CHR_PRESET_1_EXT_GPIO_0_PIN 3 // Charge Controller charge preset 1
#define BOARD_CC_CHR_PRESET_2_EXT_GPIO_0_PIN 4 // Charge Controller charge preset 2
#define BOARD_RS485_UART_RXD_NATIVE_GPIO_PIN 38
#define BOARD_RS485_RX_EN_EXT_GPIO_1_PIN                                                           \
    4 // RS485 receiver enable, pulled high externally, active LOW
#define BOARD_RS485_TX_EN_EXT_GPIO_1_PIN                                                           \
    5                              // RS485 transmitter enable, pulled low externally, active HIGH
#define BOARD_RTC_IIC_ADDRESS 0x51 // RTC IIC address

enum board_iic_device_list {
    BOARD_IIC_NODE_EXT_IO,
    BOARD_IIC_NODE_KB_IO,
    BOARD_IIC_NODE_RTC,
    BOARD_IIC_NODE_COUNT
};
