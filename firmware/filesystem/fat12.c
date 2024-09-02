/* FAT12 driver
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#include <string.h>
#include <ctype.h>
#include <stdio.h>

#include "fat12.h"

#define CLUSTER2SECTOR(c) ((c) + FAT12_DATA_START - 2)
#define SECTOR2CLUSTER(s) ((x) + 2 - FAT12_DATA_START)

#define CLUSTER_FREE     0
#define CLUSTER_RESERVED 0xFF0
#define CLUSTER_END      0xFFF

static int fat12_read_sector(struct fat12_fs *fs, uint16_t n)
{
	int ret = 0;

	if (n != fs->sno) {
		ret = fs->cb.read_sector(n, fs->sbuff);
		fs->sno = n; /* Update always, buffer is changed anyway */
	}

	return ret;
}

static int fat12_write_sector(struct fat12_fs *fs, uint16_t n)
{
	fs->sno = n;
	return fs->cb.write_sector(n, fs->sbuff);
}

static int fat12_fat_get(struct fat12_fs *fs, uint16_t n, uint16_t *cluster)
{
	if (CLUSTER2SECTOR(n) > fs->size) {
		return -1;
	}

	uint16_t idx = (3 * n) / 2;
	uint16_t sector = 1 + (idx / FAT12_SECTOR_SIZE);
	uint16_t pos = idx & (FAT12_SECTOR_SIZE - 1);

	int ret = fat12_read_sector(fs, sector);
	if (ret < 0) {
		return ret;
	}

	if (n & 1) {
		*cluster = fs->sbuff[pos] >> 4;
	}
	else {
		*cluster = fs->sbuff[pos];
	}

	++idx;

	uint16_t sector_next = 1 + (idx / FAT12_SECTOR_SIZE);
	pos = idx & (FAT12_SECTOR_SIZE - 1);

	if (sector != sector_next) {
		ret = fat12_read_sector(fs, sector_next);
		if (ret < 0) {
			return ret;
		}
	}

	if (n & 1) {
		*cluster |= (uint16_t)fs->sbuff[pos] << 4;
	}
	else {
		*cluster |= (uint16_t)(fs->sbuff[pos] & 0x0F) << 8;
	}

	if (*cluster >= 0xFF8) {
		*cluster = CLUSTER_END;
	}
	else if (*cluster >= 0xFF0) {
		*cluster = CLUSTER_RESERVED;
	}

	return 0;
}

