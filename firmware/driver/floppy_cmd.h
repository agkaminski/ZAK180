/* ZAK180 Firmaware
 * 82077 Floppy controller driver
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#ifndef DRIVER_FLOPPY_CMD_H_
#define DRIVER_FLOPPY_CMD_H_

#include <stdint.h>

struct floppy_cmd_result {
	uint8_t st0;
	uint8_t st1;
	uint8_t st2;
	uint8_t c;
	uint8_t h;
	uint8_t r;
	uint8_t n;
};

int floppy_cmd_read_data(uint16_t lba, uint8_t *buff, struct floppy_cmd_result *res);

int floppy_cmd_write_data(uint16_t lba, const uint8_t *buff, struct floppy_cmd_result *res);

int floppy_cmd_recalibrate(void);

int floppy_cmd_seek(uint8_t c);

void floppy_cmd_enable(int8_t enable);

int floppy_cmd_reset(int warm);

void floppy_cmd_init(void);

#endif
