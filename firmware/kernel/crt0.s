; ZAK180 Firmaware
; Kernel crt0.s
; Copyright: Aleksander Kaminski, 2024
; See LICENSE.md

.module ctr0

.globl _main
.globl _vga_vblank_handler
.globl _uart0_irq_handler
.globl _uart1_irq_handler
.globl _timer_irq_handler

.z180

CBR =    0x0038   ; common 1 base register
BBR =    0x0039   ; bank base register
CBAR =   0x003A   ; logical memory layout, bank on low nibble

DCNTL =  0x0032   ; DMA/WAIT control register
RCR =    0x0036   ; refresh control register

ROMDIS = 0x0060   ; ROM disable/enable control register
INT2_ACK = 0x0040 ; VBLANK INT2 acknownlage

TCR = 0x0010      ; PRT control
TMDR0L = 0x000C   ; Timer Data Register Channel 0L
TMDR0H = 0x000D   ; Timer Data Register Channel 0H

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

			call _main

			; We shouldn't get here
			di
			halt

.org 0x0100
ivt:
.word _irq_bad    ; INT1, floppy IRQ not supported
.word _irq_vblank ; INT2, VBLANK
.word _irq_prt0   ; PRT CH0, systick
.word _irq_bad    ; PRT CH1, not supported
.word _irq_bad    ; DMA CH0, not supported
.word _irq_bad    ; DMA CH1, not supported
.word _irq_bad    ; CSI/O, not supported
.word _irq_uart0  ; ASCI 0
.word _irq_uart1  ; ASCI 1

.macro SAVE
			push af
			push bc
			push de
			push hl
			push ix
			push iy

			ld hl, #0
			add hl, sp
			in0 a, (#CBAR)
			ld c, a
			ld b, #0
			push bc
			in0 a, (#BBR)
			ld c, a
			in0 a, (#CBR)
			ld b, a
			push bc
			push hl

			; Pass a pointer to the context in hl
			ld bc, #6
			add hl, bc
.endm

.macro RESTORE
			pop hl
			pop bc
			pop de
			ld a, b
			out0 (#CBR), a
			ld a, c
			out0 (#BBR), a
			pop bc
			ld a, e
			out0 (#CBAR), a
			ld sp, hl

			pop iy
			pop ix
			pop hl
			pop de
			pop bc
			pop af
.endm

_irq_bad:
			halt

_irq_vblank:
			SAVE
			out0 (#INT2_ACK), a
			call _vga_vblank_handler
			RESTORE
			ei
			reti

_irq_prt0:
			SAVE
			; acknownlage irq
			in0 a, (#TCR)
			in0 a, (#TMDR0L)
			in0 a, (#TMDR0H)
			call _timer_irq_handler
			RESTORE
			ei
			reti

_irq_uart0:
			SAVE
			call _uart0_irq_handler
			RESTORE
			ei
			reti

_irq_uart1:
			SAVE
			call _uart1_irq_handler
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
