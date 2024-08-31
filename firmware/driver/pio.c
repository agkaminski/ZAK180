/* ZAK180 Firmaware
 * Z80 PIO driver
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#include <z180/z180.h>

#include "pio.h"

__sfr __at(0xA0) PORTA;
__sfr __at(0xA1) PORTB;
__sfr __at(0xA2) CTRLA;
__sfr __at(0xA3) CTRLB;

void pio_set_iv(enum pio_port port, uint8_t iv)
{
	iv &= (uint8_t)(~1);
	if (port == pio_porta) {
		CTRLA = iv;
	}
	else {
		CTRLB = iv;
	}
}

void pio_set_irq(enum pio_port port, uint8_t options, uint8_t mask)
{
	options &= (uint8_t)(~(1 << 3));
	options |= (unsigned char)0x7;

	if (port == pio_porta) {
		CTRLA = options;
	}
	else {
		CTRLB = options;
	}

	if (options & PIO_IRQ_MASK) {
		if (port == pio_porta) {
			CTRLA = mask;
		}
		else {
			CTRLB = mask;
		}
	}
}

void pio_set_mode(enum pio_port port, uint8_t mode, uint8_t dir)
{
	uint8_t val = (mode << 6) | (uint8_t)0xf;

	if (port == pio_porta) {
		CTRLA = val;
	}
	else {
		CTRLB = val;
	}

	if (mode == PIO_MODE_BIT) {
		if (port == pio_porta) {
			CTRLA = dir;
		}
		else {
			CTRLB = dir;
		}
	}
}

uint8_t pio_get(enum pio_port port)
{
	return (port == pio_porta) ? PORTA : PORTB;
}

void pio_set(enum pio_port port, uint8_t val, uint8_t mask)
{
	if (port == pio_porta) {
		PORTA = (PORTA & (uint8_t)(~mask)) | (val & mask);
	}
	else {
		PORTB = (PORTB & (uint8_t)(~mask)) | (val & mask);
	}
}
