.PHONY: clean

all: ay38912.lib mmu.lib uart.lib vga.lib

ay38912.lib: ay38912.rel
	sdar -q "$@" "$<"

mmu.lib: mmu.rel
	sdar -q "$@" "$<"

uart.lib: uart.rel
	sdar -q "$@" "$<"

vga.lib: vga.rel
	sdar -q "$@" "$<"

%.rel: %.c
	sdcc -mz180 -c "$<"

clean:
	@rm -f *.lib *.lk *.map *.mem *.lst *.rel *.rst *.sym *.asm *.ihx *.noi *.hex
