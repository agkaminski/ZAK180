/* ZAK180 Firmaware
 * Keyboard driver
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#include <z180/z180.h>
#include <stdint.h>
#include <stdio.h>

#include "keyboard.h"

__sfr __at(0x0080) COL0;
__sfr __at(0x0081) COL1;
__sfr __at(0x0082) COL2;
__sfr __at(0x0083) COL3;
__sfr __at(0x0084) COL4;
__sfr __at(0x0085) COL5;
__sfr __at(0x0086) COL6;
__sfr __at(0x0087) COL7;
__sfr __at(0x0088) COL8;
__sfr __at(0x0089) COL9;
__sfr __at(0x008A) COL10;
__sfr __at(0x008B) COL11;
__sfr __at(0x008C) COL12;
__sfr __at(0x008D) COL13;
__sfr __at(0x008E) COL14;
__sfr __at(0x008F) COL15;

void keyboard_dump(void)
{
	printf("##########\r\n");
	printf("0:  0x%02x\r\n", COL0);
	printf("1:  0x%02x\r\n", COL1);
	printf("2:  0x%02x\r\n", COL2);
	printf("3:  0x%02x\r\n", COL3);
	printf("4:  0x%02x\r\n", COL4);
	printf("5:  0x%02x\r\n", COL5);
	printf("6:  0x%02x\r\n", COL6);
	printf("7:  0x%02x\r\n", COL7);
	printf("8:  0x%02x\r\n", COL8);
	printf("9:  0x%02x\r\n", COL9);
	printf("10: 0x%02x\r\n", COL10);
	printf("11: 0x%02x\r\n", COL11);
	printf("12: 0x%02x\r\n", COL12);
	printf("13: 0x%02x\r\n", COL13);
	printf("14: 0x%02x\r\n", COL14);
	printf("15: 0x%02x\r\n", COL15);
}
