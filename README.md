# ZAK180 computer

<img src="img/computer.png">

Work in progress.

## Hardware features

- Z180 CPU,
- 1 MB of static RAM,
- 16 KB bootloader ROM,
- Floppy disk drive,
- 16 bit GPIO user port,
- 2 UARTs: USB-C and RS232,
- AY-3-8912 sound,
- 80x60 B&W text mode VGA output,
- custom Cherry-MX mechanical keyboard,
- USB-C powered.

## Software features

ROM bootloader functions to load the kernel image from the 1.44 MB 3.5" FDD.
The kernel will a multi-tasking, preeptive in a monolithic architecture, with
UN\*X-like features (not even close to the full POSIX compability). Hopefully,
it will be possible to port some UN\*X applications (like vi), although limited
user process address space may be an obstacle (56 KB + 4 KB of stack).

## Memory map

| Start   | End     | Description                              |
|---------|---------|------------------------------------------|
| 0x00000 | 0x03FFF | ROM, disabled after boot, after that RAM |
| 0x04000 | 0xFDFFF | RAM                                      |
| 0xFE000 | 0xFFFFF | Memory mapped I/O, VGA VRAM              |

## I/O map

8-bit I/O is used.

| Start | End  | Description             |
|-------|------|-------------------------|
| 0x00  | 0x3F | Z180 internal I/O       |
| 0x40  | 0x5F | VBLANK IRQ acknowlage   |
| 0x60  | 0x7F | ROM disable control     |
| 0x80  | 0x9F | Keyboard                |
| 0xA0  | 0xBF | Z80 PIO (user port)     |
| 0xC0  | 0xDF | AY-3-8912               |
| 0xE0  | 0xFF | 82077 floppy controller |

## MMU memory layouts

### Bootloader

| Region   | Logical start | Logical end | Physical start | Physical end | Description        |
|----------|---------------|-------------|----------------|--------------|--------------------|
| Common 0 | 0x0000        | 0x5FFF      | 0x00000        | 0x05FFF      | Kernel entry point |
| Bank     | 0x6000        | 0x7FFF      | N/A            | N/A          | Memory access      |
| Common 1 | 0x8000        | 0xFFFF      | 0xE8000        | 0xEFFFF      | Code and data      |

### Kernel

| Region   | Logical start | Logical end | Physical start | Physical end | Description        |
|----------|---------------|-------------|----------------|--------------|--------------------|
| Common 0 | 0x0000        | 0xDFFF      | 0x00000        | 0x0DFFF      | Kernel code/data   |
| Bank     | 0xE000        | 0xEFFF      | N/A            | N/A          | Memory access      |
| Common 1 | 0xF000        | 0xFFFF      | N/A            | N/A          | Switchable stack   |

### User

| Region   | Logical start | Logical end | Physical start | Physical end | Description        |
|----------|---------------|-------------|----------------|--------------|--------------------|
| Common 0 | 0x0000        | 0x0FFF      | 0x00000        | 0x00FFF      | Kernel entry       |
| Bank     | 0x1000        | 0xEFFF      | N/A            | N/A          | User code/data     |
| Common 1 | 0xF000        | 0xFFFF      | N/A            | N/A          | Switchable stack   |

## Repository structure

### datasheet

Collection of useful datasheets, might be removed in the future.

### firmware

#### bootloader

Source code of a simple EPROM bootloader. It prepares the platform, initializes
FDD and fetches the kernel image from the `/BOOT/KERNEL.IMG` file. Kernel is
loaded to the start of the physical address space (0x00000 onward, variable
size). Entry point of the kernel is assumed to be at the reset vector (i.e.
address 0x0000).

#### driver

ZAK180 peripheral drivers, both internal of Z180 MPU and present on the SBC.
This includes:

- Z180 ASCI,
- Z180 MMU,
- AY-3-8912,
- 82077,
- Z80 PIO,
- VGA.

#### filesystem

FAT12 read/write driver. Many characterics of the 1.44 MB floppy FAT12 variant
are hardcoded, with no current plans of generalization. LRU sector cache is
planned.

#### kernel

Source code of the kernel of the computer operating system.

#### test

Simple dead or alive SBC test. All it does is writing to the ASCI1 (USB-C) 
at 19200 bn1.

#### build.sh

Script to build all components of the firmware.

### keyboard2

Subrepository that contains a hardware project of a mechanical keyboard for the
computer.

### kicad

KiCAD project of the computer's PCB.

### vga

HDL of the VGA timing generator implemented in a XC9535 CPLD, tool to generate
font ROM.

### schematic.pdf

Schematic of the computer in a friendly .pdf form.

### 3dprint

3D models designed for FDM priting.

## Rev A

<img src="img/pcb.png">

## License

[CC0 1.0 Universal](LICENSE)
