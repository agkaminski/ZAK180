# ZAK180 computer

<img src="img/top.png">

<img src="img/bottom.png">

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

## Firmare

ZAKOS code repository is located [here](https://github.com/agkaminski/ZAKOS).

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
| 0x40  | 0x5F | VBLANK IRQ acknowledge  |
| 0x60  | 0x7F | ROM disable control     |
| 0x80  | 0x9F | Keyboard                |
| 0xA0  | 0xBF | Z80 PIO (user port)     |
| 0xC0  | 0xDF | AY-3-8912               |
| 0xE0  | 0xFF | 82077 floppy controller |

## Repository structure

### datasheet

Collection of useful datasheets, might be removed in the future.

### keyboard2

Subrepository that contains a hardware project of a mechanical keyboard for the
computer.

### kicad

KiCAD project of the computer's PCB.

### vga

HDL of the VGA timing generator implemented in a XC9536 CPLD, tool to generate
font ROM.

### schematic.pdf

Schematic of the computer in a friendly .pdf form.

### 3dprint

3D models designed for FDM priting.

## License

[Attribution-NonCommercial 4.0](LICENSE)
