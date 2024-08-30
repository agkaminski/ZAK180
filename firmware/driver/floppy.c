/* ZAK180 Firmaware
 * 82077 Floppy controller driver
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#include <stdio.h>
#include <stdint.h>

#include "floppy.h"

/* 82077 on ZAK180 is in AT mode */

/* Registers */
__sfr __at(0xE2) DOR;  /* Digital Output Register */
__sfr __at(0xE4) MSR;  /* Main Status Register (RO) */
__sfr __at(0xE4) DSR;  /* Data Rate Select Register (WO) */
__sfr __at(0xE5) FIFO; /* Data Register (FIFO)*/
__sfr __at(0xE7) DIR;  /* Digital Input Register (RO) */

#define DOR_SELECT_NONE 0x40
#define DOR_SELECT_0    0x1C
#define DOR_SELECT_1    0x2D
#define DOR_SELECT_2    0x4E
#define DOR_SELECT_3    0x8F

#define MSR_RQM       (1 << 7)
#define MSR_DIO       (1 << 6)
#define MSR_NON_DMA   (1 << 5)
#define MSR_CMD_PROG  (1 << 4)
#define MSR_DRV3_BUSY (1 << 3)
#define MSR_DRV2_BUSY (1 << 2)
#define MSR_DRV1_BUSY (1 << 1)
#define MSR_DRV0_BUSY (1 << 0)

#define DSR_HD144 0 /* 500 kbit/s, default precompensation */

#define DIR_DSKCHG (1 << 7)

#define EOT 0x12
#define GAP 0x18

struct cmd_result {
	uint8_t st0;
	uint8_t st1;
	uint8_t st2;
	uint8_t c;
	uint8_t h;
	uint8_t r;
	uint8_t n;
};

void floppy_dumpregs(void)
{
	printf("82077 registers:\r\n");
	printf("DOR  (0xE2): 0x%02x\r\n", DOR);
	printf("MSR  (0xE4): 0x%02x\r\n", MSR);
	printf("FIFO (0xE5): 0x%02x\r\n", FIFO);
	printf("DIR  (0xE7): 0x%02x\r\n", DIR);
}

static int floppy_fifo_write(uint8_t data)
{
	uint8_t msr;

	/* TODO add timeout */
	while (!((msr = MSR) & MSR_RQM));

	if (msr & MSR_DIO) {
		/* Controller expects read, error */
		return -1;
	}

	FIFO = data;

	return 0;
}

static int floppy_fifo_read(uint8_t *data)
{
	uint8_t msr;

	/* TODO add timeout */
	while (!((msr = MSR) & MSR_RQM));

	if (!(msr & MSR_DIO)) {
		/* Controller expects write, error */
		return -1;
	}

	*data = FIFO;

	return 0;
}

static int floppy_read_result(struct cmd_result *res)
{
	if (floppy_fifo_read(&res->st0) < 0) {
		return -1;
	}

	if (floppy_fifo_read(&res->st1) < 0) {
		return -1;
	}

	if (floppy_fifo_read(&res->st2) < 0) {
		return -1;
	}

	if (floppy_fifo_read(&res->c) < 0) {
		return -1;
	}

	if (floppy_fifo_read(&res->h) < 0) {
		return -1;
	}

	if (floppy_fifo_read(&res->r) < 0) {
		return -1;
	}

	if (floppy_fifo_read(&res->n) < 0) {
		return -1;
	}

	return 0;
}

