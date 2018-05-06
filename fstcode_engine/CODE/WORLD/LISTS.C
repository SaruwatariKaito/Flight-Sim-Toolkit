/* --------------------------------------------------------------------------
 * WORLD		Copyright (c) Simis Ltd 1993,994,1995
 * ---			All Rights Reserved
 *
 * File:		lists.c
 * Ver:			2.00
 * Desc:		List handling library
 *
 * Lists are double linked and circular,
 * list cells allocated from main heap and deallocated onto
 * local list cell cache (free_lists)
 * 
 * -------------------------------------------------------------------------- */
#include <stdlib.h>
#include <stdarg.h>
#include "lists.h"

#include "mem.h"

#define INIT_LISTS 80

// count of list cells allocated
int list_count = 0;

// free list cells
static sList *free_lists = NULL;

// backup free list cells - only used when main heap is exhausted
static sList *emergency_lists = NULL;

/* --------------------------------------------
 * LOCAL more_lists
 * Returns pointer to a linked list of structures
 * ->next must be first element of structure
 */
static void *more_lists(int n, unsigned int size)
{
    int i;
    void *base, *d;

    base = (char *)alloc_bytes(size * n);
    if (base == NULL)
		return NULL;
    d = base;
    for (i = 0; i < (n - 1); i++, d = (void *)((long)d + size)) {
        *((void **)d) = (void *)((long)d + size);
    }
    *((void **)d) = NULL;
    return base;
}

/* --------------------
 * GLOBAL alloc_list
 * allocate a list cell from the free lists
 */
sList *alloc_list(void)
{
    sList *t;

    list_count++;
    if ((t = free_lists) == NULL) {
        t = (sList *)more_lists(INIT_LISTS, (unsigned)sizeof(sList));
		if (t == NULL) {
			if (emergency_lists == NULL) {
				stop("Emergency list allocation failed");
			}
			t = emergency_lists;
			emergency_lists = NULL;
		}
        free_lists = t;
    }
    free_lists = free_lists->next;
    t->next = NULL;
    return t;
}

/* --------------------
 * GLOBAL dealloc_list
 * Place a list cell back on the free list
 */
void dealloc_list(sList *l)
{
    list_count--;
    l->item = NULL;
    l->next = free_lists;
    free_lists = l;
}

/* --------------------
 * GLOBAL new_list
 * Allocate and reset a new list head
 */
sList *new_list(void)
{
    sList *l;

    l = alloc_list();
    l->item = l;
    l->next = l;
    l->prev = l;
    return l;
}

/* --------------------
 * GLOBAL list_add
 * Add item to the head of list supplied
 */
void list_add(void *item, sList *list)
{
    sList *l;

    l = alloc_list();
    l->item = item;
    list->next->prev = l;
    l->next = list->next;
    l->prev = list;
    list->next = l;
}

/* --------------------
 * GLOBAL list_add_last
 * Add item to end of the list supplied
 */
void list_add_last(void *item, sList *list)
{
    sList *l;

    l = alloc_list();
    l->item = item;
    list->prev->next = l;
    l->next = list;
    l->prev = list->prev;
    list->prev = l;
}

/* --------------------
 * GLOBAL list_insert
 * Link item into list after list cell supplied
 */
void list_insert(void *item, sList *list)
{
    sList *l;

    l = alloc_list();
    l->item = item;
    l->next = list;
    l->prev = list->prev;
    list->prev->next = l;
    list->prev = l;
}

/* --------------------
 * GLOBAL list_unlink
 * Unlink list cell from list, adds cell to free list, list cell
 * must not be head of list.
 */
void list_unlink(sList *n)
{
    list_count--;
    n->prev->next = n->next;
    n->next->prev = n->prev;
    n->item = NULL;
    n->next = free_lists;
    free_lists = n;
}

/* --------------------
 * GLOBAL list_remove
 * Search for item in list, if found unlinks the holding list cell
 */
void list_remove(void *item, sList *list)
{
    sList *l;

    for (l = list->next; (l->item != item) && (l != list); l = l->next)
            ;
    if (l != list)
        list_unlink(l);
}

