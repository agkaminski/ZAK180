/* ZAK180 Firmaware
 * Kernel locks
 * Copyright: Aleksander Kaminski, 2024-2025
 * See LICENSE.md
 */

#include <stddef.h>

#include "thread.h"
#include "proc/lock.h"

#include "lib/errno.h"
#include "lib/assert.h"

static int8_t _lock_try(struct lock *lock)
{
	if (lock->locked) {
		return -EAGAIN;
	}

	lock->locked = 1;

	return 0;
}

void _lock_lock(struct lock *lock)
{
	assert(lock != NULL);

	while (_lock_try(lock) < 0) {
		_thread_wait(&lock->queue, 0);
	}
}

void _lock_unlock(struct lock *lock)
{
	assert(lock != NULL);

	lock->locked = 0;
	_thread_signal(&lock->queue);
}

int8_t lock_try(struct lock *lock)
{
	assert(lock != NULL);

	thread_critical_start();
	int ret = _lock_try(lock);
	thread_critical_end();

	return ret;
}

void lock_lock(struct lock *lock)
{
	assert(lock != NULL);

	thread_critical_start();
	_lock_lock(lock);
	thread_critical_end();
}

void lock_unlock(struct lock *lock)
{
	assert(lock != NULL);

	thread_critical_start();
	lock->locked = 0;
	_thread_signal_yield(&lock->queue);
}

void lock_init(struct lock *lock)
{
	assert(lock != NULL);

	lock->queue = NULL;
	lock->locked = 0;
}
