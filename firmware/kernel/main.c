/* ZAK180 Firmaware
 * Kernel main
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#include <string.h>
#include <stdio.h>

#include "memory.h"

#include "../driver/uart.h"
#include "../driver/vga.h"

int putchar(int c)
{
	char t = c;

	uart1_write(&t, 1, 1);
	vga_putchar(t);

	return 1;
}

int main(void)
{
	/* Start: 64 KB reserved for the kernel
	 * End: VGA starts at @0xFE000 */
	memory_init(16, 238);

	uart_init();
	vga_init();

	printf("ZAK180 Operating System rev " VERSION " compiled on " DATE "\r\n");

	for (uint8_t i = 0; i < 254; ++i) {
		printf("Alloc %u pages, got page ", 2);
		uint8_t page = memory_alloc(MEMORY_OWNER_KERNEL, 2);
		printf("%u\r\n", page);
	}

	for (uint8_t i = 0; i < 254; ++i) {
		printf("Free %u pages at %u\r\n", 1, i);
		memory_free(i, 1);
	}

	for (uint8_t i = 0; i < 254; ++i) {
		printf("Alloc %u pages, got page ", 4);
		uint8_t page = memory_alloc(MEMORY_OWNER_KERNEL, 4);
		printf("%u\r\n", page);
	}

	while (1) {
		char c;
		uart1_read(&c, 1, 1);
		uart1_write(&c, 1, 1);
	}

	/* Give vblank some time */
	for (volatile uint16_t i = 1; i != 0; ++i);

	return 0;
}
