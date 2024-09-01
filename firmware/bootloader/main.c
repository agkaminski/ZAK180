/* ZAK180 Firmaware
 * Bootloader main
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#include <string.h>
#include <stdio.h>

#include "../driver/uart.h"
#include "../driver/floppy.h"
#include "../filesystem/fat12.h"

int putchar(int c)
{
	char t = c;

	uart1_write_poll(&t, 1);

	return 1;
}

uint8_t sector[512];
struct fat12_fs fs;
struct fat12_cb cb;


static void dump(size_t base)
{
	for (size_t i = 0; i < 512; i += 32) {
		printf("%06lx: ", i + ((long)base * 512));
		for (size_t j = 0; j < 32; ++j) {
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
/*
	while (1) {
		int ret = floppy_read_sector(0, sector);
		printf("res: %d\r\n", ret);
		dump(0);

		ret = floppy_read_sector(79 * 18 * 2, sector);
		printf("res: %d\r\n", ret);
		dump(79 * 18 * 2);
	}
*/
/*
	for (uint16_t i = 0; i < 80 * 18 * 2; i += (18 * 2)) {
		int ret = floppy_read_sector(i, sector);
		printf("res: %d\r\n", ret);
		dump(i);
	}
*/
/*
	for (uint16_t i = 1; i < 10; ++i) {
		floppy_read_sector(i, sector);
		dump(i);
	}
*/

	cb.read_sector = floppy_read_sector;
	cb.write_sector = floppy_write_sector;
	int ret = fat12_mount(&fs, &cb);
	printf("Mount ret = %d\r\n", ret);

	for (uint16_t i = 0; i < 32; ++i) {
		uint16_t cluster;
		if (fat12_fat_get(&fs, i, &cluster) < 0) {
			printf("FAT get failed\r\n");
			break;
		}

		printf("Cluster %d: %03x\r\n", i, cluster);
	}

	fat12_fat_set(&fs, 24, 0x888);
	fat12_fat_set(&fs, 25, 0x999);
	fat12_fat_set(&fs, 26, 0xAAA);
	fat12_fat_set(&fs, 27, 0xBBB);
	fat12_fat_set(&fs, 28, 0xCCC);
	fat12_fat_set(&fs, 29, 0xDDD);
	fat12_fat_set(&fs, 30, 0xEEE);

	for (uint16_t i = 0; i < 32; ++i) {
		uint16_t cluster;
		if (fat12_fat_get(&fs, i, &cluster) < 0) {
			printf("FAT get failed\r\n");
			break;
		}

		printf("Cluster %d: %03x\r\n", i, cluster);
	}

	for (uint16_t i = 16; i < 32; ++i) {
		if (fat12_fat_set(&fs, i, 0) < 0) {
			printf("FAT set failed\r\n");
			break;
		}
	}

	for (uint16_t i = 0; i < 32; ++i) {
		uint16_t cluster;
		if (fat12_fat_get(&fs, i, &cluster) < 0) {
			printf("FAT get failed\r\n");
			break;
		}

		printf("Cluster %d: %03x\r\n", i, cluster);
	}

	/*
	000200: f0 ff ff 00 40 00 05 60 00 07 80 00 09 a0 00 ff
	000210: 0f 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
	*/

	floppy_access(0);

	return 0;
}
