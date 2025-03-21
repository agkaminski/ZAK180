/* ZAK180 Firmaware
 * VGA driver
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

/* General software architecture:
 * Double buffering is used (with only visible
 * portion of the row stored), all modifications
 * are performed on the buffer, with no timing
 * requirements (apart from some variables that
 * need to be protected by a critical section).
 * Modified row is then marked as dirty, which
 * promts IRQ handler to sync it with the VRAM
 * itself. This is done via DMA channel 0 to
 * make sure we can copy data in time before
 * visible portion sets in. Also, because of
 * this, only a limited number of lines may be
 * updated per vblank event.
 * Thanks to usage of DMA we do not have to
 * ever map the VRAM to the logical address
 * space. Downside is usage of ~5 KB for the
 * buffer. This sollution might be optimised
 * for memory usage by keeping only dirty lines
 * in the buffer, although some mechanism of
 * fetching data from VRAM would be required
 * instead. Act of fetching data would block
 * the writer until next vblank, so this
 * solution seem suboptimal. */

#include <string.h>
#include <stdint.h>

#include "vga.h"
#include "ay38912.h"
#include "mmu.h"
#include "critical.h"
#include "dma.h"

#define VGA_PAGE 0xFE
#define VGA_ROWS 60
#define VGA_COLS 80

#define CURSOR   219

#define TAB_SIZE 4

static struct {
	uint8_t vbuffer[64][80];
	uint8_t vdirty[64 / 8];
	struct {
		volatile uint8_t x;
		volatile uint8_t y;
		volatile uint8_t state;
		volatile uint8_t prev;
		volatile uint8_t counter;
		volatile uint8_t enable;
	} cursor;
	uint8_t rom;
	uint8_t scroll;
	uint8_t late_irq;
} common;

static void vga_mark_dirty(uint8_t line)
{
	common.vdirty[line >> 3] |= (1 << (line & 0x7));
}

static void _vga_set(char c)
{
	uint8_t y = (common.cursor.y + common.scroll) & 0x3F;
	common.vbuffer[y][common.cursor.x] = c;
	vga_mark_dirty(y);
}

static void vga_set(char c)
{
	critical_start();
	_vga_set(c);
	critical_end();
}

static char vga_get(void)
{
	uint8_t y = (common.cursor.y + common.scroll) & 0x3F;
	return common.vbuffer[y][common.cursor.x];
}

static void vga_sync_line(uint8_t line)
{
	void *bpos = common.vbuffer[line];
	uint8_t bpage = mmu_get_page(bpos);
	uint16_t offs = (uint16_t)bpos & 0xFFF;
	uint16_t doffs = (uint16_t)line << 7;

	_dma_memcpy(VGA_PAGE, doffs, bpage, offs, 80);
}

void _vga_late_irq(void)
{
	common.late_irq = 1;
}

void vga_vblank_handler(void)
{
	uint8_t limit = 4;

	/* Check if there was FDD access, that might've
	 * caused vblank IRQ to be serviced waay late.
	 * Service it next time to avoid VGA artifacts. */
	if (common.late_irq) {
		common.late_irq = 0;
		return;
	}

	for (uint8_t i = 0; (i < sizeof(common.vdirty)) && limit; ++i) {
		if (common.vdirty[i]) {
			uint8_t mask = 1;
			for (uint8_t j = 0; (j < 8) && limit; ++j) {
				if (common.vdirty[i] & mask) {
					vga_sync_line((i << 3) + j);
					--limit;
					common.vdirty[i] &= ~mask;
				}
				mask <<= 1;
			}
		}
	}

	ay38912_writePort((common.rom << 6) | (common.scroll & 0x3F));

	if (++common.cursor.counter > 28) {
		common.cursor.counter = 0;

		if (common.cursor.state) {
			_vga_set(common.cursor.prev);
			common.cursor.state = 0;
		}
		else if (common.cursor.enable) {
			common.cursor.prev = vga_get();
			_vga_set(CURSOR);
			common.cursor.state = 1;
		}
	}
}

static void vga_new_line(void)
{
	if (++common.cursor.y >= VGA_ROWS) {
		common.cursor.y = VGA_ROWS - 1;

		/* Clear new row (not yet visible) */
		uint8_t y = (VGA_ROWS + common.scroll) & 0x3F;
		memset(common.vbuffer[y], ' ', VGA_COLS);
		vga_mark_dirty(y);
		++common.scroll;
	}
}

void vga_putchar(char c)
{
	critical_start();
	if (common.cursor.state) {
		_vga_set(common.cursor.prev);
		common.cursor.state = 0;
	}
	common.cursor.counter = 0;
	uint8_t cursor_enabled = common.cursor.enable;
	common.cursor.enable = 0;
	critical_end();

	switch (c) {
		/* TODO add terminal control etc. */
		case '\t':
			common.cursor.x += TAB_SIZE;
			common.cursor.x -= common.cursor.x % TAB_SIZE;
			break;

		case '\r':
			common.cursor.x = 0;
			break;

		case '\n':
			vga_new_line();
			break;

		default:
			vga_set(c);
			if (++common.cursor.x >= VGA_COLS) {
				common.cursor.x = 0;
				vga_new_line();
			}
			break;
	}

	common.cursor.enable = cursor_enabled;
}

void _vga_clear(void)
{
	critical_start();
	common.cursor.counter = 0;
	common.cursor.prev = ' ';
	common.cursor.state = 0;
	memset(common.vbuffer, ' ', sizeof(common.vbuffer));
	memset(common.vdirty, 0xFF, sizeof(common.vdirty));
	common.cursor.x = 0;
	common.cursor.y = 0;
	critical_end();
}

void vga_select_rom(uint8_t rom)
{
	common.rom = rom;
	ay38912_writePort((common.rom << 6) | (common.scroll & 0x3F));
}

void vga_set_cursor(uint8_t enable)
{
	common.cursor.enable = enable;
}

void vga_init(void)
{
	/* Set AY-3-8912 IOA as output */
	ay38912_setPort(1);

	vga_select_rom(0);
	_vga_clear();
	vga_set_cursor(1);
}
