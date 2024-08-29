CPU Z80

CNTLA equ 0x01
CNTLB equ 0x03
STAT  equ 0x05
TDR   equ 0x07
RDR   equ 0x09

* = 0x0000

		ld sp, 0x0000
		
		ld a, 0x64
		ld bc, CNTLA
		out (c), a
		
		ld a, 0x01
		ld bc, CNTLB
		out (c), a
		
loop	ld hl, string
print	ld bc, STAT
		in a, (c)
		and 0x02
		jr z, print

		ld a, (hl)
		cp 0
		jr z, loop
		inc hl
		
		ld bc, TDR
		out (c), a
		jr print

string	asc "ZAK180, I am alive!\r\n\0"
