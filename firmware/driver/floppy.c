/* ZAK180 Firmaware
 * 82077 Floppy controller driver
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#include <stdio.h>
#include <stdint.h>

#include <z180/z180.h>

#include "floppy.h"

#define DEBUG(fmt, ...) printf(fmt "\r\n", ##__VA_ARGS__)

/* 82077 on ZAK180 is in AT mode */

/* Registers */
__sfr __at(0xE2) DOR;  /* Digital Output Register */
__sfr __at(0xE4) MSR;  /* Main Status Register (RO) */
__sfr __at(0xE5) FIFO; /* Data Register (FIFO)*/
__sfr __at(0xE7) DIR;  /* Digital Input Register (RO) */
__sfr __at(0xE7) CCR;  /* Configuration Control Register (WO) */

//#define DOR_SELECT_NONE 0x0C
//#define DOR_SELECT_0    0x1C
//#define DOR_SELECT_1    0x2D
//#define DOR_SELECT_2    0x4E
//#define DOR_SELECT_3    0x8F

#define DOR_SELECT_NONE 0x04
#define DOR_SELECT_0    0x14
#define DOR_SELECT_1    0x25
#define DOR_SELECT_2    0x46
#define DOR_SELECT_3    0x87

#define MSR_RQM       (1 << 7)
#define MSR_DIO       (1 << 6)
#define MSR_NON_DMA   (1 << 5)
#define MSR_CMD_PROG  (1 << 4)
#define MSR_DRV3_BUSY (1 << 3)
#define MSR_DRV2_BUSY (1 << 2)
#define MSR_DRV1_BUSY (1 << 1)
#define MSR_DRV0_BUSY (1 << 0)

#define CCR_HD144 0 /* 500 kbit/s, default precompensation */

#define DIR_DSKCHG (1 << 7)

#define EOT 0x12
#define GAP 0x18

#define DRIVE_NO 1

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
	//printf("FIFO (0xE5): 0x%02x\r\n", FIFO);
	printf("DIR  (0xE7): 0x%02x\r\n", DIR);
}

static int floppy_fifo_write(uint8_t data)
{
	uint8_t msr;

	/* TODO add timeout */
	do {
		msr = MSR;
	} while ((msr & 0xc0) != 0x80);

	FIFO = data;

	return 0;
}

static int floppy_fifo_read(uint8_t *data)
{
	uint8_t msr;

	/* TODO add timeout */
	do {
		msr = MSR;
	} while ((msr & (uint8_t)0xc0) != 0xc0);

	*data = FIFO;

	//printf("%02x ", *data);

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

	printf("st0: %02x\r\n", res->st0);
	printf("st1: %02x\r\n", res->st1);
	printf("st2: %02x\r\n", res->st2);

	return 0;
}

static int floppy_write_cmd(uint8_t *cmd, uint8_t len)
{
	for (uint8_t i = 0; i < len; ++i) {
		if (floppy_fifo_write(cmd[i]) < 0) {
			return -1;
		}
	}

	return 0;
}

static void floppy_sector_read(uint8_t *buff)
{
	for (uint16_t i = 0; i < 512; ++i) {
		while ((MSR & (uint8_t)0xc0) != 0xc0);
		buff[i] = FIFO;
	}
}

int floppy_cmd_read_data(uint8_t c, uint8_t h, uint8_t r, uint8_t *buff, struct cmd_result *res)
{
	uint8_t cmd[] = { 0x46, ((h << 2) | DRIVE_NO), c, h, r, 2, EOT, GAP, 0xFF };

	DEBUG("Floppy CMD Read Data");

	if (floppy_write_cmd(cmd, sizeof(cmd)) < 0) {
		return -1;
	}

	floppy_sector_read(buff);

	printf("dupa\r\n");

	/* Cause overrun */
	for (volatile unsigned int i = 1; i != 0; ++i);

	return floppy_read_result(res);
}

