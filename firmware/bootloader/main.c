/* ZAK180 Firmaware
 * Bootloader main
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#include <string.h>
#include <stdio.h>

#include "../driver/uart.h"
#include "../driver/floppy.h"

int putchar(int c)
{
	char t = c;

	uart1_write_poll(&t, 1);

	return 1;
}

uint8_t sector[512];


static void dump(size_t base)
{
	for (size_t i = 0; i < 512; i += 16) {
		printf("%06lx: ", i + ((long)base * 512));
		for (size_t j = 0; j < 16; ++j) {
			printf("%02x ", sector[i + j]);
		}
		printf("\r\n");
	}
}

int main(void)
{
	uart_init();

	printf("ZAK180 Bootloader rev " VERSION "\r\n");

	if (floppy_init() < 0) {
		printf("Floppy init fail\r\n");
		return 1;
	}

	floppy_access(1);

	for (uint16_t i = 0; i < 80 * 18 * 2; i += (18 * 2)) {
		int ret = floppy_read_sector(i, sector);
		printf("res: %d\r\n", ret);
		dump(i);
	}

	floppy_access(0);

	return 0;
}
