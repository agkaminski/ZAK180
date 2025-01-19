/* ZAK180 Firmaware
 * Kernel API: fcntl.h
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#ifndef KERNEL_FCNTL_H_
#define KERNEL_FCNTL_H_

/* File type */
#define S_IFMT   0x1F
#define S_IFREG  0x10
#define S_IFBLK  0x08
#define S_IFDIR  0x04
#define S_IFCHR  0x02
#define S_IFIFO  0x01

#define S_ISREG(m)  (((m) & S_IFMT) == S_IFREG)
#define S_ISBLK(m)  (((m) & S_IFMT) == S_IFBLK)
#define S_ISDIR(m)  (((m) & S_IFMT) == S_IFDIR)
#define S_ISCHR(m)  (((m) & S_IFMT) == S_IFCHR)
#define S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)

/* Permissions */
#define S_IRWX 0xE0
#define S_IR   0x80
#define S_IW   0x40
#define S_IX   0x20

/* Access mode */
#define O_CREAT    0x01
#define O_TRUNC    0x02
#define O_RDONLY   0x04
#define O_WRONLY   0x08
#define O_RDWR     0x10
#define O_APPEND   0x20
#define O_NONBLOCK 0x40
#define O_ACCMODE  0x7F

#endif
