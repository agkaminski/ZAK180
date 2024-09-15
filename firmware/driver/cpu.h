/* ZAK180 Firmaware
 * CPU
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#ifndef DRIVER_CPU_H_
#define DRIVER_CPU_H_

#define CPU_FREQ (12288000UL / 2)

#define _EI __asm ei __endasm
#define _DI __asm di __endasm

struct cpu_context {
	uint16_t sp;
	uint16_t mmu;
	uint16_t layout;
	uint16_t iy;
	uint16_t ix;
	uint16_t hl;
	uint16_t de;
	uint16_t bc;
	uint16_t af;
	uint16_t pc;
};

#endif