int floppy_cmd_read_data(uint8_t c, uint8_t h, uint8_t r, uint8_t drive, uint8_t *buff, struct cmd_result *res)
{
	/* MT = 0, MFM = 1, SK = 0 */
	if (floppy_fifo_write(0x46) < 0) {
		return -1;
	}

	if (floppy_fifo_write((h << 2) | drive) < 0) {
		return -1;
	}

	if (floppy_fifo_write(c) < 0) {
		return -1;
	}

	if (floppy_fifo_write(h) < 0) {
		return -1;
	}

	if (floppy_fifo_write(r) < 0) {
		return -1;
	}

	/* Sector size code for 512 B */
	if (floppy_fifo_write(2) < 0) {
		return -1;
	}

	if (floppy_fifo_write(EOT) < 0) {
		return -1;
	}

	if (floppy_fifo_write(GAP) < 0) {
		return -1;
	}

	/* DTL, not used */
	if (floppy_fifo_write(0xFF) < 0) {
		return -1;
	}

	/* Now we read data */
	for (size_t i = 0; i < 512; ++i) {
		if (floppy_fifo_read(buff + i) < 0) {
			return -1;
		}
	}

	return floppy_read_result(res);
}

int floppy_cmd_write_data(uint8_t c, uint8_t h, uint8_t r, uint8_t drive, const uint8_t *buff, struct cmd_result *res)
{
	/* MT = 0, MFM = 1, SK = 0 */
	if (floppy_fifo_write(0x45) < 0) {
		return -1;
	}

	if (floppy_fifo_write((h << 2) | drive) < 0) {
		return -1;
	}

	if (floppy_fifo_write(c) < 0) {
		return -1;
	}

	if (floppy_fifo_write(h) < 0) {
		return -1;
	}

	if (floppy_fifo_write(r) < 0) {
		return -1;
	}

	/* Sector size code for 512 B */
	if (floppy_fifo_write(2) < 0) {
		return -1;
	}

	if (floppy_fifo_write(EOT) < 0) {
		return -1;
	}

	if (floppy_fifo_write(GAP) < 0) {
		return -1;
	}

	/* DTL, not used */
	if (floppy_fifo_write(0xFF) < 0) {
		return -1;
	}

	/* Now we write data */
	for (size_t i = 0; i < 512; ++i) {
		if (floppy_fifo_write(buff[i]) < 0) {
			return -1;
		}
	}

	return floppy_read_result(res);
}

int floppy_cmd_verify(uint8_t c, uint8_t h, uint8_t r, uint8_t drive, struct cmd_result *res)
{
	/* MT = 0, MFM = 1, SK = 0 */
	if (floppy_fifo_write(0x56) < 0) {
		return -1;
	}

	if (floppy_fifo_write((h << 2) | drive) < 0) {
		return -1;
	}

	if (floppy_fifo_write(c) < 0) {
		return -1;
	}

	if (floppy_fifo_write(h) < 0) {
		return -1;
	}

	if (floppy_fifo_write(r) < 0) {
		return -1;
	}

	/* Sector size code for 512 B */
	if (floppy_fifo_write(2) < 0) {
		return -1;
	}

	if (floppy_fifo_write(EOT) < 0) {
		return -1;
	}

	if (floppy_fifo_write(GAP) < 0) {
		return -1;
	}

	/* DTL, not used */
	if (floppy_fifo_write(0xFF) < 0) {
		return -1;
	}

	return floppy_read_result(res);
}

int floppy_cmd_version(uint8_t *version)
{
	if (floppy_fifo_write(0x10) < 0) {
		return -1;
	}

	if (floppy_fifo_read(version) < 0) {
		return -1;
	}

	return 0;
}

int floppy_cmd_recalibrate(uint8_t drive)
{
	if (floppy_fifo_write(0x07) < 0) {
		return -1;
	}

	if (floppy_fifo_write(drive) < 0) {
		return -1;
	}

	return 0;
}

int floppy_cmd_sense_interrupt(uint8_t *st0, uint8_t *pcn)
{
	if (floppy_fifo_write(0x08) < 0) {
		return -1;
	}

	if (floppy_fifo_read(st0) < 0) {
		return -1;
	}

	if (floppy_fifo_read(pcn) < 0) {
		return -1;
	}

	return 0;
}

