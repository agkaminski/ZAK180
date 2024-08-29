/* ZAK180 Firmaware
 * Z180 MMU driver
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#include <z180/z180.h>

#include "mmu.h"

void *mmu_mapScratch(uint8_t page)
{
	BBR = page;
	return (void *)(((uint16_t)(CBR & 0x0F)) << 12);
}
