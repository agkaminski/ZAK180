/* Cyclic list
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#define LIST_ADD(queue, member, next, prev) \
	do { \
		if ((*(queue)) == NULL) { \
			(*(queue)) = (member); \
			(member)->next = (member); \
			(member)->prev = (member); \
		} \
		else { \
			(member)->next = (*(queue)); \
			(member)->prev = (*(queue))->prev; \
			(*(queue))->prev->next = (member); \
			(*(queue))->prev = (member); \
		} \
	} while (0)

#define LIST_REMOVE(queue, member, next, prev) \
	do { \
		if ((member)->next == (member)) { \
			(*(queue)) = NULL; \
		} \
		else { \
			if ((*(queue)) == (member)) { \
				(*(queue)) = (member)->next; \
			} \
			(member)->prev->next = (member)->next; \
			(member)->next->prev = (member)->prev; \
		} \
		(member)->next = NULL; \
		(member)->prev = NULL; \
	} while (0)
