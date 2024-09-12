/* FAT12 driver
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#include <string.h>
#include <ctype.h>
#include <stdio.h>

#include "fat12.h"

#include "../lib/errno.h"

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
		return -EIO;
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
	uint16_t last = file->recent_offs / (uint32_t)FAT12_SECTOR_SIZE;
	uint16_t new = offs / (uint32_t)FAT12_SECTOR_SIZE;

	if (new < last) {
		/* Have to start all over again*/
		last = 0;
		file->recent_cluster = file->dentry.cluster;
	}

	while (new > last) {
		uint16_t c;
		int ret = fat12_fat_get(fs, file->recent_cluster, &c);
		if (ret < 0) {
			return ret;
		}

		if ((c == CLUSTER_FREE) || (c == CLUSTER_RESERVED)) {
			return -EIO;
		}

		if (c == CLUSTER_END) {
			return 1; /* EOF */
		}

		file->recent_cluster = c;

		++last;
	}

	file->recent_offs = offs;

	return 0;
}

int fat12_file_read_dir(struct fat12_fs *fs, struct fat12_file *dir, struct fat12_dentry *entry, uint16_t idx)
{
	uint16_t sector;
	size_t len = 0;
	uint32_t offs = (uint32_t)idx * sizeof(*entry);
	int err;

	if (dir != NULL && !(dir->dentry.attr & FAT12_ATTR_DIR)) {
		return -ENOTDIR;
	}

	/* Root dir has to be processed separatelly, as it has negative cluster index */
	if (dir == NULL) {
		sector = 1 + (FAT12_FAT_COPIES * FAT12_FAT_SIZE) + (offs / FAT12_SECTOR_SIZE);
	}
	else {
		err = fat12_file_seek(fs, dir, offs);
		if (err < 0) {
			return err;
		}

		if (err > 0) {
			/* EOF */
			return -ENOENT;
		}

		sector = CLUSTER2SECTOR(dir->recent_cluster);
	}

	err = fat12_read_sector(fs, sector);
	if (err < 0) {
		return err;
	}

	uint16_t pos = offs % FAT12_SECTOR_SIZE;
	memcpy(entry, fs->sbuff + pos, sizeof(*entry));

	return sizeof(*entry);
}

