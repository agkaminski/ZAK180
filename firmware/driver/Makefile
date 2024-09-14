.PHONY: clean

all: ay38912.lib mmu.lib uart.lib vga.lib floppy.lib pio.lib prt.lib

ay38912.lib: ay38912.rel
	@sdar -q "$@" $^

mmu.lib: mmu.rel
	@sdar -q "$@" $^

uart.lib: uart.rel
	@sdar -q "$@" $^

vga.lib: vga.rel
	@sdar -q "$@" $^

floppy.lib: floppy_cmd.rel floppy.rel
	sdar -q "$@" $^

pio.lib: pio.rel
	@sdar -q "$@" $^

prt.lib: prt.rel
	@sdar -q "$@" $^

%.rel: %.c
	sdcc -mz180 --opt-code-size --max-allocs-per-node 10000 -c "$<"

clean:
	@rm -f *.lib *.lk *.map *.mem *.lst *.rel *.rst *.sym *.asm *.ihx *.noi *.hex
