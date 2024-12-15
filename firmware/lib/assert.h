/* Assert
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#ifndef LIB_ASSERT_H_
#define LIB_ASSERT_H_

#ifndef NDEBUG
#include <stdio.h>
#include "lib/panic.h"

#define assert(__cond) \
	do { \
		if (!(__cond)) { \
			printf("Assertion '%s' failed in file %s:%d, function %s.\n", #__cond, __FILE__, __LINE__, __func__); \
			panic(); \
		} \
	} while (0)
#else
#define assert(__cond) ((void)0)
#endif

#endif