int floppy_cmd_write_data(uint8_t c, uint8_t h, uint8_t r, const uint8_t *buff, struct cmd_result *res)
{
	uint8_t cmd[] = { 0x45, ((h << 2) | DRIVE_NO), c, h, r, 2, EOT, GAP, 0xFF };

	DEBUG("Floppy CMD write data");

	if (floppy_write_cmd(cmd, sizeof(cmd)) < 0) {
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

int floppy_cmd_version(uint8_t *version)
{
	DEBUG("Floppy CMD version");

	if (floppy_fifo_write(0x10) < 0) {
		return -1;
	}

	if (floppy_fifo_read(version) < 0) {
		return -1;
	}

	return 0;
}

int floppy_cmd_recalibrate(void)
{
	DEBUG("Floppy CMD recalibrate");

	if (floppy_fifo_write(0x07) < 0) {
		return -1;
	}

	if (floppy_fifo_write(DRIVE_NO) < 0) {
		return -1;
	}

	return 0;
}

int floppy_cmd_sense_interrupt(uint8_t *st0, uint8_t *pcn)
{
	DEBUG("Floppy CMD sense interrupt");
retry:
	if (floppy_fifo_write(0x08) < 0) {
		return -1;
	}

	if (floppy_fifo_read(st0) < 0) {
		return -1;
	}

	if (*st0 == 0x80) {
		goto retry;
	}

	if (floppy_fifo_read(pcn) < 0) {
		return -1;
	}

	return 0;
}

int floppy_cmd_specify(uint8_t srt, uint8_t hut, uint8_t hlt, uint8_t nd)
{
	uint8_t cmd[] = { 0x03, ((srt << 4) | hut), ((hlt << 1) | nd) };

	DEBUG("Floppy CMD specify");

	if (floppy_write_cmd(cmd, sizeof(cmd)) < 0) {
		return -1;
	}

	return 0;
}

int floppy_cmd_seek(uint8_t c)
{
	uint8_t cmd[] = { 0x0F, DRIVE_NO, c };

	DEBUG("Floppy CMD seek");

	if (floppy_write_cmd(cmd, sizeof(cmd)) < 0) {
		return -1;
	}

	return 0;
}

int floppy_cmd_configure(uint8_t eis, uint8_t poll, uint8_t fifothr)
{
	uint8_t cmd[] = { 0x13, 0x00, ((eis << 6) | (poll << 4) | fifothr), 0x00 };

	DEBUG("Floppy CMD configure");

	if (floppy_write_cmd(cmd, sizeof(cmd)) < 0) {
		return -1;
	}

	return 0;
}

int floppy_cmd_dumpreg(void)
{
	uint8_t buff[10];

	DEBUG("Floppy CMD dumpreg");

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
		printf("%zu: 0x%02x\r\n", i, buff[i]);
	}

	return 0;
}

int floppy_cmd_lock(uint8_t lock)
{
	uint8_t readback;

	DEBUG("Floppy CMD lock");

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

static void floppy_enable(int8_t enable)
{
	uint8_t dorval[] = {
		DOR_SELECT_0,
		DOR_SELECT_1,
		DOR_SELECT_2,
		DOR_SELECT_3
	};

	DOR = enable ? dorval[DRIVE_NO] : DOR_SELECT_NONE;
}

uint8_t sector[512];
struct cmd_result res;

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

void floppy_init(void)
{
	uint8_t st0, pcn;

	/* Bring out of reset */
	for (volatile unsigned int i = 1; i != 0; ++i);
	DOR = DOR_SELECT_NONE;

	/* Poll MSR until ready */
	while (MSR != 0x80);

	for (volatile unsigned int i = 1; i != 0; ++i);
	for (volatile unsigned int i = 1; i != 0; ++i);

	floppy_cmd_sense_interrupt(&st0, &pcn);
	floppy_cmd_sense_interrupt(&st0, &pcn);
	floppy_cmd_sense_interrupt(&st0, &pcn);
	floppy_cmd_sense_interrupt(&st0, &pcn);

	CCR = CCR_HD144;

	floppy_cmd_configure(1, 1, 15);
	floppy_cmd_lock(1);
	floppy_cmd_specify(12, 0, 2, 1);

	floppy_enable(1);
	for (volatile unsigned int i = 1; i != 0; ++i);

	for (volatile unsigned int i = 1; i != 0; ++i);
	floppy_cmd_recalibrate();
	floppy_cmd_sense_interrupt(&st0, &pcn);

	floppy_enable(0);

	floppy_cmd_dumpreg();

	floppy_enable(1);
	for (volatile unsigned int i = 1; i != 0; ++i);

	floppy_cmd_read_data(0, 0, 1, sector, &res);
	dump(0);
	floppy_cmd_read_data(0, 0, 2, sector, &res);
	dump(1);
	floppy_cmd_read_data(0, 0, 3, sector, &res);
	dump(2);
	floppy_cmd_read_data(0, 0, 4, sector, &res);
	dump(3);

	floppy_enable(0);
}