/* --------------------
 * GLOBAL list_first
 * Returns the first item in the list
 */
void *list_first(sList *list)
{
	void *item;

    item = NULL;
	if (list != list->next) {
        item = list->next->item;
	}
	return item;
}

/* --------------------
 * GLOBAL list_remove_first
 * Returns the first item in the list and removes it from the list
 */
void *list_remove_first(sList *list)
{
    void *item;

    if (list != list->next) {
        item = list->next->item;
        list_unlink(list->next);
        return item;
    }
    else
        return NULL;
}

/* --------------------
 * GLOBAL list_next
 * Finds item in list and returns next item,
 * returns NULL if item is not found
 */
void* list_next(void *item, sList *list)
{
    sList *l;

    for (l = list->next; (l->item != item) && (l != list); l = l->next)
            ;
    if (l != list) {
		l = l->next;
		l = (l == list) ? list->next : l;
		return (l == list) ? NULL : l->item;
	} else {
		return NULL;
	}
}


/* --------------------
 * GLOBAL list_prev
 * Finds item in list and returns previous item,
 * returns NULL if item is not found
 */
void* list_prev(void *item, sList *list)
{
    sList *l;

    for (l = list->next; (l->item != item) && (l != list); l = l->next)
            ;
    if (l != list) {
		l = l->prev;
		l = (l == list) ? list->prev : l;
		return (l == list) ? NULL : l->item;
	} else {
		return NULL;
	}
}

/* --------------------
 * GLOBAL list_member
 * Returns true if item is in list
 */
int list_member(void *item, sList *list)
{
    sList *l;

    for (l = list->next; (l->item != item) && (l != list); l = l->next)
            ;
    if (l != list)
        return TRUE;
    return FALSE;
}

/* --------------------
 * GLOBAL list_length
 * Returns length of list
 */
int list_length(sList *list)
{
    int len;
    sList *l;

    for (l = list->next, len = 0; l != list; l = l->next)
        len++;
    return len;
}

/* --------------------
 * GLOBAL list_set_add
 * Adds item to head of list if item is not already in list
 */
void list_set_add(void *item, sList *list)
{
    sList *l;

    for (l = list->next; (l->item != item) && (l != list); l = l->next)
            ;
    if (l == list)
        list_add(item, list);
}

/* --------------------
 * GLOBAL list_append
 * Appends two lists together.
 */
void list_append(sList *l1, sList *l2)
{
	if (l2->next != l2) {
		l1->prev->next = l2->next;
		l2->next->prev = l1->prev;
		l1->prev = l2->prev;
		l2->prev->next = l1;
		l2->next = l2;
		l2->prev = l2;
	}
}

/* --------------------
 * GLOBAL copy_list
 * Creates and returns an entire copy of list
 */
sList *copy_list(sList *list)
{
    sList *l, *new;

    new = new_list();

    for (l = list->next; (l != list); l = l->next)
        list_add_last(l->item, new);

    return new;
}

/* --------------------
 * GLOBAL list_destroy
 * Frees contents of list, list head is left intact
 */
void list_destroy(sList *list)
{
    sList *l, *nl;

    if (list->item == list) {
        for (l = list->next; l != list; l = nl) {
            nl = l->next;
            list_unlink(l);
        }
    }
}

/* --------------------
 * GLOBAL list_destroy_all
 * Frees contents of list, list head is also deallocated
 *
 */
void list_destroy_all(sList *list)
{
    sList *l, *nl;

    if (list->item == list) {
        for (l = list->next; l != list; l = nl) {
            nl = l->next;
            list_unlink(l);
        }
        dealloc_list(list);
    }
}

/* --------------------
 * GLOBAL init_lists
 * Setup free list structures
 */
void init_lists(void)
{
    free_lists = (sList *)more_lists(INIT_LISTS, (unsigned)sizeof(sList));
	if (free_lists == NULL)
		stop("No memory for initial lists");
    emergency_lists = (sList *)more_lists(INIT_LISTS, (unsigned)sizeof(sList));
	if (emergency_lists == NULL)
		stop("No memory for emergency lists");
}

