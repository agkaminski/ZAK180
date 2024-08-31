/* ZAK180 Firmaware
 * 82077 Floppy controller driver
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#ifndef DRIVER_82077_H_
#define DRIVER_82077_H_

#include <stdint.h>

struct floppy_result {
	uint8_t st0;
	uint8_t st1;
	uint8_t st2;
	uint8_t c;
	uint8_t h;
	uint8_t r;
	uint8_t n;
};


int floppy_cmd_read_data(uint8_t c, uint8_t h, uint8_t r, uint8_t *buff, struct floppy_result *res);

int floppy_cmd_write_data(uint8_t c, uint8_t h, uint8_t r, const uint8_t *buff, struct floppy_result *res);

int floppy_cmd_recalibrate(void);

int floppy_cmd_seek(uint8_t c);

void floppy_enable(int8_t enable);

int floppy_reset(int warm);

void floppy_init(void);

#endif
