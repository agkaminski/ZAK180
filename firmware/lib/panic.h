/* Panic
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#ifndef LIB_PANIC_H_
#define LIB_PANIC_H_

#include <stdio.h>
#include "hal/cpu.h"

#define panic() \
	do { \
		printf("Kernel panic! Halting forever.\r\n"); \
		_DI; \
		_HALT;\
	} while (0)

#endif
