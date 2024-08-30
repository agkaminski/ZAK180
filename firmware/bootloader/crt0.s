.module ctr0
.globl _main
.z180

CBR =    0x38 ; common 1 base register
BBR =    0x39 ; bank base register
CBAR =   0x3A ; logical memory layout, bank on low nibble

ROMDIS = 0x60 ; ROM disable/enable control register

;; This code has to be position independent!
;; We'll be jumping a lot around the place
;; No interrupts in the bootloader, ignore vectors

;; We're starting with default Z80 compatible
;; memory layout (all in common 0):
;; 0x0000-0x3FFF ROM
;; 0x4000-0xFFFF RAM
;;
;; We need to copy ourself to the high RAM
;; and jump into it to save space for the kernel.
;;
;; Desired memory layout (logical):
;; Common 0: not used, leave as kernel entry point
;; 8 KB, 0x0000 -> 0x1FFF (0x00000 -> 0x01FFF)
;; Bank:     Bootloader code and data (high RAM)
;; 24 KB, 0x2000 -> 0x7FFF (0xF2000 -> 0xF7FFF)
;; Common 1: memory access window
;; 32 KB, 0x8000 -> 0xFFFF (mapped as needed)

			; We're actually @0x0000, but we need to trick
			; the linker to think otherwise to keep binary
			; in one place.
			; .org CODE_LOC

			; Setup logical layout
			ld a, #0x86
			out0 (#CBAR), a

			; Select high RAM for bank
			ld a, #0xF0
			out0 (#CBR), a

			; Select low RAM for common1
			ld a, #0
			out0 (#BBR), a

			; Now we have access to the desired bootloader
			; location in bank and window to the ROM in
			; common1. We need to copy ROM to the high RAM
			; and disable ROM /CS.

			ld hl, #0x2000    ; Destination
			ld de, #0x8000    ; Source
			ld bc, #16 * 1024 ; Count, ROM size
			ldir              ; Copy memory

			; Now jump into copied ROM to the high RAM
			; We're compiled onto destination addresses,
			; so simple absolute JMP will do the trick.

			jp high_ram
high_ram:
			; Now we're in the high RAM, we don't need
			; PIC code anymore. Do a normal init.

			; Disable ROM /CS line, allow to use low RAM.
			ld a, #0
			out0 (ROMDIS), a

			; TODO init

			call _main

			; We shouldn't get here
			halt
