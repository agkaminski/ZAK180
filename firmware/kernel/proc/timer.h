/* ZAK180 Firmaware
 * Kernel systick
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#ifndef KERNEL_TIMER_H_
#define KERNEL_TIMER_H_

#include <stdint.h>

#include "hal/cpu.h"

#define SYSTICK_INTERVAL 10 /* ms */

typedef int64_t ktime_t;

ktime_t _timer_get(void);

ktime_t timer_get(void);

void timer_irq_handler(struct cpu_context *context);

void timer_init(void);

#endif
