/* ZAK180 Firmaware
 * Critical section
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#include <z180/z180.h>
#include <stdint.h>

static volatile int8_t active;

void critical_start(void)
{
	if (active) {
		__asm di __endasm;
	}
}

void critical_end(void)
{
	if (active) {
		__asm ei __endasm;
	}
}

void critical_enable(void)
{
	active = 1;
}
