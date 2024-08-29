#ifndef DRIVER_ZAK180_H_
#define DRIVER_ZAK180_H_

#include "z180.h"

#define VBLANKACK   0x40 /* VBLANK IRQ (INT2) acknowlage */
#define ROMDIS      0x60 /* ROM enable/disable */
#define KEYB        0x80 /* Keyboard (16 registers) */

/* PIO */
#define PIO_PORTA   0xA0 /* PIO port A */
#define PIO_PORTB   0xA1 /* PIO port B */
#define PIO_CTRLA   0xA2 /* PIO command A */
#define PIO_CTRLB   0xA3 /* PIO command B */

/* AY-3-8912 */
#define AY_LATCH    0xC0 /* AY-3-8912 latch address, WO*/
#define AY_READ     0xC0 /* AY-3-8912 read, RO */
#define AY_WRITE    0xC1 /* AY-3-8912 write, WO */

/* Floppy 82077 */
#define FLOPPY_SRA  0xE0 /* Status Register A */
#define FLOPPY_SRB  0xE1 /* Status Register B */
#define FLOPPY_DOR  0xE2 /* Digital Output Register */
#define FLOPPY_TDR  0xE3 /* Tape Drive Register */
#define FLOPPY_MSR  0xE4 /* Main Status Register */
#define FLOPPY_DSR  0xE4 /* Data Rate Select Register */
#define FLOPPY_FIFO 0xE5 /* Data Register (FIFO)*/
#define FLOPPY_DIR  0xE7 /* Digital Input Register */
#define FLOPPY_CCR  0xE7 /* Configuration Control Register */

#endif
