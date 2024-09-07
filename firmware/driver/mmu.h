/* ZAK180 Firmaware
 * Z180 MMU driver
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

/* Kernel memory map:
 * 0xF000-0xFFFF common area 1: stack
 * 0xE000-0xEFFF bank area:     scratch
 * 0x0000-0xDFFF common area 0: kernel code/data
 */

/* User memory map:
 * 0xF000-0xFFFF common area 1: stack
 * 0x1000-0xEFFF bank area:     user code/data
 * 0x0000-0x0FFF common area 0: kernel entry point
*/

#ifndef DRIVER_MMU_H_
#define DRIVER_MMU_H_

#include <stdint.h>

/* Map selected page to the bank area,
 * return scratch address */
void *mmu_map_scratch(uint8_t page, uint8_t *old);

void *_mmu_map_scratch(uint8_t page, uint8_t *old);

#endif
