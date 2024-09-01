/* ZAK180 Firmaware
 * 82077 Floppy controller driver
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

/* No interrupts support - busy polling is barely fast enough,
 * no way to do this with interrupts. DMA would be great, though.
 * Z180 DMA is somewhat incompatible with the Intel stuff, but
 * perhaps it can be done with some glue logic. */

#include <stdio.h>
#include <stdint.h>

#include <z180/z180.h>

#include "floppy_cmd.h"

/* 82077 on ZAK180 is in AT mode */

/* Registers */
__sfr __at(0xE2) DOR;  /* Digital Output Register */
__sfr __at(0xE4) MSR;  /* Main Status Register (RO) */
__sfr __at(0xE5) FIFO; /* Data Register (FIFO)*/
__sfr __at(0xE7) DIR;  /* Digital Input Register (RO) */
__sfr __at(0xE7) CCR;  /* Configuration Control Register (WO) */

#define DOR_SELECT_NONE 0x0C
#define DOR_SELECT_0    0x1C
#define DOR_SELECT_1    0x2D
#define DOR_SELECT_2    0x4E
#define DOR_SELECT_3    0x8F

#define CCR_HD144 0 /* 500 kbit/s, default precompensation */

#define DIR_DSKCHG (1 << 7)

#define EOT 0x12
#define GAP 0x18

#define DRIVE_NO 1 /* Using cable without a twist */

static int floopy_cmd_is_busy(void)
{
	return !!(MSR & (1 << DRIVE_NO));
}

static void floppy_cmd_delay(void)
{
	/* Just some random amount of delay */
	for (volatile unsigned int i = 32768; i != 0; ++i);
}

static int floppy_cmd_fifo_write(uint8_t data)
{
	for (uint16_t retry = 0xFFFF; retry != 0; --retry) {
		if ((MSR & (uint8_t)0xC0) == 0x80) {
			FIFO = data;
			return 0;
		}
	}

	return -1;
}

static int floppy_cmd_fifo_read(uint8_t *data)
{
	for (uint16_t retry = 0xFFFF; retry != 0; --retry) {
		if ((MSR & (uint8_t)0xC0) == 0xC0) {
			*data = FIFO;
			return 0;
		}
	}

	return -1;
}

static int floppy_cmd_read_result(struct floppy_cmd_result *res)
{
	if (floppy_cmd_fifo_read(&res->st0) < 0) {
		return -1;
	}

	if (floppy_cmd_fifo_read(&res->st1) < 0) {
		return -1;
	}

	if (floppy_cmd_fifo_read(&res->st2) < 0) {
		return -1;
	}

	if (floppy_cmd_fifo_read(&res->c) < 0) {
		return -1;
	}

	if (floppy_cmd_fifo_read(&res->h) < 0) {
		return -1;
	}

	if (floppy_cmd_fifo_read(&res->r) < 0) {
		return -1;
	}

	if (floppy_cmd_fifo_read(&res->n) < 0) {
		return -1;
	}

	return 0;
}

static int floppy_cmd_write_cmd(uint8_t *cmd, uint8_t len)
{
	for (uint8_t i = 0; i < len; ++i) {
		if (floppy_cmd_fifo_write(cmd[i]) < 0) {
			return -1;
		}
	}

	return 0;
}

