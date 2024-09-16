/* ZAK180 Firmaware
 * Memory management
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#ifndef KERNEL_PAGE_H_
#define KERNEL_PAGE_H_

#include <stddef.h>
#include <stdint.h>

#define PAGE_OWNER_CACHE  ((void *)-1)
#define PAGE_OWNER_KERNEL NULL

#define PAGE_SIZE 4096

typedef void (*page_release)(void *addr);

/* Allocates one page of cache memory, "release_callback" is
 * called when the kernel need to reaquire the memory to
 * prepare owner to free the page */
uint8_t page_cache_alloc(page_release release_callback);

/* Returns continuous memory starting at return value of "pages" length */
/* TODO owner is a process placeholder */
uint8_t page_alloc(void *owner, uint8_t pages);

/* Frees the memory starting at "page" of size previously allocated */
void page_free(uint8_t page, uint8_t pages);

void page_init(uint8_t start, uint8_t pages);

#endif
