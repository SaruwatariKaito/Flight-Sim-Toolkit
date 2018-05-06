/* List handling library
 * ---------------------
 *
 * Lists are doubly linked and circular,
 * list cells allocated from main heap and deallocated onto
 * local list cell cache (free_lists)
 */

#include <stdlib.h>
#include <stdarg.h>
#include "lists.h"

#define INIT_LISTS 20

extern void stop(char *message);

static List *free_lists = NULL;

/* Returns pointer to a linked list of structures */
/* ->next must be first element of structure */

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void *more_lists(int n, unsigned int size)
{
	int i;
	void *base, *d;

	base = (char *)malloc(size * n);
	if (base == NULL) {
		return NULL;
	}
	d = base;
	for (i = 0; i < (n - 1); i++, d = (void *)((long)d + size)) {
		*((void **)d) = (void *)((long)d + size);
	}
	*((void **)d) = NULL;
	return base;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
List *alloc_list(void)
{
	List *t;

	if ((t = free_lists) == NULL) {
		t = (List *)more_lists(INIT_LISTS, (unsigned)sizeof(List));
		if (t == NULL) {
			stop("List allocation failed");
		}
		free_lists = t;
	}
	free_lists = free_lists->next;
	t->next = NULL;
	return t;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void dealloc_list(List *l)
{
	l->item = NULL;
	l->next = free_lists;
	free_lists = l;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
List *new_list(void)
{
	List *l;

	l = alloc_list();
	l->item = l;
	l->next = l;
	l->prev = l;
	return l;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void list_add(void *item, List *list)
{
	List *l;

	l = alloc_list();
	l->item = item;
	list->next->prev = l;
	l->next = list->next;
	l->prev = list;
	list->next = l;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void list_add_last(void *item, List *list)
{
	List *l;

	l = alloc_list();
	l->item = item;
	list->prev->next = l;
	l->next = list;
	l->prev = list->prev;
	list->prev = l;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void list_insert(void *item, List *list)
{
	List *l;

	l = alloc_list();
	l->item = item;
	l->next = list;
	l->prev = list->prev;
	list->prev->next = l;
	list->prev = l;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void list_unlink(List *n)
{
	n->prev->next = n->next;
	n->next->prev = n->prev;
	n->item = NULL;
	n->next = free_lists;
	free_lists = n;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void list_remove(void *item, List *list)
{
	List *l;

	for (l = list->next; (l->item != item) && (l != list); l = l->next)
		;
	if (l != list) {
		list_unlink(l);
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void *list_remove_first(List *list)
{
	void *item;

	if (list != list->next) {
		item = list->next->item;
		list_unlink(list->next);
		return item;
	}
	else {
        return NULL;
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
int list_member(void *item, List *list)
{
	List *l;

	for (l = list->next; (l->item != item) && (l != list); l = l->next)
		;
	if (l != list) {
        return TRUE;
	}
	return FALSE;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void list_insert_after(void *item, void *after, List *list)
{
	List *l;

	for (l = list->next; (l->item != after) && (l != list); l = l->next)
		;
	if (l != list) {
		list_insert(item, l->next);
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
int list_length(List *list)
{
    int sz;
    List *l;

    for (l = list->next, sz = 0; l != list; l = l->next)
        sz++;
    return sz;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void list_set_add(void *item, List *list)
{
	List *l;

	for (l = list->next; (l->item != item) && (l != list); l = l->next)
		;
	if (l == list) {
        list_add(item, list);
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void list_destroy(List *list)
{
	List *l, *nl;

	if (list->item == list) {
		for (l = list->next; l != list; l = nl) {
			nl = l->next;
			list_unlink(l);
		}
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void list_destroy_all(List *list)
{
	List *l, *nl;

	if (list->item == list) {
		for (l = list->next; l != list; l = nl) {
			nl = l->next;
			list_unlink(l);
		}
		dealloc_list(list);
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// returns nth item in list , first is 0th wraps around list if n > list_length(list)
void* list_nth(List *list, int n)
{
	List *l;

	if (list->next == list) {
		return NULL;
	}

	l = list->next;
	while (n-- > 0) {
		l = l->next;
		if (l == list) {
			l = l->next;  // Skip list head
		}
	}
	return l->item;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// copies contents of list to end of copy - copy must already be a list
void copy_list(List *copy, List *list)
{
	List *l;

	for (l = list->next; l != list; l = l->next) {
		list_add_last(l->item, copy);
	}
}
