/* ZAK180 Firmaware
 * Z180 MMU driver
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#include <stddef.h>
#include <z180/z180.h>

#include "mmu.h"
#include "critical.h"

void *_mmu_map_scratch(uint8_t page, uint8_t *old)
{
	uint8_t base = (CBAR & 0x0F);

	if (old != NULL) {
		*old = BBR + base;
	}

	BBR = page - base;

	return (void *)((uint16_t)base << 12);
}

void *mmu_map_scratch(uint8_t page, uint8_t *old)
{
	void *addr;

	_CRITICAL_START;
	addr = _mmu_map_scratch(page, old);
	_CRITICAL_END;

	return addr;
}
