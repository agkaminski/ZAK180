/* ZAK180 Firmaware
 * Kernel main
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#include <string.h>
#include <stdint.h>
#include <stdio.h>

#include "mem/page.h"
#include "proc/timer.h"
#include "proc/thread.h"

#include "driver/uart.h"
#include "driver/vga.h"
#include "driver/critical.h"

#include "test/test.h"

static struct {
	int8_t schedule;
} common;

int putchar(int c)
{
	char t = c;

	if (common.schedule) {
		uart1_write(&t, 1, 1);
	}
	else {
		uart1_write_poll(&t, 1);
	}

	vga_putchar(t);

	return 1;
}

int main(void)
{
	uart_init();
	vga_init();

	printf("ZAK180 Operating System rev " VERSION " " DATE "\r\n");

	/* Start: 64 KB reserved for the kernel
	 * End: VGA starts at @0xFE000 */
	page_init(16, 238);
	timer_init();
	thread_init();

	/* Enable interrupts and wait for reschedule */
	thread_start();
	common.schedule = 1;
	critical_enable();
	_EI;

	return 0;
}
