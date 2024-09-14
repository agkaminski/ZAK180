/* ZAK180 Firmaware
 * Kernel systick
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#ifndef KERNEL_TIMER_H_
#define KERNEL_TIMER_H_

#include <stdint.h>

#define SYSTICK_INTERVAL 10 /* ms */

typedef uint64_t ktime_t;

ktime_t _timer_get(void);

ktime_t timer_get(void);

void timer_irq_handler(void);

void timer_init(void);

#endif
