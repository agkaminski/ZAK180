/* ZAK180 Firmaware
 * Kernel threads
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#include <stddef.h>

#include "thread.h"
#include "memory.h"
#include "../driver/critical.h"
#include "../driver/mmu.h"
#include "../lib/errno.h"

static struct {
	struct thread *threads;
	struct thread *sleeping;
	struct thread *ready[THREAD_PRIORITY_NO];
	struct thread *current;

	uint16_t idcntr;

	struct thread idle;

	volatile int8_t schedule;
} common;

static void _thread_enqueue(struct thread **queue, struct thread *thread, ktime_t wakeup)
{
	struct thread *curr = *queue, *prev = NULL;

	thread->wakeup = wakeup;
	thread->state = THREAD_STATE_SLEEP;

	while (curr != NULL) {
		if (wakeup) {
			if (wakeup > curr->wakeup) {
				break;
			}
		}
		prev = curr;
		curr = curr->qnext;
	}

	if (curr != NULL) {
		thread->qnext = curr->qnext;
		curr->qnext = thread;
	}
	else if (prev != NULL) {
		thread->qnext = NULL;
		prev->qnext = thread;
	}
	else {
		*queue = thread;
		thread->qnext = NULL;
	}
}

static void _thread_dequeue(struct thread **queue, struct thread *thread)
{
	if (*queue == thread) {
		*queue = thread->qnext;
	}
	else {
		struct thread *curr = *queue;

		while (curr != NULL && curr->qnext != thread) {
			curr = curr->qnext;
		}

		if (curr == NULL) {
			return;
		}

		curr->qnext = thread->qnext;
	}

	thread->qnext = NULL;
	thread->wakeup = 0;
	thread->state = THREAD_STATE_READY;

	if (common.ready[thread->priority] == NULL) {
		common.ready[thread->priority] = thread;
	}
	else {
		struct thread *curr = common.ready[thread->priority];
		while (curr->qnext != NULL) {
			curr = curr->qnext;
		}

		curr->qnext = thread;
	}
}

static void _threads_add_ready(struct thread *thread)
{
	if (common.ready[thread->priority] == NULL) {
		common.ready[thread->priority] = thread;
	}
	else {
		struct thread *it = common.ready[thread->priority];
		while (it->qnext != NULL) {
			it = it->qnext;
		}

		it->qnext = thread;
	}

	thread->qnext = NULL;
	thread->state = THREAD_STATE_READY;
}

static void _thread_schedule(struct cpu_context *context)
{
	struct thread *prev = common.current;

	if (!common.schedule) {
		return;
	}

	/* Put current thread */
	if (common.current != NULL) {
		common.current->context = context;

		_threads_add_ready(common.current);
	}

	/* Select new thread */
	struct thread *selected = NULL;
	for (uint8_t priority = 0; priority < THREAD_PRIORITY_NO; ++priority) {
		if (common.ready[priority] != NULL) {
			selected = common.ready[priority];
			common.ready[priority] = selected->qnext;
			selected->qnext = NULL;
			break;
		}
	}

	if (selected == NULL) {
		_HALT;
	}

	common.current = selected;
	selected->state = THREAD_STATE_ACTIVE;

	if (selected != prev) {
		/* Map selected thread stack space into the scratch page */
		uint8_t *scratch = mmu_map_scratch(selected->stack_page, NULL);
		struct cpu_context *selctx = (void *)((uint8_t *)selected->context - PAGE_SIZE);

		/* Switch context */
		context->nsp = selctx->sp;
		context->nmmu = selctx->mmu;
		context->nlayout = selctx->layout;
	}
}

void _thread_on_tick(struct cpu_context *context)
{
	ktime_t now = timer_get();

	struct thread *it = common.sleeping;
	while (it != NULL) {
		if (it->wakeup >= now) {
			_thread_dequeue(&common.sleeping, it);
			_threads_add_ready(it);
		}
		else {
			break;
		}
	}

	_thread_schedule(context);
}

static void thread_context_create(struct thread *thread, uint16_t entry, void *arg)
{
	uint8_t *scratch = mmu_map_scratch(thread->stack_page, NULL);
	struct cpu_context *tctx = (void *)(scratch + PAGE_SIZE - sizeof(struct cpu_context));

	tctx->pc = entry;
	tctx->af = 0;
	tctx->bc = 0;
	tctx->de = 0;
	tctx->hl = (uint16_t)arg;
	tctx->ix = 0;
	tctx->iy = 0;

	/* TODO set mmu layout according to the process */
	tctx->layout = CONTEXT_LAYOUT_KERNEL;
	tctx->mmu = (uint16_t)(thread->stack_page - (tctx->layout >> 4)) << 8;

	thread->context = (void *)((uint8_t *)tctx + PAGE_SIZE);
	tctx->sp = (uint16_t)((uint8_t *)thread->context + 12);
}

static void thread_idle(void *arg)
{
	(void)arg;

	while (1) {
		_HALT;
	}
}

int thread_create(struct thread *thread, uint8_t priority, void (*entry)(void * arg), void *arg)
{
	thread->qnext = NULL;
	thread->id = ++common.idcntr;
	thread->refs = 1;
	thread->priority = priority;
	thread->exit = 0;
	thread->wakeup = 0;

	thread->stack_page = memory_alloc(NULL, 1);
	if (thread->stack_page == 0) {
		return -ENOMEM;
	}

	thread_context_create(thread, (uint16_t)entry, arg);

	if (common.schedule) {
		_CRITICAL_START;
	}
	_threads_add_ready(thread);
	if (common.schedule) {
		_CRITICAL_END;
	}

	return 0;
}

void thread_start(void)
{
	common.schedule = 1;
}

void thread_init(void)
{
	thread_create(&common.idle, THREAD_PRIORITY_NO - 1, thread_idle, NULL);
}
