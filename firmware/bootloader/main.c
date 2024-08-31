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

	printf("Initial read\r\n");
	floppy_cmd_read_data(0, 0, 1, sector, &res);
	printf("res: %02x %02x %02x\r\n", res.st0, res.st1, res.st2);
	dump(0);

	memset(sector, 0, sizeof(sector));

	printf("Writing 00\r\n");
	floppy_cmd_write_data(0, 0, 1, sector, &res);
	printf("res: %02x %02x %02x\r\n", res.st0, res.st1, res.st2);

	printf("Read, expect 00\r\n");
	floppy_cmd_read_data(0, 0, 1, sector, &res);
	printf("res: %02x %02x %02x\r\n", res.st0, res.st1, res.st2);
	dump(0);

	memset(sector, 0xAA, sizeof(sector));

	printf("Writing AA\r\n");
	floppy_cmd_write_data(0, 0, 1, sector, &res);
	printf("res: %02x %02x %02x\r\n", res.st0, res.st1, res.st2);

	printf("Read, expect AA\r\n");
	floppy_cmd_read_data(0, 0, 1, sector, &res);
	printf("res: %02x %02x %02x\r\n", res.st0, res.st1, res.st2);
	dump(0);

	memset(sector, 0x55, sizeof(sector));

	printf("Writing 55\r\n");
	floppy_cmd_write_data(0, 0, 1, sector, &res);
	printf("res: %02x %02x %02x\r\n", res.st0, res.st1, res.st2);

	printf("Read, expect 55\r\n");
	floppy_cmd_read_data(0, 0, 1, sector, &res);
	printf("res: %02x %02x %02x\r\n", res.st0, res.st1, res.st2);
	dump(0);

	floppy_enable(0);


	return 0;
}
