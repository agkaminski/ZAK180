/* ZAK180 Firmaware
 * Z180 MMU driver
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#include <stddef.h>
#include <stdint.h>
#include <z180/z180.h>

#include "mmu.h"

void *mmu_map_scratch(uint8_t page, uint8_t *old)
{
	uint8_t base = (CBAR & 0x0F);

	if (old != NULL) {
		*old = BBR + base;
	}

	BBR = page - base;

	return (void *)((uint16_t)base << 12);
}

/* Return physical page corresponding to the given vaddr */
uint8_t mmu_get_page(void *vaddr)
{
	uint8_t vpage = (uint16_t)vaddr >> 12;
	uint8_t base = CBAR;
	uint8_t ppage;

	if (vpage >= (base >> 4)) {
		/* Common 1 area */
		ppage = CBR;
	}
	else if (vpage >= (base & 0x0F)) {
		/* Bank area */
		ppage = BBR;
	}
	else {
		/* Common 0 area */
		ppage = 0;
	}

	return ppage + vpage;
}
