/* ZAK180 Firmaware
 * FAT12
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#include <stdint.h>
#include <string.h>
#include <ctype.h>

#include "fs/fat.h"
#include "fs/fs.h"
#include "lib/errno.h"

#define FAT12_FAT_SIZE    9
#define FAT12_FAT_COPIES  2
#define FAT12_ROOT_SIZE   14
#define FAT12_DATA_START  (1 + (FAT12_FAT_COPIES * FAT12_FAT_SIZE) + FAT12_ROOT_SIZE)

#define CLUSTER2SECTOR(c) ((c) + FAT12_DATA_START - 2)
#define SECTOR2CLUSTER(s) ((x) + 2 - FAT12_DATA_START)

#define CLUSTER_FREE     0
#define CLUSTER_RESERVED 0xFF0
#define CLUSTER_END      0xFFF

static int8_t fat_op_open(struct fs_ctx *ctx, struct fs_file *file, const char *name, struct fs_file *dir, int8_t create, uint8_t attr);
static int8_t fat_op_close(struct fs_ctx *ctx, struct fs_file *file);
static int16_t fat_op_read(struct fs_ctx *ctx, struct fs_file *file, void *buff, size_t bufflen, uint32_t offs);
static int8_t fat_op_truncate(struct fs_ctx *ctx, struct fs_file *file, uint32_t size);
static int16_t fat_op_write(struct fs_ctx *ctx, struct fs_file *file, const void *buff, size_t bufflen, uint32_t offs);
static int8_t fat_op_readdir(struct fs_ctx *ctx, struct fs_file *file, struct fs_dentry *dentry, uint16_t idx);
static int8_t fat_op_move(struct fs_ctx *ctx, struct fs_file *file, struct fs_file *ndir, const char *name);
static int8_t fat_op_remove(struct fs_ctx *ctx, struct fs_file *file);
static int8_t fat_op_set_attr(struct fs_ctx *ctx, struct fs_file *file, uint8_t attr, uint8_t mask);
static int8_t fat_op_mount(struct fs_ctx *ctx, struct fs_cb *cb, struct fs_file *parent, struct fs_file *rootdir);
static int8_t fat_op_unmount(struct fs_ctx *ctx);

static const struct fs_file_op fat_op = {
	.open = fat_op_open,
	.close = fat_op_close,
	.read = fat_op_read,
	.write = fat_op_write,
	.truncate = fat_op_truncate,
	.readdir = fat_op_readdir,
	.move = fat_op_move,
	.remove = fat_op_remove,
	.set_attr = fat_op_set_attr,
	.ioctl = NULL,
	.mount = fat_op_mount,
	.unmount = fat_op_unmount
};

static int fat_read_sector(struct fat_fs *fs, uint16_t n)
{
	int ret = 0;

	if (n != fs->sno) {
		ret = fs->cb.read_sector(n, fs->sbuff);
		fs->sno = n; /* Update always, buffer is changed anyway */
	}

	return ret;
}

static int fat_write_sector(struct fat_fs *fs, uint16_t n)
{
	fs->sno = n;
	return fs->cb.write_sector(n, fs->sbuff);
}

