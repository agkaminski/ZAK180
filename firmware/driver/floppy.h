/* ZAK180 Firmaware
 * Floppy disk block device driver
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#ifndef DRIVER_FLOPPY_H_
#define DRIVER_FLOPPY_H_

#include <stdint.h>

#define FLOPPY_IOERR       -1
#define FLOPPY_DISK_CHANGE -2
#define FLOPPY_NO_MEDIA    -3

void floppy_access(uint8_t enable);

int floppy_read_sector(uint16_t lba, void *buff);

int floppy_write_sector(uint16_t lba, const void *buff);

int floppy_init(void);

#endif
