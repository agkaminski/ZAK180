#include <string.h>
#include <stdio.h>

#include "../driver/uart.h"

void uart_puts(const char *str)
{
	uart1_write_poll(str, strlen(str));
}

int main(void)
{
	volatile unsigned char *ptr = (void *)0;
	int i;
	char buff[32];

	uart_init();

	for (i = 0; i < 0x2000; ++i) {
		sprintf(buff, "Checking addr 0x%04x\r\n", i);
		uart_puts(buff);

		ptr[i] = 0;
		if (ptr[i] != 0) {
			break;
		}
		ptr[i] = 0x55;
		if (ptr[i] != 0x55) {
			break;
		}
		ptr[i] = 0xaa;
		if (ptr[i] != 0xaa) {
			break;
		}
		ptr[i] = 0xff;
		if (ptr[i] != 0xff) {
			break;
		}
	}

	if (i != 0x2000) {
		uart_puts("FAIL\r\n");
	}
	else {
		uart_puts("SUCCESS\r\n");
	}

	return 0;
}
