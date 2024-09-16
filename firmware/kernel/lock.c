/* ZAK180 Firmaware
 * Kernel locks
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#include <stddef.h>

#include "thread.h"
#include "lock.h"

#include "../lib/errno.h"

static int _lock_try(struct lock *lock)
{
	if (lock->locked) {
		return -EAGAIN;
	}

	lock->locked = 1;

	return 0;
}

int lock_try(struct lock *lock)
{
	thread_critical_start();
	int ret = _lock_try(lock);
	thread_critical_end();

	return ret;
}

void lock_lock(struct lock *lock)
{
	thread_critical_start();
	while (lock_try(lock) < 0) {
		_thread_wait(&lock->queue, 0);
	}
	thread_critical_end();
}

void lock_unlock(struct lock *lock)
{
	thread_critical_start();
	lock->locked = 0;
	_thread_signal_yield(lock->queue);
}

void lock_init(struct lock *lock)
{
	lock->queue = NULL;
	lock->locked = 0;
}
