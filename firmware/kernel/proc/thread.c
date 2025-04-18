/* ZAK180 Firmaware
 * Kernel threads
 * Copyright: Aleksander Kaminski, 2024-2025
 * See LICENSE.md
 */

#include <stddef.h>

#include "proc/thread.h"
#include "proc/lock.h"
#include "proc/process.h"

#include "mem/page.h"
#include "mem/kmalloc.h"

#include "driver/critical.h"
#include "driver/mmu.h"
#include "driver/critical.h"

#include "lib/errno.h"
#include "lib/list.h"
#include "lib/assert.h"
#include "lib/bheap.h"
#include "lib/id.h"

static struct {
	struct thread *ready[THREAD_PRIORITY_NO];
	struct thread *ghosts;
	struct thread *current;
	struct thread *irq_signaled;

	struct thread *sleeping_array[THREAD_COUNT_MAX];
	struct bheap sleeping;

	struct thread idle;

	volatile int8_t schedule;
} common;

void thread_critical_start(void)
{
	critical_start();
	assert(common.schedule);
	common.schedule = 0;
	critical_end();
}

void _thread_critical_end(void)
{
	assert(!common.schedule);
	common.schedule = 1;
}

void thread_critical_end(void)
{
	critical_start();
	assert(!common.schedule);
	common.schedule = 1;
	critical_end();
}

struct thread *thread_current(void)
{
	return common.current;
}

static int8_t _thread_wakeup_compare(void *v1, void *v2)
{
	struct thread *t1 = v1;
	struct thread *t2 = v2;

	if (t1->wakeup > t2->wakeup) {
		return 1;
	}
	else if (t1->wakeup < t2->wakeup) {
		return -1;
	}

	return 0;
}

static void _thread_sleeping_enqueue(time_t wakeup)
{
	common.current->wakeup = wakeup;
	common.current->state = THREAD_STATE_SLEEP;
	bheap_insert(&common.sleeping, common.current);
}

static void _threads_add_ready(struct thread *thread)
{
	LIST_ADD(&common.ready[thread->priority], thread, struct thread, qnext, qprev);

	thread->state = THREAD_STATE_READY;

	if (thread->wakeup) {
		bheap_extract(&common.sleeping, thread);
		thread->wakeup = 0;
	}
}

static void _thread_dequeue(struct thread *thread)
{
	assert(thread != NULL);

	if (thread->qwait != NULL) {
		LIST_REMOVE(thread->qwait, thread, struct thread, qnext, qprev);
		thread->qwait = NULL;
	}
	_threads_add_ready(thread);
}

static void _thread_kill(struct thread *thread)
{
	struct process *process = thread->process;

	/* Kernel threads do not end! */
	assert(process != NULL);
	thread->state = THREAD_STATE_GHOST;
	LIST_ADD(&process->ghosts, thread, struct thread, qnext, qprev);
	if (--process->thread_no == 0) {
		_process_zombify(process);
	}
}

void _thread_end(struct thread *thread)
{
	if (thread == NULL) {
		_thread_kill(common.current);
		_thread_yield();
	}
	else {
		thread->exit = 1;
	}
}

void thread_end(struct thread *thread)
{
	thread_critical_start();
	_thread_end(thread);
	thread_critical_end();
}

void thread_join_reap(struct process *process, struct thread *ghost)
{
	lock_lock(&process->lock);
	id_remove(&process->threads, &ghost->id);
	lock_unlock(&process->lock);

	page_free(ghost->stack_page, 1);
	kfree(ghost);
}

int8_t thread_join(struct process *process, id_t tid, time_t timeout)
{
	int8_t err = 0, found = 0;
	struct thread *ghost;

	thread_critical_start();
	do {
		while (process->ghosts == NULL && !err) {
			err = _thread_wait_relative(&process->reaper, timeout);
		}

		if (err < 0) {
			thread_critical_end();
			return err;
		}

		ghost = process->ghosts;
		do {
			if (tid < 0 || ghost->id.id == tid) {
				found = 1;
				break;
			}
			ghost = ghost->qnext;
		} while (ghost != process->ghosts);
	} while (!found);

	LIST_REMOVE(&process->ghosts, ghost, struct thread, qnext, qprev);
	thread_critical_end();

	thread_join_reap(process, ghost);

	return 0;
}

