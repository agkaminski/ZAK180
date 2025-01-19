/* ZAK180 Firmaware
 * Filesystem abstraction layer
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#ifndef KERNEL_FS_H_
#define KERNEL_FS_H_

#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <time.h>

#include "dev/blk.h"
#include "proc/lock.h"
#include "proc/timer.h"
#include "fs/fat.h"

#include "include/fcntl.h"

#define FS_NAME_LENGTH_MAX 32

#define FS_TYPE_FAT 0x01

struct fs_file;
struct fs_ctx;

struct fs_dentry {
	uint8_t attr;
	ktime_t ctime;
	ktime_t atime;
	ktime_t mttime;
	uint32_t size;
	char name[FS_NAME_LENGTH_MAX];
};

struct fs_file_op {
	int8_t (*open)(struct fs_file *file, const char *name, struct fs_file *dir, int8_t create, uint8_t attr);
	int8_t (*close)(struct fs_file *file);
	int16_t (*read)(struct fs_file *file, void *buff, size_t bufflen, uint32_t offs);
	int16_t (*write)(struct fs_file *file, const void *buff, size_t bufflen, uint32_t offs);
	int8_t (*truncate)(struct fs_file *file, uint32_t size);
	int8_t (*readdir)(struct fs_file *dir, struct fs_dentry *dentry, uint16_t idx);
	int8_t (*move)(struct fs_file *file, struct fs_file *ndir, const char *name);
	int8_t (*remove)(struct fs_file *file);
	int8_t (*set_attr)(struct fs_file *file, uint8_t attr, uint8_t mask);
	int8_t (*ioctl)(struct fs_file *file, int16_t op, va_list arg);
	int8_t (*mount)(struct fs_ctx *ctx, struct fs_file *dir, struct fs_file *root);
	int8_t (*unmount)(struct fs_ctx *ctx);
};

struct fs_ctx {
	/* FS characteristic context storage */
	union  {
		struct fat_ctx fat;
		int dummy;
	};

	struct dev_blk *cb;
	const struct fs_file_op *op;
	uint8_t type;
};

struct fs_file {
	/* File system characteristic data storage */
	union {
		struct fat_file fat;
		int dummy; /* Fix SDCC retardness until more FSes are available */
	} file;

	/* Directory the file resides in */
	struct fs_file *parent;

	/* Directory mounted at this point, NULL for no mountpoint */
	struct fs_file *mountpoint;

	/* Synchronization */
	int8_t nrefs;
	struct lock lock;

	/* Children */
	struct fs_file *children;
	struct fs_file *chnext;
	struct fs_file *chprev;

	/* File type and permissions */
	uint8_t attr;

	/* Internal flags */
	uint8_t flags;

	off_t size;

	/* File system context */
	struct fs_ctx *ctx;

	char name[];
};


int8_t fs_open(const char *path, struct fs_file **file, uint8_t mode, uint8_t attr);
int8_t fs_close(struct fs_file *file);
int16_t fs_read(struct fs_file *file, void *buff, size_t bufflen, uint32_t offs);
int16_t fs_write(struct fs_file *file, const void *buff, size_t bufflen, uint32_t offs);
int8_t fs_truncate(struct fs_file *file, uint32_t size);
int8_t fs_readdir(struct fs_file *file, struct fs_dentry *dentry, uint16_t idx);
int8_t fs_move(struct fs_file *file, struct fs_file *ndir, const char *name);
int8_t fs_remove(struct fs_file *file);
int8_t fs_set_attr(struct fs_file *file, uint8_t attr, uint8_t mask);
int8_t fs_ioctl(struct fs_file *file, int16_t op, ...);
int8_t fs_mount(struct fs_ctx *ctx, struct fs_file_op *op, struct dev_blk *cb, struct fs_file *dir);
int8_t fs_unmount(struct fs_file *mountpoint);
void fs_init(void);

#endif