int fat12_file_read(struct fat12_fs *fs, struct fat12_file *file, void *buff, size_t bufflen, uint32_t offs)
{
	uint16_t sector;
	size_t len = 0;

	if (file == NULL || (file->dentry.attr & FAT12_ATTR_DIR)) {
		return -EISDIR;
	}

	if (offs >= file->dentry.size) {
		return 0;
	}
	else if (offs + (uint32_t)bufflen > file->dentry.size) {
		bufflen = file->dentry.size - offs;
	}
	if (bufflen > 0x7FFF) {
		/* Reval has to fit in int (int16_t) */
		bufflen = 0x7FFF;
	}

	while (len < bufflen) {
		int err = fat12_file_seek(fs, file, offs);
		if (err < 0) {
			return -1;
		}

		if (err > 0) {
			/* EOF */
			break;
		}

		sector = CLUSTER2SECTOR(file->recent_cluster);

		if (fat12_read_sector(fs, sector) < 0) {
			return -1;
		}

		uint16_t pos = offs % FAT12_SECTOR_SIZE;
		uint16_t chunk = FAT12_SECTOR_SIZE - pos;

		if (chunk > bufflen - len) {
			chunk = bufflen - len;
		}

		memcpy((uint8_t *)buff + len, fs->sbuff + pos, chunk);

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
	uint16_t index;

	while (path[pos] == '/') {
		++pos;
	}

	if (path[pos] == '\0') {
		/* Root dir, no dentry available */
		return 1;
	}

	while (1) {
		/* Read current directory and find the relevant file */
		for (index = 0; ; ++index) {
			int ret = fat12_file_read_dir(fs, fp, &entry, index);
			if (ret < 0) {
				return ret;
			}
			if (entry.fname[0] == 0x00) {
				return -ENOENT;
			}
			if (entry.fname[0] == 0xE5) {
				/* Deleted entry*/
				continue;
			}

			if (!fat12_file_name_cmp(&entry, path + pos)) {
				break;
			}
		}

		/* Rewind to the next file in the path */
		while (path[pos] != '/' && path[pos] != '\0') {
			++pos;
		}

		if ((path[pos] == '\0') || (path[pos + 1] == '\0')) {
			break;
		}

		/* Skip / */
		++pos;

		/* If we found directory, then go to it next */
		if (!(entry.attr & FAT12_ATTR_DIR)) {
			/* Regular file is being used as directory, error */
			return -ENOTDIR;
		}

		memcpy(&f.dentry, &entry, sizeof(entry));
		f.recent_cluster = entry.cluster;
		f.recent_offs = 0;
		fp = &f;
	}

	memcpy(&file->dentry, &entry, sizeof(entry));
	file->recent_cluster = file->dentry.cluster;
	file->recent_offs = 0;

	file->dentry_sector = (fp == NULL) ?
		(1 + (FAT12_FAT_COPIES * FAT12_FAT_SIZE) +
		(index / (FAT12_SECTOR_SIZE / sizeof(struct fat12_dentry)))) :
		CLUSTER2SECTOR(file->dentry.cluster);
	file->dentry_offs = index % (FAT12_SECTOR_SIZE / sizeof(struct fat12_dentry));

	return 0;
}

static int fat12_file_update(struct fat12_fs *fs, struct fat12_file *file)
{
	if (fat12_read_sector(fs, file->dentry_sector) < 0) {
		return -EIO;
	}

	memcpy(fs->sbuff + file->dentry_offs, &file->dentry, sizeof(file->dentry));

	if (fat12_write_sector(fs, file->dentry_sector) < 0) {
		return -EIO;
	}

	return 0;
}

int fat12_file_truncate(struct fat12_fs *fs, struct fat12_file *file, uint32_t nsize)
{
	uint32_t old_size = file->dentry.size;
	int err;

	if (file->dentry.size == nsize) {
		return 0;
	}

	if ((file->dentry.size / (uint32_t)FAT12_SECTOR_SIZE) == (nsize / (uint32_t)FAT12_SECTOR_SIZE)) {
		/* No need to add/remove clusters, just init the space */
		if (file->dentry.size < nsize) {
			err = fat12_file_seek(fs, file, file->dentry.size - 1);
			if (err < 0) {
				return err;
			}

			err = fat12_read_sector(fs, CLUSTER2SECTOR(file->recent_cluster));
			if (err < 0) {
				return err;
			}

			uint16_t chunk = nsize - file->dentry.size;
			uint16_t offs = file->dentry.size % FAT12_SECTOR_SIZE;

			memset(fs->sbuff + offs, 0, chunk);

			err = fat12_write_sector(fs, CLUSTER2SECTOR(file->recent_cluster));
			if (err < 0) {
				return err;
			}
		}
	}
	else if (file->dentry.size < nsize) {
		err = fat12_file_seek(fs, file, file->dentry.size - 1);
		if (err < 0) {
			return err;
		}

		uint16_t start = file->recent_cluster;

		/* Try to find new cluster after the current last */
		while (file->dentry.size < nsize) {
			uint16_t ncluster, ccluster = file->recent_cluster + 1;
			uint8_t retry = 0;

			for (ccluster = file->recent_cluster + 1; ; ++ccluster) {
				if (fat12_fat_get(fs, ccluster, &ncluster) < 0) {
					if (retry) {
						/* No space? Restore previous file and FAT state */
						(void)fat12_file_truncate(fs, file, old_size);
						return -ENOSPC;
					}
					else {
						/* Try again from the FAT start */
						retry = 1;
						ccluster = 0;
					}
				}

				if (retry && (ccluster == file->recent_cluster)) {
					return -ENOSPC;
				}

				if (ncluster == CLUSTER_FREE) {
					break;
				}
			}

			/* Add a new cluster to the chain */
			if (fat12_fat_set(fs, start, ccluster) < 0) {
				(void)fat12_file_truncate(fs, file, old_size);
				return -EIO;
			}

			if (fat12_fat_set(fs, ccluster, CLUSTER_END) < 0) {
				(void)fat12_file_truncate(fs, file, old_size);
				return -EIO;
			}

			/* Zero-out new cluster */
			memset(fs->sbuff, 0, sizeof(fs->sbuff));
			if (fat12_write_sector(fs, CLUSTER2SECTOR(ccluster)) < 0) {
				(void)fat12_file_truncate(fs, file, old_size);
				return -EIO;
			}

			file->dentry.size += FAT12_SECTOR_SIZE;
			start = ccluster;
		}
	}
	else {
		uint16_t ccluster = file->dentry.cluster;
		uint16_t prev = ccluster;
		for (uint32_t csize = 0; csize < nsize; csize += FAT12_SECTOR_SIZE) {
			prev = ccluster;
			if (fat12_fat_get(fs, prev, &ccluster) < 0) {
				return -EIO;
			}
		}

		if (ccluster != CLUSTER_END) {
			/* ccluster have the rest of the chain, prev is new end */
			fat12_fat_set(fs, prev, CLUSTER_END);

			while (ccluster != CLUSTER_END) {
				prev = ccluster;
				if (fat12_fat_get(fs, prev, &ccluster) < 0) {
					/* We have leaked clusters and can't do anything about it */
					return -EIO;
				}
				if (fat12_fat_set(fs, prev, CLUSTER_FREE) < 0) {
					return -EIO;
				}
			}
		}
	}

	file->dentry.size = nsize;

	return fat12_file_update(fs, file);
}

int fat12_file_write(struct fat12_fs *fs, struct fat12_file *file, const void *buff, size_t bufflen, uint32_t offs)
{
	size_t len = 0;
	int err;

	if (file == NULL || (file->dentry.attr & FAT12_ATTR_DIR)) {
		return -EISDIR;
	}

	if (file->dentry.size < offs + (uint32_t)bufflen) {
		err = fat12_file_truncate(fs, file, offs + (uint32_t)bufflen);
		if (err < 0) {
			return err;
		}
	}

	while (len < bufflen) {
		err = fat12_file_seek(fs, file, offs + len);
		if (err < 0) {
			return err;
		}

		uint16_t missalign = offs % (uint32_t)FAT12_SECTOR_SIZE;

		if (missalign || (bufflen - len < FAT12_SECTOR_SIZE)) {
			err = fat12_read_sector(fs, CLUSTER2SECTOR(file->recent_cluster));
			if (err < 0) {
				return err;
			}
		}

		size_t chunk = FAT12_SECTOR_SIZE - missalign;
		if (chunk > bufflen - len) {
			chunk = bufflen - len;
		}

		memcpy(fs->sbuff + missalign, (uint8_t *)buff + len, chunk);

		err = fat12_write_sector(fs, CLUSTER2SECTOR(file->recent_cluster));
		if (err < 0) {
			return err;
		}

		offs += chunk;
		len += chunk;
	}

	return len;
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
		return -EIO;
	}

	/* Sectors per cluster */
	t8 = *(fs->sbuff + 13);
	if (t8 != 1) {
		return -EIO;
	}

	/* Number of reserved sectors */
	memcpy(&t16, fs->sbuff + 14, sizeof(t16));
	if (t16 != 1) {
		return -EIO;
	}

	/* Number of FATs */
	t8 = *(fs->sbuff + 16);
	if (t8 != 2) {
		return -EIO;
	}

	/* Maximum number of root directory entries */
	memcpy(&t16, fs->sbuff + 17, sizeof(t16));
	if (t16 != 224) {
		return -EIO;
	}

	/* Total sector count */
	memcpy(&fs->size, fs->sbuff + 18, sizeof(t16));

	/* Sectors per FAT */
	memcpy(&t16, fs->sbuff + 22, sizeof(t16));
	if (t16 != 9) {
		return -EIO;
	}

	/* Sectors per track */
	memcpy(&t16, fs->sbuff + 24, sizeof(t16));
	if (t16 != 18) {
		return -EIO;
	}

	/* Number of heads */
	memcpy(&t16, fs->sbuff + 26, sizeof(t16));
	if (t16 != 2) {
		return -EIO;
	}

	/* Ignore rest of the fields - most likely
	 * not valid anyway. */

	return 0;
}
