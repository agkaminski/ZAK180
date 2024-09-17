/* ZAK180 Firmaware
 * Kernel unit tests - pseudo-random number generator
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#include <stdint.h>

static uint16_t seed;

void test_srand(uint16_t s)
{
	seed = s;
}

uint16_t test_rand16(void)
{
	uint32_t next = (uint32_t)(seed) * 1103515243 + 12345;
	next >>= 16;
	seed ^= (uint16_t)next;
	return next;
}
