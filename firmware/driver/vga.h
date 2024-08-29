/* ZAK180 Firmaware
 * VGA driver
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#ifndef DRIVER_VGA_H_
#define DRIVER_VGA_H_

#include <stdint.h>

void vga_putc(char c);

void vga_puts(const char *str);

void vga_clear(void);

void vga_handleCursor(void);

void vga_selectRom(uint8_t rom);

void vga_init(void);

#endif
