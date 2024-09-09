/* ZAK180 Firmaware
 * Bootloader main
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#include <string.h>
#include <stdio.h>

#include "../driver/uart.h"
#include "../driver/vga.h"
#include "../driver/floppy.h"
#include "../filesystem/fat12.h"
#include "../driver/mmu.h"

#define PAGE_SIZE    (4 * 1024)
#define SCRATCH_SIZE (8 * 1024)

int putchar(int c)
{
	char t = c;

	uart1_write_poll(&t, 1);
	vga_putchar(t);

	return 1;
}

static struct fat12_fs fs;
static const struct fat12_cb cb = {
	.read_sector = floppy_read_sector,
	.write_sector = floppy_write_sector
};

static void fatal(void)
{
	floppy_access(0);

	printf("Fatal error, halt\r\n");

	/* Give some time for vblank to come
	 * and refresh the screen. */
	for (volatile uint16_t i = 0; i < 6000; ++i);

	__asm
		di
		halt
	__endasm;
}

static void kernel_jump(void)
{
	/* Give some time for vblank to come
	 * and refresh the screen. */
	for (volatile uint16_t i = 0; i < 6000; ++i);

	__asm
		di
		ld sp, #0x0000
		jp 0x0000
	__endasm;

	/* Never reached */
}

int main(void)
{
	uart_init();
	vga_init();

	printf("ZAK180 Bootloader rev " VERSION " compiled on " DATE "\r\n");

	printf("Floppy drive initialisation\r\n");
	int ret = floppy_init();
	if (ret < 0) {
		printf("Could not initialise media, please insert the system disk\r\n");
		fatal();
	}

	printf("Mounting filesystem\r\n");
	ret = fat12_mount(&fs, &cb);
	if (ret < 0) {
		printf("No disk or inserted disk is not bootable\r\n");
		fatal();
	}

	struct fat12_file file;
	ret = fat12_file_open(&fs, &file, "/BOOT/KERNEL.IMG");
	if (ret < 0) {
		printf("Could not find the kernel image.\r\nMake sure the kernel is present in /BOOT/KERNEL.IMG\r\n");
		fatal();
	}

	printf("Loading the kernel image...\r\n");
	uint32_t total = 0;
	uint8_t page = 0;
	uint8_t done = 0;
	uint32_t offs = 0;
	do {
		uint8_t *dest = mmu_map_scratch(page, NULL);
		uint16_t left = SCRATCH_SIZE;
		uint16_t pos = 0;
		while (left) {
			int got = fat12_file_read(&fs, &file, dest + pos, SCRATCH_SIZE, offs);
			if (got < 0) {
				printf("File read error %d\r\n", got);
				fatal();
			}
			if (!got) {
				done = 1;
				break;
			}

			left -= got;
			pos += got;
			offs += got;
			total += got;
		}
		printf("\rLoaded %llu bytes", total);
		page += SCRATCH_SIZE / PAGE_SIZE;
	} while (!done);

	floppy_access(0);

	printf("\r\nStarting the kernel...\r\n");

	kernel_jump();

	/* Never reached*/
	return 0;
}
