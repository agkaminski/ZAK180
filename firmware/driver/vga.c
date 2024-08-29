/* ZAK180 Firmaware
 * VGA driver
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#include <z180/z180.h>

#include "vga.h"
#include "ay38912.h"
#include "mmu.h"

#define VGA_PAGE_LOW  0xFE
#define VGA_PAGE_HIGH 0xFF
#define VGA_ROWS      60
#define VGA_COLS      80

static unsigned char g_scroll;
static unsigned char g_cursor_x;
static unsigned char g_cursor_y;
static unsigned char g_cursor_state;
static char g_cursor_prev;

volatile unsigned char g_vsync;

void vga_vsync(void)
{
	g_vsync = 1;
	while (g_vsync);
}

static void *vga_accessPrepare(void)
{
	void *addr;

	if (g_cursor_y < 32) {
		addr = mmu_mapScratch(VGA_PAGE_LOW);
		addr = (char *)addr + (g_cursor_y << 5);
	}
	else {
		addr = mmu_mapScratch(VGA_PAGE_HIGH);
		addr = (char *)addr + ((g_cursor_y - 32) << 5);
	}

	vga_vsync();
	return addr;
}

static void vga_set(char c)
{
	char *row = vga_accessPrepare();
	row[g_cursor_x] = c;
}

char vga_get(void)
{
	char *row = vga_accessPrepare();
	return row[g_cursor_x];
}

static void vga_resetCursor(void)
{
	vga_set(g_cursor_prev);
	g_cursor_state = 0;
}

static void vga_newLine(void)
{
	++g_cursor_y;

	if (g_cursor_y == VGA_ROWS) {
		/* Clear new line (out of the screen currently) */
		char *row = vga_accessPrepare();

		for (unsigned char i = 0; i < VGA_COLS; ++i) {
			row[i] = ' ';
		}

		ay38912_writePort(++g_scroll);

		--g_cursor_y;
	}

	g_cursor_x = 0;
}

static uint8_t vga_putLine(const char *line);




void vga_putc(char c)
{
	vga_resetCursor();

	switch (c) {
		case '\t':
			g_cursor_x += 4 - (g_cursor_x & 0x3);
			break;

		case '\n':
			vga_newLine();
			break;

		case '\0':
			break;

		default:
			vga_set(c);
			++g_cursor_x;
	}

	if (g_cursor_x >= VGA_COLS) {
		vga_newLine();
	}
}

void vga_puts(const char *str);

void vga_clear(void);

void vga_handleCursor(void);

void vga_init(void)
{
	/* Set AY-3-8912 IOA as output */
	ay38912_setPort(1);

	/* Select ROM #0 and zero scroll */
	ay38912_writePort(0);

	vga_clear();
}
