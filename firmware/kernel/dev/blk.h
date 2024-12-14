/* ZAK180 Firmaware
 * Block device API
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#ifndef DEV_BLK_H_
#define DEV_BLK_H_

#include <stddef.h>
#include "lib/types.h"

struct dev_blk {
	int (*read)(off_t offs, void *buff, size_t bufflen);
	int (*write)(off_t offs, const void *buff, size_t bufflen);
	int (*sync)(off_t offs, off_t len);
	off_t size;
};

#endif
