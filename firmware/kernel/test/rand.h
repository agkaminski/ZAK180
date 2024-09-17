/* ZAK180 Firmaware
 * Kernel unit tests - pseudo-random number generator
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#ifndef KERNEL_TEST_RAND_H_
#define KERNEL_TEST_RAND_H_

#include <stdint.h>

void test_srand(uint16_t s);

static uint16_t test_rand16(void);

#endif
