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

static uint8_t mem_compare(uint8_t pattern) __naked
{
	(void)pattern;

	__asm
		ld hl, #0x6000
		ld bc, #SCRATCH_SIZE
	00000$:
		cpi
		jr nz, 00001$
		ld e, a
		ld a, b
		or c
		ret z
		ld a, e
		jr 00000$
	00001$:
		ld a, #0xFF
		ret
	__endasm;
}

static int mem_test(uint8_t start, uint8_t end)
{
	static const uint8_t patterns[] = { 0x55, 0xAA, 0x00 };
	uint32_t total = 0;

	for (uint8_t page = start; page < end; page += SCRATCH_SIZE / PAGE_SIZE) {
		uint8_t *mem = mmu_map_scratch(page, NULL);

		for (uint8_t i = 0; i < sizeof(patterns); ++i) {
			uint8_t pattern = patterns[i];
			memset(mem, pattern, SCRATCH_SIZE);
			if (mem_compare(pattern)) {
				printf("\r\nMemory test failed at page 0x%02x\r\n", page);
				return -1;
			}
		}

		total += SCRATCH_SIZE;
		printf("\r%llu bytes OK", total);
	}

	printf("\n");

	return 0;
}

int main(void)
{
	uart_init();
	vga_init();

	printf("ZAK180 Bootloader rev " VERSION " compiled on " DATE "\r\n");

	int ret = mem_test(0x00, 0xE8);
	if (ret < 0) {
		fatal();
	}

	printf("Floppy drive initialisation\r\n");
	ret = floppy_init();
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
