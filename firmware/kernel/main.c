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

static struct {
	int8_t schedule;
	struct thread thread[2];
	struct thread *queue;
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

void waiter(void *arg)
{
	while (1) {
		thread_critical_start();
		_thread_wait(&common.queue, 0);
		thread_critical_end();
		printf("Wakeup %llu\r\n", timer_get() / 1000);
	}
}

void pinger(void *arg)
{
	while (1) {
		thread_sleep_relative(1000);
		thread_critical_start();
		_thread_signal(&common.queue);
		thread_critical_end();
	}
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

	thread_create(&common.thread[0], 4, waiter, (void *)0);
	thread_create(&common.thread[1], 4, pinger, (void *)0);

	/* Enable interrupts and wait for reschedule */
	thread_start();
	common.schedule = 1;
	critical_enable();
	_EI;

	return 0;
}
