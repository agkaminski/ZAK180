/* ZAK180 Firmaware
 * Critical section
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#ifndef DRIVER_CRITICAL_H_
#define DRIVER_CRITICAL_H_

void critical_start(void);


void critical_end(void);


void critical_enable(void);

#endif
