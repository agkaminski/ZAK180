/* FAT12 driver
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#include <string.h>

#include "fat12.h"

#define CLUSTER2SECTOR(c) ((c) + FAT12_DATA_START - 2)
#define SECTOR2CLUSTER(s) ((x) + 2 - FAT12_DATA_START)

#define CLUSTER_FREE     0
#define CLUSTER_RESERVED 0xFF0
#define CLUSTER_END      0xFFF

int fat12_fat_get(struct fat12_fs *fs, uint16_t n, uint16_t *cluster)
{
	if (CLUSTER2SECTOR(n) > fs->size) {
		return -1;
	}

	uint16_t idx = (3 * n) / 2;
	uint16_t sector = 1 + (idx / FAT12_SECTOR_SIZE);
	uint16_t pos = idx & (FAT12_SECTOR_SIZE - 1);

	int ret = fs->cb.read_sector(sector, fs->sbuff);
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
		ret = fs->cb.read_sector(sector_next, fs->sbuff);
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

int fat12_fat_set(struct fat12_fs *fs,  uint16_t n, uint16_t cluster)
{
	if ((CLUSTER2SECTOR(n) > fs->size) || (cluster & 0xF000)) {
		return -1;
	}

	uint16_t idx = (3 * n) / 2;
	uint16_t sector = 1 + (idx / FAT12_SECTOR_SIZE);
	uint16_t pos = idx & (FAT12_SECTOR_SIZE - 1);

	int ret = fs->cb.read_sector(sector, fs->sbuff);
	if (ret < 0) {
		return ret;
	}

	if (n & 1) {
		fs->sbuff[pos] = (uint8_t)cluster >> 4;
	}
	else {
		fs->sbuff[pos] = (uint8_t)cluster;
	}

	++idx;

	uint16_t sector_next = 1 + (idx / FAT12_SECTOR_SIZE);
	pos = idx & (FAT12_SECTOR_SIZE - 1);

	if (sector != sector_next) {
		ret = fs->cb.write_sector(sector, fs->sbuff);
		if (ret < 0) {
			return ret;
		}

		ret = fs->cb.read_sector(sector_next, fs->sbuff);
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

	ret = fs->cb.write_sector(sector, fs->sbuff);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

int fat12_mount(struct fat12_fs *fs, struct fat12_cb *callback)
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
