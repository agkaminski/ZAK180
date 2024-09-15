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
	struct thread thread[4];
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

static uint16_t stack(void) __naked
{
	__asm
		ld hl, #0
		add hl, sp
		ex de, hl
		ret
	__endasm;
}

static void thread(void *arg)
{
	uint8_t threadno = (uint8_t)arg;

	for (volatile uint8_t i = 0; i < threadno; ++i) {
		for (volatile uint16_t i = 5000; i != 0; ++i);
	}

	while (1) {
		for (volatile uint16_t i = 5000; i != 0; ++i);
		printf("Thread %u alive sp: %p\r\n", threadno, stack());
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

	thread_create(&common.thread[0], 4, thread, (void *)1);
	thread_create(&common.thread[1], 4, thread, (void *)2);
	thread_create(&common.thread[2], 4, thread, (void *)3);
	thread_create(&common.thread[3], 4, thread, (void *)4);

	/* Enable interrupts and wait for reschedule */
	thread_start();
	common.started = 1;
	_EI;

	return 0;
}
