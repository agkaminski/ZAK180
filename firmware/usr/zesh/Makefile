TARGET = zesh

CPU = z180
CODE = 0x1020
DATA = 0x8000
CFLAGS = -m${CPU} --opt-code-size --max-allocs-per-node 10000 \
  -I ../zlibc/include
LDFLAGS = --code-loc ${CODE} --data-loc ${DATA} --no-std-crt0

SRC = main.c shell.c
OBJ = ../zlibc/crt0.rel $(SRC:.c=.rel)
LIB = "../zlibc/zlibc.lib"
TRASH = *.bin *.lk *.map *.mem *.lst *.rel *.rst *.sym *.asm *.ihx *.noi *.hex

.PHONY: clean

all: ${TARGET}.bin

${TARGET}.bin: ${TARGET}.hex
	@objcopy -Iihex -Obinary ${TARGET}.hex ${TARGET}.bin
	du -b ${TARGET}.bin

${TARGET}.hex: ${OBJ}
	sdcc -o ${TARGET}.hex -l ${LIB} ${CFLAGS} ${LDFLAGS} ${OBJ}

%.rel: %.c
	sdcc ${CFLAGS} -o "$@" -c "$<"

%.rel: %.s
	sdasz80 -l -o -s "$<"

clean:
	@rm -f ${TRASH}
