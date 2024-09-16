/* ZAK180 Firmaware
 * Kernel locks
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#ifndef KERNEL_LOCK_H_
#define KERNEL_LOCK_H_

#include <stdint.h>

#include "thread.h"

struct lock {
	struct thread *queue;
	int8_t locked;
};

int lock_try(struct lock *lock);

void lock_lock(struct lock *lock);

void lock_unlock(struct lock *lock);

void lock_init(struct lock *lock);

#endif
