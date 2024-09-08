TARGET = kernel

CPU = z180
CODE = 0x0200
DATA = 0x8000
GHASH = "$(shell git describe --always --dirty)"
DATE = "$(shell date +"%X %x")"
CFLAGS = -m${CPU} -DVERSION='${GHASH}' -DDATE='${DATE}' -L../driver -L../filesystem -I. -I../driver
LDFLAGS = --code-loc ${CODE} --data-loc ${DATA} --no-std-crt0

SRC = main.c
OBJ = crt0.rel $(SRC:.c=.rel)
DRIVERS = uart.lib floppy.lib ay38912.lib mmu.lib vga.lib
FILESYSTEMS = fat12.lib
LIB = ${DRIVERS} ${FILESYSTEMS}

.PHONY: clean

all: ${TARGET}.bin

${TARGET}.bin: ${TARGET}.hex
	@objcopy -Iihex -Obinary ${TARGET}.hex ${TARGET}.bin
	du -b ${TARGET}.bin

${TARGET}.hex: ${OBJ}
	@sdcc -o ${TARGET}.hex -l ${LIB} ${CFLAGS} ${LDFLAGS} ${OBJ}

%.rel: %.c
	@sdcc ${CFLAGS} -c "$<"

%.rel: %.s
	@sdasz80 -l -o -s "$<"

clean:
	@rm -f *.bin *.lk *.map *.mem *.lst *.rel *.rst *.sym *.asm *.ihx *.noi *.hex