int floppy_cmd_specify(uint8_t srt, uint8_t hut, uint8_t hlt, uint8_t nd)
{
	if (floppy_fifo_write(0x08) < 0) {
		return -1;
	}

	if (floppy_fifo_write((srt << 4) | hut) < 0) {
		return -1;
	}

	if (floppy_fifo_write((hlt << 1) | nd) < 0) {
		return -1;
	}

	return 0;
}

int floppy_cmd_sense_drive_status(uint8_t h, uint8_t drive, uint8_t *status)
{
	if (floppy_fifo_write(0x04) < 0) {
		return -1;
	}

	if (floppy_fifo_write((h << 2) | drive) < 0) {
		return -1;
	}

	if (floppy_fifo_read(status) < 0) {
		return -1;
	}

	return 0;
}

int floppy_cmd_seek(uint8_t c, uint8_t h, uint8_t drive)
{
	if (floppy_fifo_write(0x0F) < 0) {
		return -1;
	}

	if (floppy_fifo_write((h << 2) | drive) < 0) {
		return -1;
	}

	if (floppy_fifo_write(c) < 0) {
		return -1;
	}

	return 0;
}

int floppy_cmd_configure(uint8_t eis, uint8_t poll, uint8_t fifothr)
{
	if (floppy_fifo_write(0x13) < 0) {
		return -1;
	}

	if (floppy_fifo_write(0x00) < 0) {
		return -1;
	}

	/* EFIFO = 1 */
	if (floppy_fifo_write((eis << 6) | (1 << 5) | (poll << 4) | fifothr) < 0) {
		return -1;
	}

	/* PRETRK */
	if (floppy_fifo_write(0x00) < 0) {
		return -1;
	}

	return 0;
}

int floppy_cmd_dumpreg(void)
{
	uint8_t buff[10];

	/* Stricly for debug */
	if (floppy_fifo_write(0x0E) < 0) {
		return -1;
	}

	for (size_t i = 0; i < 10; ++i) {
		if (floppy_fifo_read(buff + i) < 0) {
			return -1;
		}
	}

	printf("82077 dump regs:\r\n");
	for (size_t i = 0; i < 10; ++i) {
		printf("%zu: 0x%02x\r\n", buff[i]);
	}

	return 0;
}

int floppy_cmd_read_id(uint8_t h, uint8_t drive, struct cmd_result *res)
{
	if (floppy_fifo_write(0x45) < 0) {
		return -1;
	}

	if (floppy_fifo_write((h << 2) | drive) < 0) {
		return -1;
	}

	return floppy_read_result(res);
}

int floppy_cmd_lock(uint8_t lock)
{
	uint8_t readback;

	if (floppy_fifo_write((lock << 7) | 0x14) < 0) {
		return -1;
	}

	if (floppy_fifo_read(&readback) < 0) {
		return -1;
	}

	if (readback ^ (lock << 4)) {
		return -1;
	}

	return 0;
}

int floppy_cmd_powerdown_mode(uint8_t enable)
{
	uint8_t readback;

	if (floppy_fifo_write(0x17) < 0) {
		return -1;
	}

	if (floppy_fifo_write(0x02 | enable) < 0) {
		return -1;
	}

	if (floppy_fifo_read(&readback) < 0) {
		return -1;
	}

	if (readback ^ (0x02 | enable)) {
		return -1;
	}

	return 0;
}

void floppy_init(void)
{
	uint8_t st0, pcn;

	/* Bring out of reset */
	DOR = 0x0C;

	/* Poll MSR until ready */
	while (MSR != 0x80);

	DSR = DSR_HD144;

	floppy_cmd_sense_interrupt(&st0, &pcn);
	floppy_cmd_sense_interrupt(&st0, &pcn);
	floppy_cmd_sense_interrupt(&st0, &pcn);
	floppy_cmd_sense_interrupt(&st0, &pcn);
	floppy_cmd_configure(1, 0, 1);
	floppy_cmd_lock(1);
	floppy_cmd_specify(4, 64, 64, 1);
	floppy_cmd_recalibrate(0);
	floppy_cmd_recalibrate(1);
}
