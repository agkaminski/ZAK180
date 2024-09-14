/* ZAK180 Firmaware
 * Memory management
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#ifndef KERNEL_MEMORY_H_
#define KERNEL_MEMORY_H_

#include <stddef.h>
#include <stdint.h>

#define MEMORY_OWNER_CACHE  ((void *)-1)
#define MEMORY_OWNER_KERNEL NULL

#define MEMORY_PAGE_SIZE 4096

typedef void (*memory_page_release)(void *addr);

/* Allocates one page of cache memory, "release_callback" is
 * called when the kernel need to reaquire the memory to
 * prepare owner to free the page */
uint8_t memory_cache_alloc(memory_page_release release_callback);

/* Returns continuous memory starting at return value of "pages" length */
/* TODO owner is a process placeholder */
uint8_t memory_alloc(void *owner, uint8_t pages);

/* Frees the memory starting at "page" of size previously allocated */
void memory_free(uint8_t page, uint8_t pages);

#endif