void thread_join_all(struct process *process)
{
	thread_critical_start();
	while (process->ghosts != NULL) {
		struct thread *ghost = process->ghosts;
		LIST_REMOVE(&process->ghosts, ghost, struct thread, qnext, qprev);
		thread_critical_end();

		thread_join_reap(process, ghost);

		thread_critical_start();
	}
	thread_critical_end();
}

void _thread_schedule(struct cpu_context *context)
{
	struct thread *prev = common.current;

	/* Put current thread */
	if (common.current != NULL) {
		common.current->context = context;

		if (common.current->state == THREAD_STATE_ACTIVE) {
			_threads_add_ready(common.current);
		}
	}

	/* Select new thread */
	for (uint8_t priority = 0; priority < THREAD_PRIORITY_NO; ++priority) {
		if (common.ready[priority] != NULL) {
			struct thread *selected = common.ready[priority];
			LIST_REMOVE(&common.ready[priority], selected, struct thread, qnext, qprev);

			/* Map selected thread stack space into the scratch page */
			/* Scratch page is one page before stack page */
			(void)mmu_map_scratch(selected->stack_page, NULL);
			struct cpu_context *selctx = (void *)((uint8_t *)selected->context - PAGE_SIZE);

			if ((selected->exit) && (selctx->layout != CONTEXT_LAYOUT_KERNEL)) {
				_thread_kill(selected);
			}
			else {
				common.current = selected;
				selected->state = THREAD_STATE_ACTIVE;

				/* Switch context */
				context->nsp = selctx->sp;
				context->nmmu = selctx->mmu;
				context->nlayout = selctx->layout;
				break;
			}
		}
	}

	_DI;
	common.schedule = 1;
}

static void _thread_set_return(struct thread *thread, int8_t value)
{
	assert(thread != NULL);
	assert(thread->state == THREAD_STATE_SLEEP);

	uint8_t *scratch = mmu_map_scratch(thread->stack_page, NULL);
	struct cpu_context *tctx = (void *)((uint8_t *)thread->context - PAGE_SIZE);
	tctx->af = (tctx->af & 0x0F) | ((uint16_t)(value) << 8);
}

int8_t _thread_reschedule(volatile uint8_t *scheduler_lock);

int8_t _thread_yield(void)
{
	return _thread_reschedule(&common.schedule);
}

void _thread_on_tick(struct cpu_context *context)
{
	if (common.schedule) {
		/* Put threads signaled by interrupts to the ready list */
		(void)_thread_broadcast(&common.irq_signaled);

		/* Allow HW IRQ to preempt the scheduler */
		common.schedule = 0;
		_EI;

		time_t now = _timer_get();
		struct thread *t;

		while (!bheap_peek(&common.sleeping, &t) && (t->wakeup <= now)) {
			_thread_set_return(t, -ETIME);
			_thread_dequeue(t);
		}

		_thread_schedule(context);
	}
}

int8_t thread_sleep(time_t wakeup)
{
	thread_critical_start();
	_thread_sleeping_enqueue(wakeup);
	return _thread_yield();
}

int8_t thread_sleep_relative(time_t sleep)
{
	return thread_sleep(timer_get() + sleep);
}

int8_t _thread_wait(struct thread **queue, time_t wakeup)
{
	assert(queue != NULL);

	LIST_ADD(queue, common.current, struct thread, qnext, qprev);

	common.current->wakeup = wakeup;
	common.current->state = THREAD_STATE_SLEEP;
	common.current->qwait = queue;

	if (wakeup) {
		_thread_sleeping_enqueue(wakeup);
	}

	int8_t ret = _thread_yield();
	thread_critical_start();

	return ret;
}

int8_t _thread_wait_relative(struct thread **queue, time_t timeout)
{
	time_t wakeup = timeout ? timer_get() + timeout : 0;
	return _thread_wait(queue, wakeup);
}

