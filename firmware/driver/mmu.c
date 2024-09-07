/* ZAK180 Firmaware
 * Z180 MMU driver
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#include <z180/z180.h>

#include "mmu.h"
#include "critical.h"

void *mmu_map_scratch(uint8_t page, uint8_t *old)
{
	_CRITICAL_START;
	uint8_t base = (CBR & 0x0F);

	if (old != NULL) {
		*old = BBR + base;
	}

	BBR = page - base;
	_CRITICAL_END;

	return (void *)((uint16_t)base << 12);
}