static int8_t fat_fat_get(struct fat_fs *fs, uint16_t n, uint16_t *cluster)
{
	if (CLUSTER2SECTOR(n) > fs->size) {
		return -EIO;
	}

	uint16_t idx = (3 * n) / 2;
	uint16_t sector = 1 + (idx / FAT12_SECTOR_SIZE);
	uint16_t pos = idx & (FAT12_SECTOR_SIZE - 1);

	int ret = fat_read_sector(fs, sector);
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
		ret = fat_read_sector(fs, sector_next);
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

static int8_t fat_fat_set(struct fat_fs *fs,  uint16_t n, uint16_t cluster)
{
	if ((CLUSTER2SECTOR(n) > fs->size) || (cluster & 0xF000)) {
		return -1;
	}

	uint16_t idx = (3 * n) / 2;
	uint16_t sector = 1 + (idx / FAT12_SECTOR_SIZE);
	uint16_t pos = idx & (FAT12_SECTOR_SIZE - 1);

	int ret = fat_read_sector(fs, sector);
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
		ret = fat_write_sector(fs, sector);
		if (ret < 0) {
			return ret;
		}

		/* Need to update FAT copy too */
		ret = fat_write_sector(fs, sector + FAT12_FAT_SIZE);
		if (ret < 0) {
			return ret;
		}

		ret = fat_read_sector(fs, sector_next);
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
	ret = fat_write_sector(fs, sector + FAT12_FAT_SIZE);
	if (ret < 0) {
		return ret;
	}

	/* Update main after copy to adjust fs->sno */
	ret = fat_write_sector(fs, sector);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

/* Find a cluster associated with the offset */
static int8_t fat_file_seek(struct fat_fs *fs, struct fat_file *file, uint32_t offs)
{
	uint16_t last = file->recent_offs / (uint32_t)FAT12_SECTOR_SIZE;
	uint16_t new = offs / (uint32_t)FAT12_SECTOR_SIZE;

	if (new < last) {
		/* Have to start all over again*/
		last = 0;
		file->recent_cluster = file->dentry->cluster;
	}

	while (new > last) {
		uint16_t c;
		int ret = fat_fat_get(fs, file->recent_cluster, &c);
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


static int8_t fat_file_dir_access(struct fat_fs *fs, struct fat_file *dir, uint16_t idx, uint16_t *sector, uint16_t *offset)
{
	size_t len = 0;
	uint32_t offs = (uint32_t)idx * sizeof(struct fat_dentry);
	int err;

	if (dir != NULL && !(dir->dentry->attr & FAT12_ATTR_DIR)) {
		return -ENOTDIR;
	}

	/* Root dir has to be processed separatelly, as it has negative cluster index */
	if (dir == NULL) {
		*sector = 1 + (FAT12_FAT_COPIES * FAT12_FAT_SIZE) + (offs / FAT12_SECTOR_SIZE);
	}
	else {
		err = fat_file_seek(fs, dir, offs);
		if (err < 0) {
			return err;
		}

		if (err > 0) {
			/* EOF */
			return -ENOENT;
		}

		*sector = CLUSTER2SECTOR(dir->recent_cluster);
	}

	*offset = offs % FAT12_SECTOR_SIZE;

	return 0;
}

static int8_t fat_file_dir_read(struct fat_fs *fs, struct fat_file *dir, struct fat_dentry *entry, uint16_t idx)
{
	uint16_t sector, offset;
	int8_t err = fat_file_dir_access(fs, dir, idx, &sector, &offset);

	if (err < 0) {
		return err;
	}

	err = fat_read_sector(fs, sector);
	if (err < 0) {
		return err;
	}

	memcpy(entry, fs->sbuff + offset, sizeof(*entry));

	return 0;
}

static int8_t fat_file_dir_write(struct fat_fs *fs, struct fat_file *dir, const struct fat_dentry *entry, uint16_t idx)
{
	uint16_t sector, offset;
	int8_t err = fat_file_dir_access(fs, dir, idx, &sector, &offset);

	if (err < 0) {
		return err;
	}

	err = fat_read_sector(fs, sector);
	if (err < 0) {
		return err;
	}

	memcpy(fs->sbuff + offset, entry, sizeof(*entry));

	return fat_write_sector(fs, sector);
}

static uint8_t fat_file_namelen(const char *str, uint8_t max)
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

static int8_t fat_file_name_cmp(const struct fat_dentry *entry, const char *path)
{
	uint8_t i, pos;
	char c;
	uint8_t len = fat_file_namelen(entry->fname, sizeof(entry->fname));

	for (i = 0; i < len; ++i) {
		c = toupper(path[i]);
		if (c == '/' || c == '\0') {
			return 1;
		}

		if (entry->fname[i] != c) {
			return 1;
		}
	}

	len = fat_file_namelen(entry->extension, sizeof(entry->extension));

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

static void fat_file_init(struct fs_file *file, uint8_t type, const struct fat_dentry *dentry)
{
	memset(file, 0, sizeof(*file));

	file->attr = type;
	if (dentry != NULL) {
		file->file.fat.dentry_storage = *dentry;
		file->file.fat.dentry = &file->file.fat.dentry_storage;
	}
	else {
		file->file.fat.dentry = NULL;
	}
	file->file.fat.idx = 0;
	file->file.fat.recent_cluster = 0xFFFF;
	file->file.fat.recent_offs = 0xFFFFFFFL;
	file->fnext = NULL;
	file->fprev = NULL;
	file->nlinks = 1;
	file->nrefs = 1;
	file->op = &fat_op;
}






static int8_t fat_op_open(struct fs_ctx *ctx, struct fs_file *file, const char *name, struct fs_file *dir, int8_t create, uint8_t attr)
{
	uint16_t idx;
	struct fat_dentry dentry;
	uint16_t free_idx = 0xffff;

	for (idx = 0; ; ++idx) {
		int ret = fat_file_dir_read(&ctx->fs.fat, &dir->file.fat, &dentry, idx);
		if (ret < 0) {
			return ret;
		}
		if (dentry.fname[0] == 0x00) {
			break;
		}
		if (dentry.fname[0] == 0xE5) {
			/* Deleted entry */
			free_idx = idx;
			continue;
		}

		if (!fat_file_name_cmp(&dentry, name)) {
			file->file.fat.dentry_storage = dentry;
			file->file.fat.dentry = &file->file.fat.dentry_storage;
			file->file.fat.recent_cluster = 0;
			file->file.fat.recent_offs = 0;
			file->file.fat.idx = idx;
			return 0;
		}
	}

	if (!create) {
		return -ENOENT;
	}

	/* TODO create */
	(void)attr;
	return -ENOSYS;
}

static int8_t fat_op_close(struct fs_ctx *ctx, struct fs_file *file)
{
	(void)ctx;
	(void)file;
	return 0;
}

static int16_t fat_op_read(struct fs_ctx *ctx, struct fs_file *file, void *buff, size_t bufflen, uint32_t offs)
{
	uint16_t sector;
	size_t len = 0;

	if (file == NULL || (file->file.fat.dentry->attr & FAT12_ATTR_DIR)) {
		return -EISDIR;
	}

	if (offs >= file->file.fat.dentry->size) {
		return 0;
	}
	else if (offs + (uint32_t)bufflen > file->file.fat.dentry->size) {
		bufflen = file->file.fat.dentry->size - offs;
	}
	if (bufflen > 0x7FFF) {
		/* Reval has to fit in int (int16_t) */
		bufflen = 0x7FFF;
	}

	while (len < bufflen) {
		int err = fat_file_seek(&ctx->fs.fat, &file->file.fat, offs);
		if (err < 0) {
			return -1;
		}

		if (err > 0) {
			/* EOF */
			break;
		}

		sector = CLUSTER2SECTOR(file->file.fat.recent_cluster);

		if (fat_read_sector(&ctx->fs.fat, sector) < 0) {
			return -1;
		}

		uint16_t pos = offs % FAT12_SECTOR_SIZE;
		uint16_t chunk = FAT12_SECTOR_SIZE - pos;

		if (chunk > bufflen - len) {
			chunk = bufflen - len;
		}

		memcpy((uint8_t *)buff + len, ctx->fs.fat.sbuff + pos, chunk);

		len += chunk;
		offs += chunk;
	}

	return (int16_t)len;
}

static int8_t fat_op_truncate(struct fs_ctx *ctx, struct fs_file *file, uint32_t size)
{
	uint32_t old_size = file->file.fat.dentry->size;
	int err;

	if (file->file.fat.dentry->size == size) {
		return 0;
	}

	if ((file->file.fat.dentry->size / (uint32_t)FAT12_SECTOR_SIZE) == (size / (uint32_t)FAT12_SECTOR_SIZE)) {
		/* No need to add/remove clusters, just init the space */
		if (file->file.fat.dentry->size < size) {
			err = fat_file_seek(&ctx->fs.fat, &file->file.fat, file->file.fat.dentry->size - 1);
			if (err < 0) {
				return err;
			}

			err = fat_read_sector(&ctx->fs.fat, CLUSTER2SECTOR(file->file.fat.recent_cluster));
			if (err < 0) {
				return err;
			}

			uint16_t chunk = size - file->file.fat.dentry->size;
			uint16_t offs = file->file.fat.dentry->size % FAT12_SECTOR_SIZE;

			memset(ctx->fs.fat.sbuff + offs, 0, chunk);

			err = fat_write_sector(&ctx->fs.fat, CLUSTER2SECTOR(file->file.fat.recent_cluster));
			if (err < 0) {
				return err;
			}
		}
	}
	else if (file->file.fat.dentry->size < size) {
		err = fat_file_seek(&ctx->fs.fat, &file->file.fat, file->file.fat.dentry->size - 1);
		if (err < 0) {
			return err;
		}

		uint16_t start = file->file.fat.recent_cluster;

		/* Try to find new cluster after the current last */
		while (file->file.fat.dentry->size < size) {
			uint16_t ncluster, ccluster = file->file.fat.recent_cluster + 1;
			uint8_t retry = 0;

			for (ccluster = file->file.fat.recent_cluster + 1; ; ++ccluster) {
				if (fat_fat_get(&ctx->fs.fat, ccluster, &ncluster) < 0) {
					if (retry) {
						/* No space? Restore previous file and FAT state */
						(void)fat_op_truncate(ctx, file, old_size);
						return -ENOSPC;
					}
					else {
						/* Try again from the FAT start */
						retry = 1;
						ccluster = 0;
					}
				}

				if (retry && (ccluster == file->file.fat.recent_cluster)) {
					return -ENOSPC;
				}

				if (ncluster == CLUSTER_FREE) {
					break;
				}
			}

			/* Add a new cluster to the chain */
			if (fat_fat_set(&ctx->fs.fat, start, ccluster) < 0) {
				(void)fat_op_truncate(ctx, file, old_size);
				return -EIO;
			}

			if (fat_fat_set(&ctx->fs.fat, ccluster, CLUSTER_END) < 0) {
				(void)fat_op_truncate(ctx, file, old_size);
				return -EIO;
			}

			/* Zero-out new cluster */
			memset(ctx->fs.fat.sbuff, 0, sizeof(ctx->fs.fat.sbuff));
			if (fat_write_sector(&ctx->fs.fat, CLUSTER2SECTOR(ccluster)) < 0) {
				(void)fat_op_truncate(ctx, file, old_size);
				return -EIO;
			}

			file->file.fat.dentry->size += FAT12_SECTOR_SIZE;
			start = ccluster;
		}
	}
	else {
		uint16_t ccluster = file->file.fat.dentry->cluster;
		uint16_t prev = ccluster;
		for (uint32_t csize = 0; csize < size; csize += FAT12_SECTOR_SIZE) {
			prev = ccluster;
			if (fat_fat_get(&ctx->fs.fat, prev, &ccluster) < 0) {
				return -EIO;
			}
		}

		if (ccluster != CLUSTER_END) {
			/* ccluster have the rest of the chain, prev is new end */
			fat_fat_set(&ctx->fs.fat, prev, CLUSTER_END);

			while (ccluster != CLUSTER_END) {
				prev = ccluster;
				if (fat_fat_get(&ctx->fs.fat, prev, &ccluster) < 0) {
					/* We have leaked clusters and can't do anything about it */
					return -EIO;
				}
				if (fat_fat_set(&ctx->fs.fat, prev, CLUSTER_FREE) < 0) {
					return -EIO;
				}
			}
		}
	}

	file->file.fat.dentry->size = size;

	return fat_file_dir_write(&ctx->fs.fat, &file->parent->file.fat, file->file.fat.dentry, file->file.fat.idx);
}

static int16_t fat_op_write(struct fs_ctx *ctx, struct fs_file *file, const void *buff, size_t bufflen, uint32_t offs)
{
	size_t len = 0;
	int8_t err;

	if (file == NULL || (file->file.fat.dentry->attr & FAT12_ATTR_DIR)) {
		return -EISDIR;
	}

	if (file->file.fat.dentry->size < offs + (uint32_t)bufflen) {
		err = fat_op_truncate(ctx, file, offs + (uint32_t)bufflen);
		if (err < 0) {
			return err;
		}
	}

	while (len < bufflen) {
		err = fat_file_seek(&ctx->fs.fat, &file->file.fat, offs + len);
		if (err < 0) {
			return err;
		}

		uint16_t missalign = offs % (uint32_t)FAT12_SECTOR_SIZE;

		if (missalign || (bufflen - len < FAT12_SECTOR_SIZE)) {
			err = fat_read_sector(&ctx->fs.fat, CLUSTER2SECTOR(file->file.fat.recent_cluster));
			if (err < 0) {
				return err;
			}
		}

		size_t chunk = FAT12_SECTOR_SIZE - missalign;
		if (chunk > bufflen - len) {
			chunk = bufflen - len;
		}

		memcpy(ctx->fs.fat.sbuff + missalign, (uint8_t *)buff + len, chunk);

		err = fat_write_sector(&ctx->fs.fat, CLUSTER2SECTOR(file->file.fat.recent_cluster));
		if (err < 0) {
			return err;
		}

		offs += chunk;
		len += chunk;
	}

	return len;
}

static int32_t fat_attr2epoch(uint16_t date, uint16_t time)
{
	/* TODO */
	(void)date;
	(void)time;
	return 0;
}

static int8_t fat_op_readdir(struct fs_ctx *ctx, struct fs_file *file, struct fs_dentry *dentry, uint16_t idx)
{
	struct fat_dentry fentry;
	int err = fat_file_dir_read(&ctx->fs.fat, &file->file.fat, &fentry, idx);
	if (!err) {
		dentry->atime = fat_attr2epoch(fentry.adate, 0);
		dentry->ctime = fat_attr2epoch(fentry.cdate, fentry.ctime);
		dentry->mttime = fat_attr2epoch(fentry.mdate, fentry.mtime);

		memcpy(dentry->name, fentry.fname, sizeof(fentry.fname));
		uint8_t pos = sizeof(fentry.fname);
		while (dentry->name[pos] == ' ' && pos) {
			--pos;
		}

		if (!pos) {
			return -EIO;
		}

		strncpy(dentry->name + pos + 1, fentry.extension, sizeof(fentry.extension));
		pos += sizeof(fentry.extension);
		while (dentry->name[pos] == ' ') {
			--pos;
		}

		dentry->name[pos - 1] = '\0';

		/* TODO - convert or adopt FATness? */
		dentry->attr = fentry.attr;
		dentry->size = fentry.size;
	}
	return err;
}

static int8_t fat_op_move(struct fs_ctx *ctx, struct fs_file *file, struct fs_file *ndir, const char *name)
{
	/* TODO */
	(void)ctx;
	(void)file;
	(void)ndir;
	(void)name;
	return -ENOSYS;
}

static int8_t fat_op_remove(struct fs_ctx *ctx, struct fs_file *file)
{
	/* TODO */
	(void)ctx;
	(void)file;
	return -ENOSYS;
}

static int8_t fat_op_set_attr(struct fs_ctx *ctx, struct fs_file *file, uint8_t attr, uint8_t mask)
{
	/* TODO */
	(void)ctx;
	(void)file;
	(void)attr;
	(void)mask;
	return -ENOSYS;
}

static int8_t fat_op_mount(struct fs_ctx *ctx, struct fs_cb *cb, struct fs_file *parent, struct fs_file *rootdir)
{
	int err;
	uint16_t t16;
	uint8_t t8;

	ctx->fs.fat.cb = *cb;
	ctx->fs.fat.sno = 0xFFFF;

	/* Read and parse boot sector */
	err = fat_read_sector(&ctx->fs.fat, 0);
	if (err < 0) {
		return err;
	}

	/* Check if the filesystem is present and
	 * what we expect it to be. FAT12 is LE,
	 * Z180 is LE too, ignore endianess. */

	/* Bytes per sector */
	memcpy(&t16, ctx->fs.fat.sbuff + 11, sizeof(t16));
	if (t16 != FAT12_SECTOR_SIZE) {
		return -EIO;
	}

	/* Sectors per cluster */
	t8 = *(ctx->fs.fat.sbuff + 13);
	if (t8 != 1) {
		return -EIO;
	}

	/* Number of reserved sectors */
	memcpy(&t16, ctx->fs.fat.sbuff + 14, sizeof(t16));
	if (t16 != 1) {
		return -EIO;
	}

	/* Number of FATs */
	t8 = *(ctx->fs.fat.sbuff + 16);
	if (t8 != 2) {
		return -EIO;
	}

	/* Maximum number of root directory entries */
	memcpy(&t16, ctx->fs.fat.sbuff + 17, sizeof(t16));
	if (t16 != 224) {
		return -EIO;
	}

	/* Total sector count */
	memcpy(&ctx->fs.fat.size, ctx->fs.fat.sbuff + 18, sizeof(t16));

	/* Sectors per FAT */
	memcpy(&t16, ctx->fs.fat.sbuff + 22, sizeof(t16));
	if (t16 != 9) {
		return -EIO;
	}

	/* Sectors per track */
	memcpy(&t16, ctx->fs.fat.sbuff + 24, sizeof(t16));
	if (t16 != 18) {
		return -EIO;
	}

	/* Number of heads */
	memcpy(&t16, ctx->fs.fat.sbuff + 26, sizeof(t16));
	if (t16 != 2) {
		return -EIO;
	}

	/* Ignore rest of the fields - most likely
	 * not valid anyway. */

	fat_file_init(rootdir, S_IFDIR, NULL);

	/* FAT12 does not have physical . and .. entries in rootdir */
	(void)parent;

	return 0;
}

static int8_t fat_op_unmount(struct fs_ctx *ctx)
{
	(void)ctx;
	return 0;
}
