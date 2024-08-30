TARGET = bootloader

CPU = z180
CODE = 0xC000
DATA = 0xF000
GHASH = "$(shell git describe --always --dirty)"
CFLAGS = -m${CPU}
LDFLAGS = --code-loc ${CODE} --data-loc ${DATA} --no-std-crt0

SRC = main.c
OBJ = $(SRC:.c=.rel)

.PHONY: clean

all: ${TARGET}.bin

${TARGET}.bin: ${TARGET}.hex
	objcopy -Iihex -Obinary ${TARGET}.hex ${TARGET}.bin

${TARGET}.hex: ${OBJ}
	sdcc -o ${TARGET}.hex ${CFLAGS} ${LDFLAGS} -DVERSION='${GHASH}' ${OBJ}

%.rel: %.c
	sdcc ${CFLAGS} -c "$<"

clean:
	@rm -f *.bin *.lk *.map *.mem *.lst *.rel *.rst *.sym *.asm *.ihx *.noi
	@rm -f ${TARGET}.bin ${TARGET}.hex

