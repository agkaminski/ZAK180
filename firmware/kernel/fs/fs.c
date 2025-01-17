/* ZAK180 Firmaware
 * Filesystem abstraction layer
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#include <stdarg.h>
#include <string.h>

#include "fs/fs.h"
#include "proc/lock.h"
#include "lib/errno.h"
#include "lib/assert.h"
#include "lib/list.h"
#include "mem/kmalloc.h"

#define FLAG_DELETE 0x1

static struct {
	struct lock lock;
	struct fs_file *root;
} common;

static void fs_file_get(struct fs_file *file)
{
	++file->nrefs;
}

static int8_t fs_file_put(struct fs_file *file)
{
	int8_t ret = 0;

	--file->nrefs;
	assert(file->nrefs >= 0);

	if (!file->nrefs) {
		lock_lock(&file->lock);
		assert(file->mountpoint == NULL);
		assert(file->chnext == NULL && file->chprev == NULL);

		if (file->parent != NULL) {
			lock_lock(&file->parent->lock);
			LIST_REMOVE(&file->parent->children, file, chnext, chprev);
			lock_unlock(&file->parent->lock);
			(void)fs_file_put(file->parent);
		}

		if (file->flags & FLAG_DELETE) {
			ret = file->ctx->op->remove(file);
		}
		lock_unlock(&file->lock);

		kfree(file);
	}

	return ret;
}

static struct fs_file *fs_file_spawn(uint8_t attr)
{
	struct fs_file *file = kmalloc(sizeof(struct fs_file));
	if (file != NULL) {
		memset(file, 0, sizeof(*file));
		file->attr = attr;
		file->nrefs = 1;
		lock_init(&file->lock);
	}
	return file;
}

static int8_t fs_namecmp(const char *path, const struct fs_file *f)
{
	for (size_t i = 0; ; ++i) {
		if (path[i] == '/' || path[i] == '\0') {
			if (f->name[i] == '\0') {
				return 0;
			}
		}

		if (path[i] != f->name[i]) {
			return -1;
		}
	}
}

int8_t fs_open(const char *path, struct fs_file **file, uint8_t mode, uint8_t attr)
{
	if ((path[0] != '/') || (mode & O_RDWR) && (mode & (O_RDONLY | O_WRONLY))) {
		return -EINVAL;
	}

	lock_lock(&common.lock);
	struct fs_file *dir = common.root;
	if (dir == NULL) {
		/* rootfs not mounted */
		lock_unlock(&common.lock);
		return -ENOENT;
	}

	size_t pos = 0;

	while (path[pos] != '\0') {
		if (!S_ISDIR(dir->attr)) {
			lock_unlock(&common.lock);
			return -ENOTDIR;
		}

		struct fs_file *f = NULL;

		while (path[pos] == '/') ++pos;
		if (path[pos] == '\0') {
			break;
		}

		uint8_t found = 0;

		if (dir->children != NULL) {
			f = dir->children;

			do {
				if (!fs_namecmp(&path[pos], f)) {
					found = 1;
					break;
				}
				f = f->chnext;
			} while (f != dir->children && !found);
		}

		if (found) {
			dir = f;
			continue;
		}

		lock_lock(&dir->lock);
		/* TODO */
		lock_unlock(&dir->lock);
	}

	lock_unlock(&common.lock);

	return 0;
}

int8_t fs_close(struct fs_file *file)
{
	if (file->ctx->op->close == NULL) return -ENOSYS;

	lock_lock(&file->lock);
	int8_t ret = file->ctx->op->close(file);
	lock_unlock(&file->lock);

	lock_lock(&common.lock);
	ret = fs_file_put(file);
	lock_unlock(&common.lock);

	return ret;
}

int16_t fs_read(struct fs_file *file, void *buff, size_t bufflen, uint32_t offs)
{
	if (file->ctx->op->read == NULL) return -ENOSYS;

	lock_lock(&file->lock);
	int16_t ret = file->ctx->op->read(file, buff, bufflen, offs);
	lock_unlock(&file->lock);

	return ret;
}

int16_t fs_write(struct fs_file *file, const void *buff, size_t bufflen, uint32_t offs)
{
	if (file->ctx->op->write == NULL) return -ENOSYS;

	lock_lock(&file->lock);
	int16_t ret = file->ctx->op->write(file, buff, bufflen, offs);
	lock_unlock(&file->lock);

	return ret;
}

int8_t fs_truncate(struct fs_file *file, uint32_t size)
{
	if (file->ctx->op->truncate == NULL) return -ENOSYS;

	lock_lock(&file->lock);
	int8_t ret = file->ctx->op->truncate(file, size);
	lock_unlock(&file->lock);

	return ret;
}

int8_t fs_readdir(struct fs_file *file, struct fs_dentry *dentry, uint16_t idx)
{
	if (file->ctx->op->readdir == NULL) return -ENOSYS;
}

int8_t fs_move(struct fs_file *file, struct fs_file *ndir, const char *name)
{
	if (file->ctx->op->move == NULL) return -ENOSYS;
}

int8_t fs_remove(struct fs_file *file)
{
	if (file->ctx->op->remove == NULL) return -ENOSYS;

	lock_lock(&file->lock);
	file->flags |= FLAG_DELETE;
	lock_unlock(&file->lock);

	lock_lock(&common.lock);
	int8_t ret = fs_file_put(file);
	lock_unlock(&common.lock);

	return ret;
}

int8_t fs_set_attr(struct fs_file *file, uint8_t attr, uint8_t mask)
{
	int8_t ret = 0;

	lock_lock(&file->lock);
	if (file->ctx->op->set_attr != NULL) {
		ret = file->ctx->op->set_attr(file, attr, mask);
	}

	if (!ret) {
		file->attr = (file->attr & ~mask) | (attr & mask);
	}
	lock_unlock(&file->lock);

	return ret;
}

int8_t fs_ioctl(struct fs_file *file, int16_t op, ...)
{
	if (file->ctx->op->ioctl == NULL) return -ENOSYS;

	va_list args;
	va_start(args, op);
	lock_lock(&file->lock);
	int8_t ret = file->ctx->op->ioctl(file, op, args);
	lock_unlock(&file->lock);
	va_end(args);
	return ret;
}

int8_t fs_mount(struct fs_ctx *ctx, struct fs_file_op *op, struct dev_blk *cb, struct fs_file *dir)
{
	if (dir == NULL && common.root != NULL) {
		return -EINVAL;
	}

	struct fs_file *rootdir = fs_file_spawn(S_IFDIR | S_IR | S_IW);
	if (rootdir == NULL) {
		return -ENOMEM;
	}

	ctx->cb = cb;
	ctx->op = op;

	if (dir != NULL) {
		lock_lock(&dir->lock);
		if (dir->mountpoint != NULL) {
			lock_unlock(&dir->lock);
			kfree(rootdir);
			return -EINVAL;
		}
	}

	int8_t ret = ctx->op->mount(ctx, dir, rootdir);
	if (ret < 0) {
		kfree(rootdir);
		return ret;
	}

	rootdir->ctx = ctx;

	if (dir != NULL) {
		dir->mountpoint = rootdir;
		lock_unlock(&dir->lock);
	}
	else {
		common.root = rootdir;
	}

	return 0;
}

int8_t fs_unmount(struct fs_file *mountpoint)
{
	if (mountpoint->mountpoint == NULL) return -EINVAL;


}

void fs_init(void)
{
	lock_init(&common.lock);
}
