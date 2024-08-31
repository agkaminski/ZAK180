/* ZAK180 Firmaware
 * Z80 PIO driver
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#ifndef DRIVER_PIO_H_
#define DRIVER_PIO_H_

#include <stdint.h>

#define PIO_IRQ_ENABLE  (1 << 7)
#define PIO_IRQ_DISABLE 0
#define PIO_IRQ_AND     (1 << 6)
#define PIO_IRQ_OR      0
#define PIO_IRQ_HIGH    (1 << 5)
#define PIO_IRQ_LOW     0
#define PIO_IRQ_MASK    (1 << 4)

#define PIO_MODE_OUT 0
#define PIO_MODE_IN  1
#define PIO_MODE_BI  2
#define PIO_MODE_BIT 3

enum pio_port { pio_porta = 0, pio_portb };

void pio_set_iv(enum pio_port port, uint8_t iv);

void pio_set_irq(enum pio_port port, uint8_t options, uint8_t mask);

void pio_set_mode(enum pio_port port, uint8_t mode, uint8_t dir);

uint8_t pio_get(enum pio_port port);

void pio_set(enum pio_port port, uint8_t val, uint8_t mask);

#endif