int8_t _thread_signal(struct thread **queue)
{
	assert(queue != NULL);

	if (*queue != NULL) {
		_thread_dequeue(*queue);
		return 1;
	}

	return 0;
}

/* Synchronized by irq disable */
void _thread_signal_irq(struct thread **queue)
{
	assert(queue != NULL);

	while (*queue != NULL) {
		struct thread *thread = *queue;

		assert(thread->wakeup == 0);

		LIST_REMOVE(thread->qwait, thread, struct thread, qnext, qprev);
		LIST_ADD(&common.irq_signaled, thread, struct thread, qnext, qprev);
		thread->qwait = &common.irq_signaled;
	}
}

int8_t _thread_signal_yield(struct thread **queue)
{
	assert(queue != NULL);

	if (_thread_signal(queue)) {
		(void)_thread_yield();
		return 1;
	}
	else {
		thread_critical_end();
	}

	return 0;
}

int8_t _thread_broadcast(struct thread **queue)
{
	assert(queue != NULL);

	int ret = 0;

	while (*queue != NULL) {
		_thread_dequeue(*queue);
		ret = 1;
	}

	return ret;
}

int8_t _thread_broadcast_yield(struct thread **queue)
{
	assert(queue != NULL);

	if (_thread_broadcast(queue)) {
		(void)_thread_yield();
		return 1;
	}
	else {
		thread_critical_end();
	}

	return 0;
}

static void thread_context_create(struct thread *thread, uint16_t entry, void *arg)
{
	assert(thread != NULL);

	uint8_t prev;
	uint8_t *scratch = mmu_map_scratch(thread->stack_page, &prev);
	struct cpu_context *tctx = (void *)(scratch + PAGE_SIZE - sizeof(struct cpu_context));

	tctx->pc = entry;
	tctx->af = 0;
	tctx->bc = 0;
	tctx->de = 0;
	tctx->hl = (uint16_t)arg;
	tctx->ix = 0;
	tctx->iy = 0;

	tctx->layout = CONTEXT_LAYOUT_KERNEL;
	tctx->mmu = (uint16_t)(thread->stack_page - (CONTEXT_LAYOUT_KERNEL >> 4)) << 8;

	thread->context = (void *)((uint8_t *)tctx + PAGE_SIZE);
	tctx->sp = (uint16_t)((uint8_t *)thread->context + 12);

	(void)mmu_map_scratch(prev, NULL);
}

static void thread_idle(void *arg)
{
	(void)arg;

	while (1) {
		_HALT;
	}
}

int8_t thread_create(struct thread *thread, id_t pid, uint8_t priority, void (*entry)(void *arg), void *arg)
{
	assert(thread != NULL);
	assert(entry != NULL);

	thread->qnext = NULL;
	thread->qwait = NULL;
	thread->priority = priority;
	thread->wakeup = 0;

	thread->stack_page = page_alloc(NULL, 1);
	if (thread->stack_page == 0) {
		return -ENOMEM;
	}

	if (pid) {
		struct process *p = process_get(pid);
		if (p == NULL) {
			page_free(thread->stack_page, 1);
			return -EINVAL;
		}

		lock_lock(&p->lock);
		int8_t err = id_insert(&p->threads, &thread->id);
		if (err != 0) {
			lock_unlock(&p->lock);
			process_put(p);
			page_free(thread->stack_page, 1);
			return err;
		}
		++p->thread_no;
		thread->process = p;
		lock_unlock(&p->lock);

		process_put(p);
	}

	thread_context_create(thread, (uint16_t)entry, arg);

	thread_critical_start();
	_threads_add_ready(thread);
	thread_critical_end();

	return 0;
}

void thread_init(void)
{
	common.schedule = 1;
	bheap_init(&common.sleeping, common.sleeping_array, THREAD_COUNT_MAX, _thread_wakeup_compare);
	thread_create(&common.idle, 0, THREAD_PRIORITY_NO - 1, thread_idle, NULL);
}
