/* ZAK180 Firmaware
 * FAT12
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#ifndef FS_FAT12_H_
#define FS_FAT12_H_

#include <stdint.h>
#include "fs/fs.h"

#define FAT12_ATTR_READONLY (1 << 0)
#define FAT12_ATTR_HIDDEN   (1 << 1)
#define FAT12_ATTR_SYSTEM   (1 << 2)
#define FAT12_ATTR_VLABEL   (1 << 3)
#define FAT12_ATTR_DIR      (1 << 4)
#define FAT12_ATTR_ARCHIVE  (1 << 5)

#define FAT12_SECTOR_SIZE 512

extern const struct fs_file_op fat_op;

struct fat_dentry {
	char fname[8];
	char extension[3];
	uint8_t attr;
	uint16_t res0;
	uint16_t ctime;
	uint16_t cdate;
	uint16_t adate;
	uint16_t res1;
	uint16_t mtime;
	uint16_t mdate;
	uint16_t cluster;
	uint32_t size;
};

struct fat_file {
	uint16_t cluster;
	uint16_t recent_cluster;
	uint32_t recent_offs;
	uint16_t idx;
};

struct fat_ctx {
	uint8_t fat_page[2];
};

#endif