/* Slightly optimised function, we barely are able to keep up the pace */
static void floppy_cmd_sector_read(uint8_t *buff)
{
	(void)buff;

	/* c  - loop counter
	 * de - retry counter
	 * hl - buff pointer
	 */

__asm
	ld c, #0
	ld de, #0xFFFF
00000$: /* SDCC local labels suck */
	in0 a, (#_MSR)

	bit 5, a
	jr z, 00004$
	bit 7, a
	jr nz, 00001$

	dec de
	ld a, d
	or e
	jr z, 00004$
	jr 00000$

00001$:
	in0 a, (#_FIFO)
	ld (hl), a
	inc hl
	inc c
	jr nz, 00000$

	ld de, #0xFFFF
00002$:
	in0 a, (#_MSR)

	bit 5, a
	jr z, 00004$
	bit 7, a
	jr nz, 00003$

	dec de
	ld a, d
	or e
	jr z, 00004$
	jr 00002$

00003$:
	in0 a, (#_FIFO)
	ld (hl), a
	inc hl
	inc c
	jr nz, 00002$

00004$:

__endasm;
}

/* TODO this has to be critical */
int floppy_cmd_read_data(uint8_t c, uint8_t h, uint8_t r, uint8_t *buff, struct floppy_cmd_result *res)
{
	uint8_t cmd[] = { 0x46, ((h << 2) | DRIVE_NO), c, h, r, 2, r, GAP, 0xFF };

	if (floppy_cmd_write_cmd(cmd, sizeof(cmd)) < 0) {
		return -1;
	}

	/* Slightly optimised, we are barely able to keep up the pace
	 * No error checking here, but it's ok, we'll fail on read_result */
	floppy_cmd_sector_read(buff);

	/* We lied that the sector is at end of the track, we'll get the result right away */
	/* On success ST0 = 0x41, ST1 = 0x80 (EOT) */

	return floppy_cmd_read_result(res);
}

/* Slightly optimised function, we barely are able to keep up the pace */
static void floppy_cmd_sector_write(const uint8_t *buff)
{
	(void)buff;

	/* c  - loop counter
	 * de - retry counter
	 * hl - buff pointer
	 */

__asm
	ld c, #0
	ld de, #0xFFFF
00000$:
	in0 a, (#_MSR)

	bit 5, a
	jr z, 00004$
	bit 7, a
	jr nz, 00001$

	dec de
	ld a, d
	or e
	jr z, 00004$
	jr 00000$

00001$:
	ld (hl), a
	out0 (#_FIFO), a
	inc hl
	inc c
	jr nz, 00000$

	ld de, #0xFFFF
00002$:
	in0 a, (#_MSR)

	bit 5, a
	jr z, 00004$
	bit 7, a
	jr nz, 00003$

	dec de
	ld a, d
	or e
	jr z, 00004$
	jr 00002$

00003$:
	ld (hl), a
	out0 (#_FIFO), a
	inc hl
	inc c
	jr nz, 00002$

00004$:

__endasm;
}

int floppy_cmd_write_data(uint8_t c, uint8_t h, uint8_t r, const uint8_t *buff, struct floppy_cmd_result *res)
{
	uint8_t cmd[] = { 0x45, ((h << 2) | DRIVE_NO) | 0x80, c, h, r, 2, r, GAP, 0xFF };

	if (floppy_cmd_write_cmd(cmd, sizeof(cmd)) < 0) {
		return -1;
	}

	/* Slightly optimised, we are barely able to keep up the pace
	 * No error checking here, but it's ok, we'll fail on read_result */
	floppy_cmd_sector_write(buff);

	return floppy_cmd_read_result(res);
}

static int floppy_cmd_sense_interrupt(uint8_t *st0, uint8_t *pcn)
{
	uint8_t st0_l = 0, pcn_l = 0;
	uint16_t retry;

	for (retry = 0xFFFF; retry != 0; --retry) {
		if (floppy_cmd_fifo_write(0x08) < 0) {
			return -1;
		}

		if (floppy_cmd_fifo_read(&st0_l) < 0) {
			return -1;
		}

		if (st0_l != 0x80) {
			if (floppy_cmd_fifo_read(&pcn_l) < 0) {
				return -1;
			}
			break;
		}
		else {
			printf("FUCKUP\r\n");
		}
	}

	if (!retry) {
		return -1;
	}

	if (st0 != NULL) {
		*st0 = st0_l;
	}

	if (pcn != NULL) {
		*pcn = pcn_l;
	}

	return 0;
}

int floppy_cmd_recalibrate(void)
{
	uint8_t st0, cmd[] = { 0x07, DRIVE_NO };

	if (floppy_cmd_write_cmd(cmd, sizeof(cmd)) < 0) {
		return -1;
	}

	/* TODO add timeout, long one though */
	while (floopy_cmd_is_busy());
	floppy_cmd_delay();

	if (floppy_cmd_sense_interrupt(&st0, NULL) < 0) {
		return -1;
	}

	if (st0 & 0xC0) {
		return -1;
	}

	return 0;
}

static int floppy_cmd_specify(uint8_t srt, uint8_t hut, uint8_t hlt, uint8_t nd)
{
	uint8_t cmd[] = { 0x03, ((srt << 4) | hut), ((hlt << 1) | nd) };

	if (floppy_cmd_write_cmd(cmd, sizeof(cmd)) < 0) {
		return -1;
	}

	return 0;
}

int floppy_cmd_seek(uint8_t c)
{
	uint8_t cmd[] = { 0x0F, DRIVE_NO, c };
	uint8_t st0;

	if (floppy_cmd_write_cmd(cmd, sizeof(cmd)) < 0) {
		return -1;
	}

	/* TODO add timeout, long one though */
	while (floopy_cmd_is_busy());

	if (floppy_cmd_sense_interrupt(&st0, NULL) < 0) {
		return -1;
	}

	if (st0 & 0xC0) {
		return -1;
	}

	return 0;
}

static int floppy_cmd_configure(uint8_t eis, uint8_t poll, uint8_t fifothr)
{
	uint8_t cmd[] = { 0x13, 0x00, ((eis << 6) | (poll << 4) | fifothr), 0x00 };

	if (floppy_cmd_write_cmd(cmd, sizeof(cmd)) < 0) {
		return -1;
	}

	return 0;
}

static int floppy_cmd_lock(uint8_t lock)
{
	uint8_t readback;

	if (floppy_cmd_fifo_write((lock << 7) | 0x14) < 0) {
		return -1;
	}

	if (floppy_cmd_fifo_read(&readback) < 0) {
		return -1;
	}

	if (readback ^ (lock << 4)) {
		return -1;
	}

	return 0;
}

void floppy_cmd_enable(int8_t enable)
{
	static const uint8_t dorval[] = {
		DOR_SELECT_0,
		DOR_SELECT_1,
		DOR_SELECT_2,
		DOR_SELECT_3
	};

	if (enable) {
		DOR = dorval[DRIVE_NO];
		floppy_cmd_delay();
	}
	else {
		DOR = DOR_SELECT_NONE;
	}
}

/* FIXME DSKCHG not clearing, persistent error */
int floppy_cmd_eject_status(void)
{
	if (DIR & (1 << 7)) {
		/* Media was ejected, clear the flag and
		 * return information. */

		/* Do seek + recalibrate to force movement,
		 * otherwise if we hit the track the controller
		 * thinks we're already at, there will be no
		 * operation and Disk Change flag won't be cleared. */
		if ((floppy_cmd_seek(10) < 0) || (floppy_cmd_recalibrate() < 0)) {
			/* Seek failed, most likely ejected */
			return FLOPPY_CMD_NO_MEDIA;
		}

		return FLOPPY_CMD_CHANGE;
	}

	return FLOPPY_CMD_NO_CHANGE;
}

int floppy_cmd_reset(int warm)
{
	/* Reset */
	DOR = 0;

	floppy_cmd_delay();

	/* Bring out of reset */
	DOR = DOR_SELECT_NONE;

	/* Poll MSR until ready */
	while (MSR != 0x80);

	if (!warm) {
		floppy_cmd_delay();

		if (floppy_cmd_sense_interrupt(NULL, NULL) < 0) {
			return -1;
		}
		if (floppy_cmd_sense_interrupt(NULL, NULL) < 0) {
			return -1;
		}
		if (floppy_cmd_sense_interrupt(NULL, NULL) < 0) {
			return -1;
		}
		if (floppy_cmd_sense_interrupt(NULL, NULL) < 0) {
			return -1;
		}

		CCR = CCR_HD144;

		if (floppy_cmd_configure(1, 1, 15) < 0) {
			return -1;
		}
		if (floppy_cmd_lock(1) < 0) {
			return -1;
		}
	}

	if (floppy_cmd_specify(12, 15, 2, 1) < 0) {
		return -1;
	}

	floppy_cmd_enable(1);
	floppy_cmd_delay();
	if (floppy_cmd_recalibrate() < 0) {
		floppy_cmd_enable(0);
		return -1;
	}
	floppy_cmd_enable(0);

	return 0;
}
