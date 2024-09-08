/* ZAK180 Firmaware
 * AY-3-8912 driver
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#include <z180/z180.h>

#include "ay38912.h"

__sfr __at(0x00C0) LATCH;
__sfr __at(0x00C0) READ;
__sfr __at(0x00C1) WRITE;

#define R0 0
#define R1 1
#define R2 2
#define R3 3
#define R4 4
#define R5 5
#define R6 6
#define R7 7
#define R10 8
#define R11 9
#define R12 10
#define R13 11
#define R14 12
#define R15 13
#define R16 14
#define R17 15

unsigned char ay38912_readPort(void)
{
	LATCH = R16;
	return READ;
}

void ay38912_writePort(unsigned char byte)
{
	LATCH = R16;
	WRITE = byte;
}

void ay38912_setPort(unsigned char dir)
{
	LATCH = R7;
	WRITE = READ | (!!dir << 6);
}
