; ZAK180 Firmaware
; Kernel crt0.s
; Copyright: Aleksander Kaminski, 2024
; See LICENSE.md

.module ctr0

.globl _main
.globl _vga_vblank_handler

.z180

CBR =    0x0038   ; common 1 base register
BBR =    0x0039   ; bank base register
CBAR =   0x003A   ; logical memory layout, bank on low nibble

DCNTL =  0x0032   ; DMA/WAIT control register
RCR =    0x0036   ; refresh control register

ROMDIS = 0x0060   ; ROM disable/enable control register
INT2_ACK = 0x0040 ; VBLANK INT2 acknownlage

ITC = 0x0034      ; INT/TRAP Control Register

; Start code has to be position independent!
;
; We're starting with the bootloader layout:
; Common 0:
; 24 KB, 0x0000 -> 0x5FFF (0x00000 -> 0x05FFF)
; Bank:
; 8 KB, 0x6000 -> 0x8000 (mapped as needed)
; Common 1:
; 32 KB, 0x8000 -> 0xFFFF (0xE8000 -> 0xEFFFF)
;
; Desired memory layout (logical):
; Common 0: kernel code and data
; 56 KB, 0x0000 -> 0xDFFF (0x00000 -> 0x0DFFF)
; Bank: memory access window
; 4 KB, 0xE000 -> 0xEFFF (mapped as needed)
; Common 1: Switchable stack area
; 4 KB, 0xF000 -> 0xFFFF (0xEF000 -> 0xEFFFF)

.area _HEADER (ABS)
.org 0x0000

reset:
			; Disable RAM wait-states
			ld a, #0x00
			out0 (#DCNTL), a

			; Disable refresh (no DRAM on the system)
			ld a, #0x00
			out0 (#RCR), a

			; Setup logical layout
			ld a, #0xFE
			out0 (#CBAR), a

			; Setup stack page
			ld a, #0xEF - 0x0F
			out0 (#CBR), a

			; Setup stack
			ld sp, #0x0000

			; Init .bss and .data
			call bss_init
			call data_init

			; Setup IVT
			ld a, #0x01 ; 0x0100 >> 8
			ld i, a

			; Enable INT2 (VBLANK) only
			ld a, #0x04
			out0 (#ITC), a

			; That's enough to jump to C, we can finish
			; init there

			ei
			call _main

			; We shouldn't get here
			di
			halt

.org 0x0100
ivt:
.word _irq_bad    ; INT1, floppy IRQ not supported
.word _irq_vblank ; INT2, VBLANK
.word _irq_bad    ; PRT CH0, not supported
.word _irq_bad    ; PRT CH1, not supported
.word _irq_bad    ; DMA CH0, not supported
.word _irq_bad    ; DMA CH1, not supported
.word _irq_bad    ; CSI/O, not supported
.word _irq_uart0  ; ASCI 0
.word _irq_uart1  ; ASCI 1

.macro SAVE
			ex af, af'
			exx
			push ix
			push iy
.endm

.macro RESTORE
			pop iy
			pop ix
			exx
			ex af, af'
.endm

_irq_bad:
			halt

_irq_vblank:
			SAVE
			out0 (INT2_ACK), a
			call _vga_vblank_handler
			RESTORE
			ei
			reti

_irq_uart0:
			SAVE
			; TODO
			RESTORE
			ei
			reti

_irq_uart1:
			SAVE
			; TODO
			RESTORE
			ei
			reti

.area _HOME
.area _CODE
.area _INITIALIZER
.area _GSINIT
.area _GSFINAL

.area _DATA
.area _INITIALIZED
.area _BSEG
.area _BSS
.area _HEAP

.area _GSINIT

.globl l__DATA
.globl s__DATA
.globl l__INITIALIZER
.globl s__INITIALIZER
.globl s__INITIALIZED

bss_init:
			ld bc, #l__DATA ; Length to zero-out
			ld a, b         ; Check if zero
			or a, c
			ret z

			ld hl, #s__DATA ; Start of the .bss
			ld (hl), #0     ; Zero-out first byte
			dec bc          ; One byte already done
			ld a, b         ; Check if zero
			or a, c
			ret z

			ld d, h
			ld e, l
			inc de
			ldir            ; Copy first zeroed byte

			ret

data_init:
			ld bc, #l__INITIALIZER
			ld a, b         ; Check if zero
			or a, c
			ret z

			ld	de, #s__INITIALIZED
			ld	hl, #s__INITIALIZER
			ldir

			ret
