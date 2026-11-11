#ifndef INC_KTS1622_H_
#define INC_KTS1622_H_

#include <stdbool.h>
#include <stdint.h>

#define KTS1622_DIRECTION_OUTPUT(x) (0 << x)
#define KTS1622_DIRECTION_INPUT(x) (1 << x)
#define KTS1622_OUTPUT_PORT_PIN_VALUE_LOW(x) (0 << x)
#define KTS1622_OUTPUT_PORT_PIN_VALUE_HIGH(x) (1 << x)
#define KTS1622_PDPU_SELECTION_PULL_DOWN(x) (0 << x)
#define KTS1622_PDPU_SELECTION_PULL_UP(x) (1 << x)
#define KTS1622_PDPU_ENABLE(x) (1 << x)
#define KTS1622_PDPU_DISABLE(x) (0 << x)
#define KTS1622_IRQ_MASK_ENABLE(x) (0 << x)
#define KTS1622_IRQ_MASK_DISABLE(x) (1 << x)
#define KTS1622_IRQ_EDGE_SELECT_LVL(x) (0 << (x * 2))
#define KTS1622_IRQ_EDGE_SELECT_RISING(x) (1 << (x * 2))
#define KTS1622_IRQ_EDGE_SELECT_FALLING(x) (2 << (x * 2))
#define KTS1622_IRQ_EDGE_SELECT_ANY(x) (3 << (x * 2))

enum kts1622_port { KTS1622_PORT_0 = 0, KTS1622_PORT_1 };

enum kts1622_direction { KTS1622_DIRECTION_OUTPUT = 0, KTS1622_DIRECTION_INPUT };

struct kts1622_dev_hnd {
    bool (*write8)(uint8_t reg, uint8_t value);
    bool (*write16)(uint8_t reg, uint16_t value);
    bool (*read8)(uint8_t reg, uint8_t *value);
    bool (*read16)(uint8_t reg, uint16_t *value);
};

bool kts1622_write_port_direction(const struct kts1622_dev_hnd *dev_hnd, enum kts1622_port port,
                                  uint8_t direction);
bool kts1622_set_port_pin_direction(const struct kts1622_dev_hnd *dev_hnd, enum kts1622_port port,
                                    uint8_t pin, enum kts1622_direction pin_direction);
bool kts1622_write_port_value(const struct kts1622_dev_hnd *dev_hnd, enum kts1622_port port,
                              uint8_t value);
bool kts1622_set_port_pin_value(const struct kts1622_dev_hnd *dev_hnd, enum kts1622_port port,
                                uint8_t pin, bool value);

/** @brief Read the value of an output port
 * @param dev_hnd Pointer to the device handle
 * @param port The port number (0 or 1)
 * @param value Pointer to a uint8_t variable where the port value will be stored
 * @return true if the operation was successful, false otherwise
 */
bool kts1622_read_output_port_value(const struct kts1622_dev_hnd *dev_hnd, enum kts1622_port port,
                                    uint8_t *value);

/** @brief Read the value of an input port
 * @param dev_hnd Pointer to the device handle
 * @param port The port number (0 or 1)
 * @param value Pointer to a uint8_t variable where the port value will be stored
 * @return true if the operation was successful, false otherwise
 *
 */
bool kts1622_read_input_port_value(const struct kts1622_dev_hnd *dev_hnd, enum kts1622_port port,
                                   uint8_t *value);

/** @brief Get the value of a port pin
 * @note This function reads the current state of the specified pin of the output port.
 * @param dev_hnd Pointer to the device handle
 * @param port The port number (0 or 1)
 * @param pin The pin number on the specified port
 * @param value Pointer to a boolean variable where the pin value will be stored
 * @return true if the operation was successful, false otherwise
 */
bool kts1622_get_output_port_pin_value(const struct kts1622_dev_hnd *dev_hnd,
                                       enum kts1622_port port, uint8_t pin, bool *value);

/** @brief Get the value of an input port pin
 * This function reads the current state of the specified pin on the input port. Use it to read a
 * single GPIO pin value
 * @param dev_hnd Pointer to the device handle
 * @param port The port number (0 or 1)
 * @param pin The pin number on the specified port
 * @param value Pointer to a boolean variable where the pin value will be stored
 * @return true if the operation was successful, false otherwise
 */
bool kts1622_get_input_port_pin_value(const struct kts1622_dev_hnd *dev_hnd, enum kts1622_port port,
                                      uint8_t pin, bool *value);

/** @brief Set the pull-up/pull-down enable for a port pin
 * The Pull-up and Pull-down Enable Registers allow the user to enable or disable pull-up/pull-down
 * resistors on the I/O pins. Setting the bit to logic 1 enables the selection of pull-up/pull-down
 * resistors. Setting the bit to logic 0 disconnects the pull-up/pull-down resistors from the I/O
 * pins. Also, the resistors will be disconnected when the outputs are configured as open-drain
 * outputs. Use the pull-up/pull-down registers to select either a pull-up or pull-down resistor.
 * @param dev_hnd Pointer to the device handle
 * @param port The port number
 * @param value The register value to write, see KTS1622_PDPU_ENABLE and KTS1622_PDPU_DISABLE
 * @return true on success
 */
bool kts1622_set_pd_pu_enable(const struct kts1622_dev_hnd *dev_hnd, enum kts1622_port port,
                              uint8_t value);

