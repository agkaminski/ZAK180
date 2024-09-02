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

struct fat12_fs fs;
static const struct fat12_cb cb = {
	.read_sector = floppy_read_sector,
	.write_sector = floppy_write_sector
};

static void dump(const uint8_t *buff, size_t bufflen)
{
	for (size_t i = 0; i < bufflen; i += 32) {
		printf("%04x: ", i);
		for (size_t j = 0; j < 32; ++j) {
			printf("%02x ", buff[i + j]);
		}
		printf("\r\n");
	}
}

int main(void)
{
	uart_init();

	printf("ZAK180 Bootloader rev " VERSION "\r\n");

	floppy_init();
	fat12_mount(&fs, &cb);

	struct fat12_file file;

	int ret = fat12_file_open(&fs, &file, "/TEST");
	printf("open ret %d\r\n", ret);

	printf("file cluster: %u, size: %llu\r\n", file.dentry.cluster, file.dentry.size);

	if (ret == 0) {
		uint8_t buff[64];
		ret = fat12_file_read(&fs, &file, buff, sizeof(buff), 0);
		printf("read ret %d\r\n", ret);
		dump(buff, sizeof(buff));
	}

	ret = fat12_file_open(&fs, &file, "/TEST2.EXT");
	printf("open ret %d\r\n", ret);

	printf("file cluster: %u, size: %llu\r\n", file.dentry.cluster, file.dentry.size);

	if (ret == 0) {
		uint8_t buff[64];
		ret = fat12_file_read(&fs, &file, buff, sizeof(buff), 0);
		printf("read ret %d\r\n", ret);
		dump(buff, sizeof(buff));
	}

	ret = fat12_file_truncate(&fs, &file, 1024);
	printf("truncate ret: %d\r\n", ret);

	printf("file cluster: %u, size: %llu\r\n", file.dentry.cluster, file.dentry.size);

	for (uint8_t i = 0; i < 100; ++i) {
		ret = fat12_file_write(&fs, &file, "Ala ma kota. ", 13, (uint16_t)i * 13);
		printf("write @%u ret: %d\r\n", (uint16_t)i * 13, ret);
	}

	floppy_access(0);

	return 0;
}
