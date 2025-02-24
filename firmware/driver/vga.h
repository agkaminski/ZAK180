/* ZAK180 Firmaware
 * VGA driver
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#ifndef DRIVER_VGA_H_
#define DRIVER_VGA_H_

#include <stdint.h>

void _vga_late_irq(void);

void vga_putchar(char c);

void _vga_clear(void);

void vga_select_rom(uint8_t rom);

void vga_set_cursor(uint8_t enable);

void vga_init(void);

#endif
