/* ZAK180 Firmaware
 * Kernel systick
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#include "timer.h"
#include "thread.h"
#include "driver/prt.h"
#include "driver/critical.h"

static struct {
	ktime_t jiffies;
} common;

ktime_t _timer_get(void)
{
	return common.jiffies + prt_val_to_ms(_prt0_timer_get());
}

ktime_t timer_get(void)
{
	critical_start();
	ktime_t ret = _timer_get();
	critical_end();

	return ret;
}

void timer_irq_handler(struct cpu_context *context)
{
	common.jiffies += SYSTICK_INTERVAL;
	_thread_on_tick(context);
}

void timer_init(void)
{
	prt0_init(SYSTICK_INTERVAL);
}