static int fat12_fat_set(struct fat12_fs *fs,  uint16_t n, uint16_t cluster)
{
	if ((CLUSTER2SECTOR(n) > fs->size) || (cluster & 0xF000)) {
		return -1;
	}

	uint16_t idx = (3 * n) / 2;
	uint16_t sector = 1 + (idx / FAT12_SECTOR_SIZE);
	uint16_t pos = idx & (FAT12_SECTOR_SIZE - 1);

	int ret = fat12_read_sector(fs, sector);
	if (ret < 0) {
		return ret;
	}

	if (n & 1) {
		fs->sbuff[pos] &= 0x0F;
		fs->sbuff[pos] |= (uint8_t)cluster << 4;
	}
	else {
		fs->sbuff[pos] = (uint8_t)cluster;
	}

	++idx;

	uint16_t sector_next = 1 + (idx / FAT12_SECTOR_SIZE);
	pos = idx & (FAT12_SECTOR_SIZE - 1);

	if (sector != sector_next) {
		ret = fat12_write_sector(fs, sector);
		if (ret < 0) {
			return ret;
		}

		/* Need to update FAT copy too */
		ret = fat12_write_sector(fs, sector + FAT12_FAT_SIZE);
		if (ret < 0) {
			return ret;
		}

		ret = fat12_read_sector(fs, sector_next);
		if (ret < 0) {
			return ret;
		}
		sector = sector_next;
	}

	if (n & 1) {
		fs->sbuff[pos] = (uint16_t)cluster >> 4;
	}
	else {
		fs->sbuff[pos] &= 0xF0;
		fs->sbuff[pos] |= (cluster >> 8) & 0x0F;
	}

	/* Need to update FAT copy too */
	ret = fat12_write_sector(fs, sector + FAT12_FAT_SIZE);
	if (ret < 0) {
		return ret;
	}

	/* Update main after copy to adjust fs->sno */
	ret = fat12_write_sector(fs, sector);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

/* Find a cluster associated with the offset */
static int fat12_file_seek(struct fat12_fs *fs, struct fat12_file *file, uint32_t offs)
{
	uint16_t last = file->last_offs / (uint32_t)FAT12_SECTOR_SIZE;
	uint16_t new = offs / (uint32_t)FAT12_SECTOR_SIZE;

	if (new < last) {
		/* Have to start all over again*/
		last = 0;
		file->last_cluster = file->dentry.cluster;
	}

	while (new > last) {
		uint16_t c;
		if (fat12_fat_get(fs, file->last_cluster, &c) < 0) {
			return -1;
		}

		if ((c == CLUSTER_FREE) || (c == CLUSTER_RESERVED)) {
			return -1;
		}

		if (c == CLUSTER_END) {
			return 1; /* EOF */
		}

		file->last_cluster = c;
		file->last_offs = (uint32_t)last * (uint32_t)FAT12_SECTOR_SIZE;

		++last;
	}

	return 0;
}

int fat12_file_read(struct fat12_fs *fs, struct fat12_file *file, void *buff, size_t bufflen, uint32_t offs)
{
	uint16_t sector;
	size_t len = 0;
	uint32_t fsize = (file == NULL) ? FAT12_ROOT_SIZE * FAT12_SECTOR_SIZE : file->dentry.size;

	if (offs >= fsize) {
		return 0;
	}
	else if (offs + (uint32_t)bufflen > fsize) {
		bufflen = fsize - offs;
	}
	if (bufflen > 0x7FFF) {
		/* Reval has to fit in int (int16_t) */
		bufflen = 0x7FFF;
	}

	while (len < bufflen) {
		/* Root dir has to be processed separatelly, as it has negative cluster index */
		if (file == NULL) {
			sector = 1 + (FAT12_FAT_COPIES * FAT12_FAT_SIZE) + (offs / FAT12_SECTOR_SIZE);
		}
		else {
			int err = fat12_file_seek(fs, file, offs);
			if (err < 0) {
				return -1;
			}

			if (err > 0) {
				/* EOF */
				break;
			}

			sector = CLUSTER2SECTOR(file->last_cluster);
		}

		if (fat12_read_sector(fs, sector) < 0) {
			return -1;
		}

		uint16_t pos = offs % FAT12_SECTOR_SIZE;
		uint16_t chunk = FAT12_SECTOR_SIZE - pos;

		if (chunk > bufflen - len) {
			chunk = bufflen - len;
		}

		memcpy(buff + len, fs->sbuff + pos, chunk);

		len += chunk;
		offs += chunk;
	}

	return (int)len;
}

static uint8_t fat12_file_namelen(const char *str, uint8_t max)
{
	uint8_t pos = max - 1;

	while (pos != 0 && str[pos] == ' ') {
		--pos;
	}

	if (str[pos] == ' ') {
		return 0;
	}

	return pos + 1;
}

static int fat12_file_name_cmp(const struct fat12_dentry *entry, const char *path)
{
	uint8_t i, pos;
	char c;

	uint8_t len = fat12_file_namelen(entry->fname, sizeof(entry->fname));

	printf("fname: %8s.%3s, looking for: %s\r\n", entry->fname, entry->extension, path);

	for (i = 0; i < len; ++i) {
		c = toupper(path[i]);
		if (c == '/' || c == '\0') {
			return 1;
		}

		if (entry->fname[i] != c) {
			return 1;
		}
	}

	len = fat12_file_namelen(entry->extension, sizeof(entry->extension));

	pos = i;
	if (path[i] == '.') {
		++pos;
	}
	else {
		return ((path[i] != '/' && path[i] != '\0') || len != 0) ? 1 : 0;
	}

	for (i = 0; i < len; ++i) {
		c = toupper(path[pos + i]);
		if (c == '/' || c == '\0') {
			return 1;
		}

		if (entry->extension[i] != c) {
			return 1;
		}
	}

	return (path[pos + i] != '/' && path[pos + i] != '\0') ? 1 : 0;
}

int fat12_file_open(struct fat12_fs *fs, struct fat12_file *file, const char *path)
{
	uint8_t pos = 0;
	struct fat12_file f, *fp = NULL; /* NULL means root dir */
	struct fat12_dentry entry;

	while (path[pos] == '/') {
		++pos;
	}

	if (path[pos] == '\0') {
		/* Root dir, no dentry available */
		return 1;
	}

	while (1) {
		/* Read current directory and find the relevant file */
		for (uint32_t filepos = 0; ; filepos += sizeof(struct fat12_dentry)) {
			int ret = fat12_file_read(fs, fp, &entry, sizeof(entry), filepos);
			if (ret < 0) {
				return ret;
			}
			else if ((ret == 0) || (entry.fname[0] == 0x00)) {
				/* EOF, not found */
				return -1; /* FIXME ENOENT */
			}
			else if (entry.fname[0] == 0xE5) {
				/* Deleted entry*/
				continue;
			}

			if (!fat12_file_name_cmp(&entry, path + pos)) {
				break;
			}
		}

		/* Rewind to the next file in path */
		while (path[pos] != '/' && path[pos] != '\0') {
			++pos;
		}

		if (path[pos] == '\0') {
			break;
		}

		/* If we found directory, then go to it next */
		if (!(entry.attr & FAT12_ATTR_DIR)) {
			/* Regular file is being used as directory, error */
			return -1; /* FIXME ENOTDIR */
		}

		memcpy(&f.dentry, &entry, sizeof(entry));
		f.last_cluster = entry.cluster;
		f.last_offs = 0;
		fp = &f;
	}

	memcpy(&file->dentry, &entry, sizeof(entry));
	file->last_cluster = file->dentry.cluster;
	file->last_offs = 0;
	return 0;
}

int fat12_mount(struct fat12_fs *fs, const struct fat12_cb *callback)
{
	int err;
	uint16_t t16;
	uint8_t t8;

	/* Register callbacks */
	memcpy(&fs->cb, callback, sizeof(fs->cb));

	/* Read and parse boot sector */
	err = fs->cb.read_sector(0, fs->sbuff);
	if (err < 0) {
		return err;
	}
	fs->sno = 0;

	/* Check if the filesystem is present and
	 * what we expect it to be. FAT12 is LE,
	 * Z180 is LE too, ignore endianess. */

	/* Bytes per sector */
	memcpy(&t16, fs->sbuff + 11, sizeof(t16));
	if (t16 != FAT12_SECTOR_SIZE) {
		return -1;
	}

	/* Sectors per cluster */
	t8 = *(fs->sbuff + 13);
	if (t8 != 1) {
		return -1;
	}

	/* Number of reserved sectors */
	memcpy(&t16, fs->sbuff + 14, sizeof(t16));
	if (t16 != 1) {
		return -1;
	}

	/* Number of FATs */
	t8 = *(fs->sbuff + 16);
	if (t8 != 2) {
		return -1;
	}

	/* Maximum number of root directory entries */
	memcpy(&t16, fs->sbuff + 17, sizeof(t16));
	if (t16 != 224) {
		return -1;
	}

	/* Total sector count */
	memcpy(&fs->size, fs->sbuff + 18, sizeof(t16));

	/* Sectors per FAT */
	memcpy(&t16, fs->sbuff + 22, sizeof(t16));
	if (t16 != 9) {
		return -1;
	}

	/* Sectors per track */
	memcpy(&t16, fs->sbuff + 24, sizeof(t16));
	if (t16 != 18) {
		return -1;
	}

	/* Number of heads */
	memcpy(&t16, fs->sbuff + 26, sizeof(t16));
	if (t16 != 2) {
		return -1;
	}

	/* Ignore rest of the fields - most likely
	 * not valid anyway. */

	return 0;
}
