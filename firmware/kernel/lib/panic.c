/* ZAK180 Firmaware
 * Panic
 * Copyright: Aleksander Kaminski, 2025
 * See LICENSE.md
 */

#include "hal/cpu.h"
#include "lib/kprintf.h"

void panic(void) __naked
{
	_DI;
	_kprintf("Kernel panic! Halting forever.\r\n");
	while (1) _HALT;
}
