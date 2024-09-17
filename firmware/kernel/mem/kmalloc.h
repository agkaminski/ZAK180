/* ZAK180 Firmaware
 * Kernel dynamic memory allocation
 * Adapted from github.com/agkaminski/ualloc
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#ifndef KERNEL_KMALLOC_H_
#define KERNEL_KMALLOC_H_

#include <stddef.h>

void *kmalloc(size_t size);

void *krealloc(void *ptr, size_t size);

void kfree(void *ptr);

void kmalloc_stat(size_t *used, size_t *free);

void kalloc_init(void *buff, size_t size);

#endif
