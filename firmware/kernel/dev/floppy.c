/* ZAK180 Firmaware
 * Floppy block device
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#include <errno.h>
#include <string.h>
#include "floppy.h"
#include "driver/floppy.h"
#include "lib/errno.h"

#define SECTOR_SIZE 512

/* Temporary solution - 1 sector write-through cache */
struct {
	uint8_t sector[SECTOR_SIZE];
	uint16_t sno;
} cache;

static int blk_floppy_read(off_t offs, void *buff, size_t bufflen);
static int blk_floppy_write(off_t offs, const void *buff, size_t bufflen);
static int blk_floppy_sync(off_t offs, off_t len);

static const struct dev_blk blk_floppy = {
	.read = blk_floppy_read,
	.write = blk_floppy_write,
	.sync = blk_floppy_sync,
	.size = 1474560UL
};

static uint16_t offs2lba(uint32_t offs)
{
	return offs / SECTOR_SIZE;
}

static int update_cache(uint16_t sector)
{
	if (cache.sno != sector) {
		if (floppy_read_sector(sector, cache.sector) < 0) {
			return -EIO;
		}

		cache.sno = sector;
	}

	return 0;
}

static int blk_floppy_read(off_t offs, void *buff, size_t bufflen)
{
	if (offs >= blk_floppy.size) {
		return 0;
	}

	if (offs + bufflen >= blk_floppy.size) {
		bufflen = blk_floppy.size - offs;
	}

	size_t len = 0;
	while (len < bufflen) {
		uint16_t sector = offs2lba(offs);
		if (update_cache(sector) < 0) {
			return -EIO;
		}

		uint16_t pos = offs % SECTOR_SIZE;
		uint16_t chunk = SECTOR_SIZE - pos;

		if (chunk > bufflen - len) {
			chunk = bufflen - len;
		}

		memcpy((uint8_t *)buff + len, cache.sector + pos, chunk);

		len += chunk;
		offs += chunk;
	}

	return len;
}

static int blk_floppy_write(off_t offs, const void *buff, size_t bufflen)
{
	if (offs >= blk_floppy.size) {
		return 0;
	}

	if (offs + bufflen >= blk_floppy.size) {
		bufflen = blk_floppy.size - offs;
	}

	size_t len = 0;
	while (len < bufflen) {
		uint16_t sector = offs2lba(offs);
		if (update_cache(sector) < 0) {
			return -EIO;
		}

		uint16_t pos = offs % SECTOR_SIZE;
		uint16_t chunk = SECTOR_SIZE - pos;

		if (chunk > bufflen - len) {
			chunk = bufflen - len;
		}

		memcpy(cache.sector + pos, (uint8_t *)buff + len, chunk);

		len += chunk;
		offs += chunk;

		if (floppy_write_sector(sector, cache.sector) < 0) {
			return -EIO;
		}
	}

	return len;
}

static int blk_floppy_sync(off_t offs, off_t len)
{
	/* TODO when write-back cache is implemented */
	return 0;
}

int blk_floppy_init(struct dev_blk *blk)
{
	cache.sno = 0xFFFF;

	*blk = blk_floppy;
	return floppy_init();
}
