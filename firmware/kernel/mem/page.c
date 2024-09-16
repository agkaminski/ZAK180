/* ZAK180 Firmaware
 * Memory management
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#include <stdint.h>

#include "page.h"
#include "../proc/lock.h"

struct page_element {
	struct page_element *next;
	void *owner;
	page_release callback;
	uint8_t start;
	uint8_t length;
};

static struct {
	struct page_element element_pool[32];
	uint32_t used;
	struct page_element *free;
	struct page_element *alloc;
	struct page_element *cache;
	struct lock lock;
} common;

static struct page_element *page_element_alloc(void)
{
	struct page_element *element = NULL;

	for (uint8_t i = 0; i < sizeof(common.element_pool) / sizeof(*common.element_pool); ++i) {
		if (!(common.used & (1 << i))) {
			element = &common.element_pool[i];
			common.used |= (1 << i);
			break;
		}
	}

	return element;
}

static void page_element_free(struct page_element *element)
{
	for (uint8_t i = 0; i < sizeof(common.element_pool) / sizeof(*common.element_pool); ++i) {
		if (element == &common.element_pool[i]) {
			common.used &= ~(1 << i);
			break;
		}
	}
}

static int page_element_merge(struct page_element *dest, struct page_element *victim)
{
	if (dest == NULL || victim == NULL) {
		return -1;
	}

	if ((dest->start + dest->length == victim->start) && (dest->owner == victim->owner)) {
		dest->length += victim->length;
		dest->next = victim->next;
		page_element_free(victim);
		return 0;
	}

	return -1;
}

static void page_element_attach(struct page_element **list, struct page_element *element)
{
	struct page_element *it = *list;

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

	if (page_element_merge(it, element) == 0) {
		page_element_merge(it, it->next);
	}
	else {
		page_element_merge(element, element->next);
	}
}

static void page_element_detach(struct page_element **list, struct page_element *element)
{
	struct page_element *it = *list, *prev = NULL;

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

static struct page_element *page_element_split(struct page_element *victim, uint8_t pages)
{
	struct page_element *newborn = page_element_alloc();
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

static uint8_t page_alloc_callback(void *owner, uint8_t pages, page_release callback)
{
	lock_lock(&common.lock);

	struct page_element *curr = common.free, *prev;
	uint8_t page = 0;

	while (curr != NULL && curr->length < pages) {
		prev = curr;
		curr = curr->next;
	}

	if (curr != NULL) {
		if (curr->length == pages) {
			page_element_detach(&common.free, curr);
		}
		else {
			curr = page_element_split(curr, pages);
		}
		curr->callback = callback;
		curr->owner = owner;
		page = curr->start;
		page_element_attach(&common.alloc, curr);
	}

	lock_unlock(&common.lock);

	return page;
}

uint8_t page_alloc(void *owner, uint8_t pages)
{
	return page_alloc_callback(owner, pages, NULL);
}

void page_free(uint8_t page, uint8_t pages)
{
	lock_lock(&common.lock);

	struct page_element *curr = common.alloc;

	while (pages) {
		while (curr != NULL) {
			if (curr->start <= page && (curr->start + curr->length) > page) {
				break;
			}

			curr = curr->next;
		}

		if (curr != NULL) {
			if (curr->start == page) {
				if (curr->length > pages) {
					curr = page_element_split(curr, pages);
				}
				else {
					page_element_detach(&common.alloc, curr);
				}
			}
			else {
				struct page_element *t = page_element_split(curr, page - curr->start);

				/* Need to detach curr to avoid merge in attach */
				page_element_detach(&common.alloc, curr);
				page_element_attach(&common.alloc, t);

				if (curr->length > pages) {
					t = page_element_split(curr, curr->length - pages);
					page_element_attach(&common.alloc, curr);
					curr = t;
				}
			}

			page += curr->length;
			pages -= curr->length;

			page_element_attach(&common.free, curr);
		}
		else {
			break;
		}
	}

	lock_unlock(&common.lock);
}

uint8_t page_cache_alloc(page_release release_callback)
{
	return page_alloc_callback(PAGE_OWNER_CACHE, 1, release_callback);
}

void page_init(uint8_t start, uint8_t pages)
{
	/* No error check, this is certain to be ok. */
	struct page_element *element = page_element_alloc();

	element->start = start;
	element->length = pages;
	element->owner = NULL;

	page_element_attach(&common.free, element);

	lock_init(&common.lock);
}
