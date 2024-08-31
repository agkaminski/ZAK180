/* ZAK180 Firmaware
 * AY-3-8912 driver
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#include <z180/z180.h>

#include "ay38912.h"

__sfr __at(0xC0) LATCH;
__sfr __at(0xC1) READ;
__sfr __at(0xC1) WRITE;

unsigned char ay38912_readPort(void)
{
	LATCH = 17;
	return READ;
}

void ay38912_writePort(unsigned char byte)
{
	LATCH = 17;
	WRITE = byte;
}

void ay38912_setPort(unsigned char dir)
{
	LATCH = 7;
	WRITE = READ | (!!dir << 6);
}
