/* ZAK180 Firmaware
 * Floppy block device
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#ifndef DEV_FLOPPY_H_
#define DEV_FLOPPY_H_

#include "blk.h"

int blk_floppy_init(struct dev_blk *blk);

#endif
