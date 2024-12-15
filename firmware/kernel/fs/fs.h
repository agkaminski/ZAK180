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

#include "dev/blk.h"
#include "proc/lock.h"
#include "proc/timer.h"
#include "fs/fat.h"

/* File type */
#define S_IFMT   0x1F
#define S_IFREG  0x10
#define S_IFBLK  0x08
#define S_IFDIR  0x04
#define S_IFCHR  0x02
#define S_IFIFO  0x01

/* Permissions */
#define S_IRWX 0xE0
#define S_IR   0x80
#define S_IW   0x40
#define S_IX   0x20

#define S_ISREG(m)  (((m) & S_IFMT) == S_IFREG)
#define S_ISBLK(m)  (((m) & S_IFMT) == S_IFBLK)
#define S_ISDIR(m)  (((m) & S_IFMT) == S_IFDIR)
#define S_ISCHR(m)  (((m) & S_IFMT) == S_IFCHR)
#define S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)

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
	struct fs_file *fnext;
	struct fs_file *fprev;

	/* File type and permissions */
	uint8_t attr;

	/* Internal flags */
	uint8_t flags;

	/* File system context */
	struct fs_ctx *ctx;
};


int8_t fs_open(const char *path, struct fs_file **file, int8_t mode, uint8_t attr);
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

#endif
