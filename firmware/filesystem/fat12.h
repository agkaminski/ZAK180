/* FAT12 driver
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#ifndef FAT12_H_
#define FAT12_H_

#include <stdint.h>
#include <stddef.h>

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

#define FAT12_SECTOR_SIZE 512
#define FAT12_FAT_SIZE    9
#define FAT12_FAT_COPIES  2
#define FAT12_ROOT_SIZE   14
#define FAT12_DATA_START  (1 + (FAT12_FAT_COPIES * FAT12_FAT_SIZE) + FAT12_ROOT_SIZE)

struct fat12_fs {
	uint8_t sbuff[FAT12_SECTOR_SIZE]; /* Sector buffer */
	uint16_t sno;                     /* Number of the sector in the buffer */
	struct fat12_cb cb;               /* Physical media callbacks */
	uint16_t size;                    /* Total media size in sectors */
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
	struct fat12_dentry dentry;
	uint16_t recent_cluster;
	uint32_t recent_offs;
	uint16_t dentry_sector;
	uint16_t dentry_offs;
};

int fat12_file_open(struct fat12_fs *fs, struct fat12_file *file, const char *path);

int fat12_file_truncate(struct fat12_fs *fs, struct fat12_file *file, uint32_t nsize);

int fat12_file_read_dir(struct fat12_fs *fs, struct fat12_file *dir, struct fat12_dentry *entry, uint16_t idx);

int fat12_file_read(struct fat12_fs *fs, struct fat12_file *file, void *buff, size_t bufflen, uint32_t offs);

int fat12_file_write(struct fat12_fs *fs, struct fat12_file *file, const void *buff, size_t bufflen, uint32_t offs);

int fat12_mount(struct fat12_fs *fs, const struct fat12_cb *callback);

#endif
