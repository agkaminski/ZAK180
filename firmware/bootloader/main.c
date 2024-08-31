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
struct floppy_result res;


static void dump(size_t base)
{
	for (size_t i = 0; i < 512; i += 16) {
		printf("%04x: ", i + (base * 512));
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

	floppy_init();





	floppy_enable(1);

	floppy_cmd_read_data(0, 0, 1, sector, &res);
	dump(0);
	floppy_cmd_read_data(0, 0, 2, sector, &res);
	dump(1);
	floppy_cmd_read_data(0, 0, 3, sector, &res);
	dump(2);
	floppy_cmd_read_data(0, 0, 4, sector, &res);
	dump(3);

	floppy_cmd_seek(30);

	floppy_cmd_read_data(30, 0, 1, sector, &res);
	dump(0);
	floppy_cmd_read_data(30, 0, 2, sector, &res);
	dump(1);
	floppy_cmd_read_data(30, 0, 3, sector, &res);
	dump(2);
	floppy_cmd_read_data(30, 0, 4, sector, &res);
	dump(3);

	floppy_enable(0);


	return 0;
}
