/* ZAK180 Firmaware
 * Kernel threads
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#ifndef KERNEL_THREDS_H_
#define KERNEL_THREDS_H_

#include <stdint.h>

#include "timer.h"
#include "../driver/cpu.h"

#define THREAD_PRIORITY_NO 8

#define THREAD_STATE_ACTIVE 0
#define THREAD_STATE_READY  1
#define THREAD_STATE_SLEEP  2
#define THREAD_STATE_GHOST  3

#define CONTEXT_LAYOUT_KERNEL 0xFE
#define CONTEXT_LAYOUT_USER   0xF1

struct thread {
	/* Wait queue or ready list */
	struct thread *qnext;
	struct thread *qprev;

	/* Sleeping list */
	struct thread *snext;
	struct thread *sprev;

	struct thread **qwait;

	struct thread *idnext;
	uint8_t id;

	uint8_t refs : 5;
	uint8_t state : 3;
	uint8_t priority : 4;
	uint8_t exit : 4;

	ktime_t wakeup;

	struct cpu_context *context;
	uint8_t stack_page;
};

void thread_yield(void);

void thread_sleep(ktime_t wakeup);

void thread_sleep_relative(ktime_t sleep);

void _thread_on_tick(struct cpu_context *context);

int thread_create(struct thread *thread, uint8_t priority, void (*entry)(void * arg), void *arg);

void thread_start(void);

void thread_init(void);

#endif
