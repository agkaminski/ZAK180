/* ZAK180 Firmaware
 * Kernel threads
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#ifndef KERNEL_THREDS_H_
#define KERNEL_THREDS_H_

#include <stdint.h>

struct thread_context {
	uint16_t sp;
	uint16_t mmu;
	uint16_t iy;
	uint16_t ix;
	uint16_t hl;
	uint16_t de;
	uint16_t bc;
	uint16_t af;
	uint16_t pc;
};

#endif
