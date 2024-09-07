/* ZAK180 Firmaware
 * VGA driver
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#include <string.h>
#include <z180/z180.h>

#include "vga.h"
#include "ay38912.h"
#include "mmu.h"

#define VGA_PAGE 0xFE
#define VGA_ROWS 60
#define VGA_COLS 80

#define CURSOR   219

static unsigned char g_scroll;
static unsigned char g_cursor_x;
static unsigned char g_cursor_y;
static unsigned char g_cursor_state;
static char g_cursor_prev;

volatile unsigned char g_vsync;

static struct {
	char fifo[32];
	volatile uint8_t wptr;
	volatile uint8_t rptr;
	unsigned char scroll;
	struct {
		unsigned char x;
		unsigned char y;
		unsigned char state;
		unsigned char prev;
		unsigned char counter;
	} cursor;
	unsigned char mmu_save;
	volatile unsigned char vsync;
	volatile char *vram;
	unsigned char rom;
} common;

static void vga_enqueue(char c)
{
	while (((common.wptr + 1) % sizeof(common.fifo)) == common.rptr) {
		/* TODO reschedule */
		__asm halt __endasm;
	}

	common.fifo[common.wptr] = c;
	common.wptr = (common.wptr + 1) % sizeof(common.fifo);
}

static char vga_dequeue(void)
{
	char c = 0;

	if (common.wptr != common.rptr) {
		c = common.fifo[common.rptr];
		common.rptr = (common.rptr + 1) % sizeof(common.fifo);
	}

	return c;
}

static void vga_set(char c)
{
	*(common.vram + ((uint16_t)common.cursor.y << 7) + common.cursor.x) = c;
}

static char vga_get(void)
{
	return *(common.vram + ((uint16_t)common.cursor.y << 7) + common.cursor.x);
}

static void vga_new_line(void)
{
	if (++common.cursor.y >= VGA_ROWS) {
		common.cursor.y = VGA_ROWS - 1;

		/* Clear new row (not yet visible) */
		for (uint16_t i = 0; i < VGA_COLS; ++i) {
			*(common.vram + (128 * VGA_ROWS) + i) = ' ';
		}

		/* Set scroll register */
		ay38912_writePort((common.rom << 6) | ((++common.scroll) & 0x3F));
	}
}

void vga_vblank_handler(void)
{
	char c = vga_dequeue();
	unsigned char nl = 0;

	if (c != 0) {
		common.vram = _mmu_map_scratch(VGA_PAGE, &common.mmu_save);

		if (common.cursor.state) {
			vga_set(common.cursor.prev);
			common.cursor.state = 0;
		}

		do {
			switch (c) {
				/* TODO add cursor movement etc */

				case '\r':
					common.cursor.x = 0;
					break;

				default:
					vga_set(c);
					if (++common.cursor.x < VGA_COLS) {
						break;
					}
					common.cursor.x = 0;
					/* fall though */
				case '\n':
					vga_new_line();
					nl = 1;
					break;
			}

			if (nl) {
				break;
			}

			c = vga_dequeue();
		} while (c != 0);

		_mmu_map_scratch(common.mmu_save, NULL);
	}
	else {
		/* Handle cursor, toggle once*/
		if (++common.cursor.counter >= 26) {
			common.cursor.counter = 0;

			common.vram = _mmu_map_scratch(VGA_PAGE, &common.mmu_save);

			if (common.cursor.state) {
				vga_set(common.cursor.prev);
				common.cursor.state = 0;
			}
			else {
				common.cursor.state = 1;
				common.cursor.prev = vga_get();
				vga_set(CURSOR);
			}

			_mmu_map_scratch(common.mmu_save, NULL);
		}
	}
}

void vga_putchar(char c)
{
	vga_enqueue(c);
}

void vga_clear(void)
{
	/* Select ROM #3 (should be blank) to hide VRAM manipulation
	 * during visible portion */
	ay38912_writePort((0x3 << 6) | (common.scroll & 0x3F));

	/* Fill whole VRAM with spaces */
	common.vram = mmu_map_scratch(VGA_PAGE, &common.mmu_save);
	memset(common.vram, ' ', 8 * 1024);
	mmu_map_scratch(common.mmu_save, NULL);

	/* Restore previous ROM */
	ay38912_writePort((common.rom << 6) | (common.scroll & 0x3F));
}

void vga_select_rom(uint8_t rom)
{
	common.rom = rom;
	ay38912_writePort((common.rom << 6) | (common.scroll & 0x3F));
}

void vga_init(void)
{
	/* Set AY-3-8912 IOA as output */
	ay38912_setPort(1);

	vga_select_rom(0);
	vga_clear();
}
