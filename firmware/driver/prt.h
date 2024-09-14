/* ZAK180 Firmaware
 * Programmable Reload Timer driver
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#ifndef DRIVER_PRT_H_
#define DRIVER_PRT_H_

#include <stdint.h>

uint16_t _prt0_timer_get(void);

uint16_t _prt1_timer_get(void);

uint16_t prt0_timer_get(void);

uint16_t prt1_timer_get(void);

uint32_t prt_val_to_ms(uint16_t val);

/* Interval in miliseconds */

void prt0_init(uint8_t interval);

void prt0_init(uint8_t interval);

#endif
