#include <string.h>
#include <stdio.h>

#include "../driver/uart.h"

int putchar(int c)
{
	char t = c;

	uart1_write_poll(&t, 1);

	return 1;
}

int data = 5;
int bss;

int main(void)
{
	uart_init();

	printf("data = %d, 0x%p\r\n", data, &data);
	printf("bss = %d, 0x%p\r\n", bss, &bss);

	return 0;
}
