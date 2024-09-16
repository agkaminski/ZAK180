/* ZAK180 Firmaware
 * Programmable Reload Timer driver
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#include <stdint.h>
#include <z180/z180.h>

#include "prt.h"
#include "critical.h"

#define CPU_FREQ (12288000UL / 2)

uint16_t _prt0_timer_get(void)
{
	uint16_t ret = TMDR0L;
	return ret | ((uint16_t)TMDR0H << 8);
}

uint16_t _prt1_timer_get(void)
{
	uint16_t ret = TMDR1L;
	return ret | ((uint16_t)TMDR1H << 8);
}

uint16_t prt0_timer_get(void)
{
	_CRITICAL_START;
	uint16_t ret = _prt0_timer_get();
	_CRITICAL_END;

	return ret;
}

uint16_t prt1_timer_get(void)
{
	_CRITICAL_START;
	uint16_t ret = _prt1_timer_get();
	_CRITICAL_END;

	return ret;
}

uint32_t prt_val_to_ms(uint16_t val)
{
	return ((uint32_t)val * 1000) / (CPU_FREQ / 20);
}

void prt0_init(uint8_t interval)
{
	/* Disable timer and its interrupt */
	TCR &= ~((1 << 4) | 1);

	uint16_t reload = ((CPU_FREQ / 20) * (uint32_t)interval) / 1000;

	RLDR0L = reload & 0xFF;
	RLDR0H = reload >> 8;

	/* Enable timer and its interrupt */
	TCR |= (1 << 4) | 1;
}

void prt1_init(uint8_t interval)
{
	/* Disable timer and its interrupt */
	TCR &= ~((1 << 5) | (1 << 1));

	uint16_t reload = ((CPU_FREQ / 20) * (uint32_t)interval) / 1000;

	RLDR1L = reload & 0xFF;
	RLDR1H = reload >> 8;

	/* Enable timer and its interrupt */
	TCR |= (1 << 5) | (1 << 1);
}
