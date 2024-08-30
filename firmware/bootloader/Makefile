TARGET = bootloader

CPU = z180
CODE = 0xC000
CODEC = 0xC100
DATA = 0xF000
GHASH = "$(shell git describe --always --dirty)"
CFLAGS = -m${CPU} -DVERSION='${GHASH}' -DCODE_LOC=${CODE}
LDFLAGS = --code-loc ${CODEC} --data-loc ${DATA} --no-std-crt0

SRC = main.c
OBJ = crt0.rel $(SRC:.c=.rel)

.PHONY: clean

all: ${TARGET}.bin

${TARGET}.bin: ${TARGET}.hex
	objcopy -Iihex -Obinary ${TARGET}.hex ${TARGET}.bin

${TARGET}.hex: ${OBJ}
	sdcc -o ${TARGET}.hex ${CFLAGS} ${LDFLAGS} ${OBJ}

%.rel: %.c
	sdcc ${CFLAGS} -c "$<"

%.rel: %.s
	sdasz80 -l -o -s "$<"

clean:
	@rm -f *.bin *.lk *.map *.mem *.lst *.rel *.rst *.sym *.asm *.ihx *.noi
	@rm -f ${TARGET}.bin ${TARGET}.hex