/** @brief Set the pull-up/pull-down selection for a port pin
 * The I/O port can be configured to have pull-up or pull-down resistor by programming the
 * pull-up/pull-down selection register. Setting a bit to logic 1 selects a 100kΩ pull-up resistor
 * for that I/O pin. Setting a bit to logic 0 selects a 100kΩ pull-down resistor for that I/O pin.
 * If the pull-up/down feature is disconnected, writing to this register will have no effect on I/O
 * pin. Typical value is 100kΩ with minimum of 50kΩ and maximum of 150kΩ.
 * @param dev_hnd Pointer to the device handle
 * @param port The port number
 * @param value The register value to write, see KTS1622_PDPU_SELECTION_PULL_UP and
 * KTS1622_PDPU_SELECTION_PULL_DOWN
 * @return true on success
 */
bool kts1622_set_pd_pu_selection(const struct kts1622_dev_hnd *dev_hnd, enum kts1622_port port,
                                 uint8_t value);

/** @brief Set the interrupt mask for a port pin
 * Interrupt Mask Registers are set to logic 1 upon power-on, disabling interrupts during system
 * start-up. Interrupts may be enabled by setting corresponding mask bits to logic 0.
 * If an input changes state and the corresponding bit in the Interrupt mask register is set to 1,
 * the interrupt is masked and the interrupt pin will not be asserted. If the corresponding bit in
 * the Interrupt mask register is set to 0, the interrupt pin will be asserted. When an input
 * changes state and the resulting interrupt is masked (interrupt mask bit is 1), setting the input
 * mask register bit to 0 will cause the interrupt pin to be asserted. If the interrupt mask bit of
 * an input that is currently the source of an interrupt is set to 1, the interrupt pin will be
 * de-asserted.
 * @param dev_hnd Pointer to the device handle
 * @param port The port number
 * @param value The register value to write, see KTS1622_IRQ_MASK_ENABLE and
 * KTS1622_IRQ_MASK_DISABLE
 * @return true on success
 */
bool kts1622_set_irq_msk(const struct kts1622_dev_hnd *dev_hnd, enum kts1622_port port,
                         uint8_t value);

/** @brief Get the interrupt status for a port pin
 *  The read-only interrupt status registers are used to identify the source of an interrupt.
 * When read, a logic 1 indicates that the corresponding input pin was the source of the interrupt.
 * A logic 0 indicates that the input pin is not the source of an interrupt.
 * When a corresponding bit in the interrupt mask register is set to 1 (masked), the interrupt
 * status bit will return logic 0.
 * @param dev_hnd Pointer to the device handle
 * @param port The port number
 * @param value Pointer to the variable to store the register value
 * @return true on success
 */
bool kts1622_get_irq_status_by_port(const struct kts1622_dev_hnd *dev_hnd, enum kts1622_port port,
                                    uint8_t *value);

/** @brief Get both interrupt status register and combine them into a single 16-bit value
 * @param dev_hnd Pointer to the device handle
 * @param value Pointer to the variable to store the register value
 * @return true on success
 */
bool kts1622_get_irq_status(const struct kts1622_dev_hnd *dev_hnd, uint16_t *value);

/** @brief Set the interrupt edge select for a port pin
 * The Interrupt Edge Registers determine what action on an input pin will cause an interrupt along
 * with the Interrupt Mask registers (4Ah, 4Bh). If the Interrupt is enabled (set ‘0’ in the Mask
 * register) and the action at the corresponding pin matches the required activity, the INT output
 * will become active. The default value for each pin is 00b or level triggered, meaning a level
 * change on the pin will cause an interrupt event. A level triggered action means a change in logic
 * state (HIGH-to-LOW or LOW-to-HIGH), since the last read of the Input Port Register (00h, 01h)
 * which can be latched with a corresponding ‘1’ set in the Input Latch Register (44h, 45h). If the
 * Interrupt Edge Register entry is set to 11b, any edge, positive or negative, causes an interrupt
 * event. If an entry is 01b, only a positive-going edge will cause an interrupt event, while a 10b
 * will require a negative edge to cause an interrupt event. These edge interrupt events are
 * latched, regardless of the status of the Input Latch Register (44h, 45h). These edged interrupts
 * can be cleared in a number of ways: Reading Input Port Registers (00h, 01h) Setting the Interrupt
 * Mask Register (4Ah, 4Bh) to 1 (masked) Setting the Interrupt Clear Register (54h, 55h) to 1 (this
 * is a write-only register) Resetting the Interrupt Edge Register (50h to 53h) back to 0.
 * @param dev_hnd Pointer to the device handle
 * @param port The port number
 * @param value The register value to write, see KTS1622_IRQ_EDGE_SELECT_LVL,
 * KTS1622_IRQ_EDGE_SELECT_RISING, KTS1622_IRQ_EDGE_SELECT_FALLING and KTS1622_IRQ_EDGE_SELECT_ANY
 * @return true on success
 */
bool kts1622_set_irq_edge_select(const struct kts1622_dev_hnd *dev_hnd, enum kts1622_port port,
                                 uint16_t value);

/** @brief Clear the interrupt status for all port pins
 * @param dev_hnd Pointer to the device handle
 * @return true on success
 */
bool kts1622_clear_irq(const struct kts1622_dev_hnd *dev_hnd);
#endif /* INC_KTS1622_H_ */
