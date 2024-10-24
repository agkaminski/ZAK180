.PHONY: clean

OBJ := ay38912.rel critical.rel floppy_cmd.rel floppy.rel \
  keyboard.rel mmu.rel pio.rel prt.rel uart.rel vga.rel

all: driver.lib

driver.lib: ${OBJ}
	@sdar -q "$@" $^

%.rel: %.c
	sdcc -mz180 --opt-code-size --max-allocs-per-node 10000 -c "$<"

clean:
	@rm -f *.lib *.lk *.map *.mem *.lst *.rel *.rst *.sym *.asm *.ihx *.noi *.hex
