/* ZAK180 Firmaware
 * Kernel unit tests - cond wait
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#include <stdio.h>

#include "proc/timer.h"
#include "proc/thread.h"

static struct {
	struct thread thread[2];
	struct thread *queue;
} common;

static void waiter(void *arg)
{
	(void)arg;

	while (1) {
		thread_critical_start();
		_thread_wait(&common.queue, 0);
		thread_critical_end();
		printf("Wakeup %llu\r\n", timer_get() / 1000);
	}
}

static void pinger(void *arg)
{
	(void)arg;

	while (1) {
		thread_sleep_relative(1000);
		thread_critical_start();
		_thread_signal(&common.queue);
		thread_critical_end();
	}
}

void test_condwait(void)
{
	thread_create(&common.thread[0], 4, waiter, (void *)0);
	thread_create(&common.thread[1], 4, pinger, (void *)0);
}
