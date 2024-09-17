/* ZAK180 Firmaware
 * Kernel dynamic memory allocation
 * Adapted from github.com/agkaminski/ualloc
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#include <stddef.h>
#include <string.h>

#include "proc/lock.h"

#include "kmalloc.h"

#define FLAG_MASK  (1UL)
#define FLAG(size) !!((size) & FLAG_MASK)
#define SIZE(size) ((size) & ~FLAG_MASK)
#define ANTIFRAG   8

#define ALIGN(size) ((size) + ANTIFRAG - 1) & ~(ANTIFRAG - 1)

typedef struct _header_t {
	size_t size;
	struct _header_t *next;
	unsigned char payload[];
} header_t;

static struct {
	header_t *heap;
	header_t *hint;
	struct lock lock;
} common;

void *kmalloc(size_t size)
{
	if (!size) {
		return NULL;
	}

	size = ALIGN(size);

	lock_lock(&common.lock);
	if (common.hint == NULL) {
		common.hint = common.heap;
	}

	for (header_t *curr = common.hint; curr != NULL; curr = curr->next) {
		if (!FLAG(curr->size) && SIZE(curr->size) >= size) {
			if (SIZE(curr->size) >= size + sizeof(header_t) + ANTIFRAG) {
				header_t *spawn = (void *)(curr->payload + size);
				spawn->next = curr->next;
				spawn->size = (SIZE(curr->size) - size - sizeof(header_t)) & ~FLAG_MASK;

				curr->size = size;
				curr->next = spawn;
			}

			curr->size |= FLAG_MASK;

			if (curr == common.hint) {
				common.hint = curr->next;
			}
			lock_unlock(&common.lock);

			return (void *)curr->payload;
		}
	}
	lock_unlock(&common.lock);

	return NULL;
}

void *krealloc(void *ptr, size_t size)
{
	if (!size) {
		kfree(ptr);
		return NULL;
	}

	size = ALIGN(size);

	if (ptr == NULL) {
		return kmalloc(size);
	}

	lock_lock(&common.lock);
	header_t *curr = (header_t *)((unsigned char *)ptr - sizeof(header_t));

	if (SIZE(curr->size) >= size) {
		if (SIZE(curr->size) > size + sizeof(header_t) + ANTIFRAG) {
			header_t *spawn = (void *)(curr->payload + size);
			spawn->next = curr->next;
			spawn->size = SIZE(curr->size) - size - sizeof(header_t);
			curr->size = size | FLAG_MASK;
			curr->next = spawn;

			if (spawn->next != NULL && !FLAG(spawn->next->size)) {
				spawn->size = SIZE(spawn->size) + SIZE(spawn->next->size) + sizeof(header_t);
				spawn->next = spawn->next->next;
			}

			if (spawn < common.hint) {
				common.hint = spawn;
			}
		}
		lock_unlock(&common.lock);

		return ptr;
	}

	size_t t = SIZE(curr->size) + SIZE(curr->next->size) + sizeof(header_t);
	if (curr->next != NULL && !FLAG(curr->next->size) && t >= size) {

		if (curr->next == common.hint) {
			common.hint = curr->next->next;
		}

		curr->size = t | FLAG_MASK;
		curr->next = curr->next->next;

		if (t > size + sizeof(header_t) + ANTIFRAG) {
			curr->size = size | FLAG_MASK;
			header_t *spawn = (void *)(curr->payload + size);
			spawn->next = curr->next;
			spawn->size = t - size - sizeof(header_t);

			if (curr->next == common.hint) {
				common.hint = spawn;
			}

			curr->next = spawn;
		}
		lock_unlock(&common.lock);

		return (void *)curr->payload;
	}
	lock_unlock(&common.lock);

	unsigned char *buff = kmalloc(size);
	if (buff != NULL) {
		memcpy(buff, ptr, SIZE(curr->size));
		kfree(ptr);
	}

	return buff;
}

void kfree(void *ptr)
{
	header_t *prev = NULL;

	lock_lock(&common.lock);
	for (header_t *curr = common.heap; curr != NULL; prev = curr, curr = curr->next) {
		if ((void *)curr->payload == ptr) {
			if (prev != NULL && !FLAG(prev->size)) {
				prev->size = SIZE(prev->size) + SIZE(curr->size) + sizeof(header_t);
				prev->next = curr->next;
				curr = prev;
			}

			if (curr->next != NULL && !FLAG(curr->next->size)) {
				curr->size = SIZE(curr->size) + SIZE(curr->next->size) + sizeof(header_t);
				curr->next = curr->next->next;
			}

			curr->size &= ~FLAG_MASK;

			if (curr < common.hint) {
				common.hint = curr;
			}

			break;
		}
	}
	lock_unlock(&common.lock);
}

void kmalloc_stat(size_t *used, size_t *free)
{
	size_t u = 0, f = 0;

	lock_lock(&common.lock);
	header_t *curr = common.heap;
	while (curr != NULL) {
		size_t sz = SIZE(curr->size);
		if (FLAG(curr->size)) {
			u += sz;
		}
		else {
			f += sz;
		}
		curr = curr->next;
	}
	lock_unlock(&common.lock);

	if (used != NULL) {
		*used = u;
	}
	if (free != NULL) {
		*free = f;
	}
}

void kalloc_init(void *buff, size_t size)
{
	common.heap = buff;
	common.heap->size = (size - sizeof(header_t)) & ~FLAG_MASK;
	common.heap->next = NULL;
	common.hint = common.heap;
	lock_init(&common.lock);
}
