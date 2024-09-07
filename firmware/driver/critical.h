/* ZAK180 Firmaware
 * Critical section
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#ifndef DRIVER_CRITICAL_H_
#define DRIVER_CRITICAL_H_

#define _CRITICAL_START __asm di __endasm
#define _CRITICAL_END   __asm ei __endasm

#endif
