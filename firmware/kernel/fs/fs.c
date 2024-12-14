/* ZAK180 Firmaware
 * Filesystem abstraction layer
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#include <stdarg.h>

#include "fs.h"
#include "proc/lock.h"
#include "lib/errno.h"
#include "lib/assert.h"
#include "lib/list.h"
#include "mem/kmalloc.h"

static struct {
	struct lock lock;
	struct fs_file *root;
} common;

static void fs_file_get(struct fs_file *file)
{
	++file->nrefs;
}

static void fs_file_put(struct fs_file *file)
{
	--file->nrefs;
	assert(file->nrefs >= 0);

	if (!file->nrefs) {
		assert(file->mountpoint == NULL && file->mountfs == NULL);
		assert(file->fnext == NULL && file->fprev == NULL);

		if (file->parent != NULL) {
			struct fs_file *parent = file->parent;
			LIST_REMOVE(&parent, file, fnext, fprev);
			fs_file_put(file->parent);
		}

		kfree(file);
	}
}

int8_t fs_open(struct fs_file *file, const char *name, struct fs_file *dir, int8_t create, uint8_t attr)
{
	if (file->op->open == NULL) return -ENOSYS;
}

int8_t fs_close(struct fs_file *file)
{
	if (file->op->close == NULL) return -ENOSYS;
}

int16_t fs_read(struct fs_file *file, void *buff, size_t bufflen, uint32_t offs)
{
	if (file->op->read == NULL) return -ENOSYS;
}

int16_t fs_write(struct fs_file *file, const void *buff, size_t bufflen, uint32_t offs)
{
	if (file->op->write == NULL) return -ENOSYS;
}

int8_t fs_truncate(struct fs_file *file, uint32_t size)
{
	if (file->op->truncate == NULL) return -ENOSYS;
}

int8_t fs_readdir(struct fs_file *file, struct fs_dentry *dentry, uint16_t idx)
{
	if (file->op->readdir == NULL) return -ENOSYS;
}

int8_t fs_move(struct fs_file *file, struct fs_file *ndir, const char *name)
{
	if (file->op->move == NULL) return -ENOSYS;
}

int8_t fs_remove(struct fs_file *file)
{
	if (file->op->remove == NULL) return -ENOSYS;
}

int8_t fs_set_attr(struct fs_file *file, uint8_t attr, uint8_t mask)
{
	if (file->op->set_attr == NULL) return -ENOSYS;
	return file->op->set_attr(file->op->ctx, file, attr, mask);
}

int8_t fs_ioctl(struct fs_file *file, int16_t op, ...)
{
	if (file->op->ioctl == NULL) return -ENOSYS;

	va_list args;
	va_start(args, op);
	int8_t ret = file->op->ioctl(file->op->ctx, file, op, args);
	va_end(args);
	return ret;
}

int8_t fs_mount(struct fs_ctx *ctx, struct fs_cb *cb, struct fs_file *parent, struct fs_file *rootdir)
{
	if (rootdir == NULL && common.root != NULL) {
		/* rootfs already mounted */
		return -EINVAL;
	}

	switch (ctx->type) {
		case fs_fat:

		default:
			return -EINVAL;
	}
}

int8_t fs_unmount(struct fs_file *mountpoint)
{

}
