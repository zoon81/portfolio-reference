#ifndef INC_WSR_IIC_H
#define INC_WSR_IIC_H

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>

// Default 100KHz clock
#define WSR_IIC_CLOCK_SPEED_DEFAULT_HZ (100 * 1000)
// Fast 400KHz clock
#define WSR_IIC_CLOCK_SPEED_FAST_HZ (400 * 1000)

#define WSR_IIC_CLOCK_SPEED_HZ WSR_IIC_CLOCK_SPEED_DEFAULT_HZ

#define WSR_IIC_TRANSFER_TIMEOUT_MS 100

/*  @brief Initialize IIC bus and devices
 *  @return true on success
 */
bool wsr_iic_0_init(void);

/*  @brief Transfer data to the ext-IO IIC device
 *  @return true on success
 */
bool wsr_iic_0_ext_io_transfer(const uint8_t *tx_data, size_t tx_len, uint8_t *rx_data,
                               size_t rx_len);

/*  @brief Transfer data to the keyboard IIC device
 *  @return true on success
 */
bool wsr_iic_0_kb_io_transfer(const uint8_t *tx_data, size_t tx_len, uint8_t *rx_data,
                              size_t rx_len);

/*  @brief Transfer data to the RTC IIC device
 *  @return true on success
 */
bool wsr_iic_0_rtc_transfer(const uint8_t *tx_data, size_t tx_len, uint8_t *rx_data, size_t rx_len);

/*  @brief Probe the ext-IO IIC device
 *  @return true on success
 */
bool wsr_iic_0_probe_ext_io(void);

/*  @brief Probe the keyboard IIC device
 *  @return true on success
 */
bool wsr_iic_0_probe_kb_io(void);

/*  @brief Probe the RTC IIC device
 *  @return true on success
 */
bool wsr_iic_0_probe_rtc(void);

#endif
