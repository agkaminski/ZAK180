/* ZAK180 Firmaware
 * AY-3-8912 driver
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#include <z180/z180.h>

#include "ay38912.h"

__sfr __at(0xC0) AY_LATCH;
__sfr __at(0xC1) AY_READ;
__sfr __at(0xC1) AY_WRITE;

unsigned char ay38912_readPort(void)
{
	AY_LATCH = 17;
	return AY_READ;
}

void ay38912_writePort(unsigned char byte)
{
	AY_LATCH = 17;
	AY_WRITE = byte;
}

void ay38912_setPort(unsigned char dir)
{
	AY_LATCH = 7;
	AY_WRITE = AY_READ | (!!dir << 6);
}
