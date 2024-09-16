/* ZAK180 Firmaware
 * Kernel main
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#include <string.h>
#include <stdint.h>
#include <stdio.h>

#include "memory.h"
#include "timer.h"
#include "thread.h"

#include "../driver/uart.h"
#include "../driver/vga.h"

static struct {
	int8_t started;
	struct thread thread;
} common;

int putchar(int c)
{
	char t = c;

	if (common.started) {
		uart1_write(&t, 1, 1);
	}
	else {
		uart1_write_poll(&t, 1);
	}

	//vga_putchar(t);

	return 1;
}

void thread(void *arg)
{
	int i = (int)arg;

	while (1) {
		ktime_t now = timer_get();
		printf("Now: %llu\r\n", now);
		thread_sleep(now + 1000);
	}
}


int main(void)
{
	uart_init();
	vga_init();

	printf("ZAK180 Operating System rev " VERSION " " DATE "\r\n");

	/* Start: 64 KB reserved for the kernel
	 * End: VGA starts at @0xFE000 */
	memory_init(16, 238);
	timer_init();
	thread_init();

	thread_create(&common.thread, 4, thread, (void *)0);

	/* Enable interrupts and wait for reschedule */
	thread_start();
	common.started = 1;
	_EI;

	return 0;
}
