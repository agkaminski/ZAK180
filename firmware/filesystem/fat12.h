/* FAT12 driver
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#ifndef FAT12_H_
#define FAT12_H_

#include <stdint.h>

#define FAT12_ATTR_READONLY (1 << 0)
#define FAT12_ATTR_HIDDEN   (1 << 1)
#define FAT12_ATTR_SYSTEM   (1 << 2)
#define FAT12_ATTR_VLABEL   (1 << 3)
#define FAT12_ATTR_DIR      (1 << 4)
#define FAT12_ATTR_ARCHIVE  (1 << 5)

struct fat12_cb {
	int (*read_sector)(uint16_t sector, void *buff);
	int (*write_sector)(uint16_t sector, void *buff);
};

struct fat12_fs {

};

struct fat12_dentry {
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

struct fat12_file {

};

int fat12_create(struct fat12_fs *fs, struct fat12_file *dir, const char *name, uint8_t attr);

int fat12_mount(struct fat12_fs *fs, struct fat12_cb *callback);
