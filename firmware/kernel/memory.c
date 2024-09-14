/* ZAK180 Firmaware
 * Memory management
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#include <stdint.h>

#include "memory.h"

struct memory_element {
	struct memory_element *next;
	void *owner;
	memory_page_release callback;
	uint8_t start;
	uint8_t length;
};

static struct {
	struct memory_element element_pool[32];
	uint32_t used;
	struct memory_element *free;
	struct memory_element *alloc;
	struct memory_element *cache;
} common;

static struct memory_element *memory_element_alloc(void)
{
	struct memory_element *element = NULL;

	for (uint8_t i = 0; i < sizeof(common.element_pool) / sizeof(*common.element_pool); ++i) {
		if (!(common.used & (1 << i))) {
			element = &common.element_pool[i];
			common.used |= (1 << i);
			break;
		}
	}

	return element;
}

static void memory_element_free(struct memory_element *element)
{
	for (uint8_t i = 0; i < sizeof(common.element_pool) / sizeof(*common.element_pool); ++i) {
		if (element == &common.element_pool[i]) {
			common.used &= ~(1 << i);
			break;
		}
	}
}

static int memory_element_merge(struct memory_element *dest, struct memory_element *victim)
{
	if (dest == NULL || victim == NULL) {
		return -1;
	}

	if ((dest->start + dest->length == victim->start) && (dest->owner == victim->owner)) {
		dest->length += victim->length;
		dest->next = victim->next;
		memory_element_free(victim);
		return 0;
	}

	return -1;
}

static void memory_element_attach(struct memory_element **list, struct memory_element *element)
{
	struct memory_element *it = *list;

	while (it != NULL && element->start < it->start) {
		it = it->next;
	}

	if (it != NULL) {
		element->next = it->next;
		it->next = element;
	}
	else {
		element->next = *list;
		*list = element;
	}

	if (memory_element_merge(it, element) == 0) {
		memory_element_merge(it, it->next);
	}
}

static void memory_element_detach(struct memory_element **list, struct memory_element *element)
{
	struct memory_element *it = *list, *prev = NULL;

	while (it != NULL && it != element) {
		prev = it;
		it = it->next;
	}

	if (it != NULL) {
		if (prev != NULL) {
			prev->next = it->next;
		}
		else {
			*list = it->next;
		}
	}
}

static struct memory_element *memory_element_split(struct memory_element *victim, uint8_t pages)
{
	struct memory_element *newborn = memory_element_alloc();
	if (newborn == NULL) {
		return NULL;
	}

	if (victim->length <= pages) {
		return NULL;
	}

	newborn->start = victim->start;
	newborn->length = pages;

	victim->start += pages;
	victim->length -= pages;

	return newborn;
}

#include <stdio.h>
static void memory_dump(struct memory_element *list)
{
	struct memory_element *it = list;

	while (it != NULL) {
		printf("[ %u, %u, %p ]->", it->start, it->length, it->owner);
		it = it->next;
	}
	printf("\r\n");
}

static uint8_t memory_alloc_callback(void *owner, uint8_t pages, memory_page_release callback)
{
	struct memory_element *curr = common.free, *prev;
	uint8_t page = 0;

	while (curr != NULL && curr->length < pages) {
		prev = curr;
		curr = curr->next;
	}

	if (curr != NULL) {
		if (curr->length == pages) {
			memory_element_detach(&common.free, curr);
		}
		else {
			curr = memory_element_split(curr, pages);
		}
		curr->callback = callback;
		curr->owner = owner;
		page = curr->start;
		memory_element_attach(&common.alloc, curr);
	}

	printf("alloc:\r\n");
	memory_dump(common.alloc);
	printf("free:\r\n");
	memory_dump(common.free);

	return page;
}

uint8_t memory_alloc(void *owner, uint8_t pages)
{
	return memory_alloc_callback(owner, pages, NULL);
}

void memory_free(uint8_t page, uint8_t pages)
{
	struct memory_element *curr = common.alloc;

	while (pages) {
		while (curr != NULL) {
			if (curr->start <= page && (curr->start + curr->length) > page) {
				break;
			}

			curr = curr->next;
		}

		if (curr != NULL) {
			if (curr->start == page) {
				if (curr->length != pages) {
					curr = memory_element_split(curr, pages);
				}
				else {
					memory_element_detach(&common.alloc, curr);
				}
			}
			else {
				page = curr->start;
				struct memory_element *t = memory_element_split(curr, page - curr->start);

				/* Need to detach curr to avoid merge in attach */
				memory_element_detach(&common.alloc, curr);
				memory_element_attach(&common.alloc, t);
				if (curr->length != pages) {
					memory_element_attach(&common.free, curr);
					t = memory_element_split(curr, curr->length - pages);
					memory_element_attach(&common.alloc, curr);
					curr = t;
				}
			}

			page += curr->length;
			pages -= curr->length;

			memory_element_attach(&common.free, curr);
		}
		else {
			break;
		}
	}

	printf("alloc:\r\n");
	memory_dump(common.alloc);
	printf("free:\r\n");
	memory_dump(common.free);

}

uint8_t memory_cache_alloc(memory_page_release release_callback)
{
	return memory_alloc_callback(MEMORY_OWNER_CACHE, 1, release_callback);
}

void memory_init(uint8_t start, uint8_t pages)
{
	/* No error check, this is certain to be ok. */
	struct memory_element *element = memory_element_alloc();

	element->start = start;
	element->length = pages;
	element->owner = NULL;

	memory_element_attach(&common.free, element);
}
