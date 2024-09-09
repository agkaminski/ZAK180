.PHONY: clean

all: fat12.lib

fat12.lib: fat12.rel
	@sdar -q "$@" $^

%.rel: %.c
	sdcc -mz180 --opt-code-size --max-allocs-per-node 10000 -c "$<"

clean:
	@rm -f *.lib *.lk *.map *.mem *.lst *.rel *.rst *.sym *.asm *.ihx *.noi *.hex
