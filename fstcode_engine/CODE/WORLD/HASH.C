/* --------------------------------------------------------------------------
 * WORLD		Copyright (c) Simis Ltd 1993,994,1995
 * ---			All Rights Reserved
 *
 * File:		hash.c
 * Ver:			1.00
 * Desc:		Implements general hash table keyed by 32 bit datum
 *
 * -------------------------------------------------------------------------- */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "mem.h"
#include "hash.h"

static Hash_entry *free_hash = NULL;
#define MOREHASH 256

static void more_hash()
{
    int i;
    Hash_entry *h;

    for (i = 0; i < MOREHASH; i++) {
        h = heap_alloc(sizeof(Hash_entry));
        h->next = free_hash;
        free_hash = h;
    }
}

static Hash_entry *alloc_hash()
{
    Hash_entry *h;
    if (free_hash == NULL)
        more_hash();

    h = free_hash;
    free_hash = free_hash->next;
    return h;
}

static void dealloc_hash(Hash_entry *h)
{
    h->next = free_hash;
    free_hash = h;
}

static int hash_code(Hash_table *t, long key)
{
	int size, code;

	size = t->size;
	code = key % size;
	if (code < 0)
		code = -code;
	return code;
}

void hash_add(Hash_table *table, long key, void *data)
{
    Hash_entry* h;
    long code;

    code = hash_code(table, key);
    h = alloc_hash();
    h->next = table->table[code];
    table->table[code] = h;
    h->key  = key;
    h->data = data;
    table->n += 1;
}

void hash_del(Hash_table *table, long key)
{
    Hash_entry** hep;
    Hash_entry* he;
    long int code;

    code = hash_code(table, key);
    for (hep = &(table->table[code]), he = *hep; he; hep = &(he->next), he = *hep) {
        if (key == he->key) {
            *hep = he->next;
            dealloc_hash(he);
            table->n -= 1;
            break;
        }
    }
}

void *hash_get(Hash_table *table, long key)
{
    Hash_entry* h;
    long int code;

    code = hash_code(table, key);
    for (h = table->table[code]; h != NULL; h = h->next) {
        if (key == h->key)
            break;
    }

    return (h) ? h->data : NULL;
}

static void map_hash_table_entry(Hash_entry *e, void(*map)(long int key, void *data, void *handle), void *handle)
{
    Hash_entry *next;

    while (e) {
        next = e->next;
        (*map)(e->key, e->data, handle);
        e = next;
    }
}

void map_hash_table(Hash_table *table, void(*map)(long int key, void *data, void *handle), void *handle)
{
    int i;

    for (i = 0; i < table->size; i++) {
        map_hash_table_entry(table->table[i], map, handle);
    }
}

long int hash_size(Hash_table *t)
{
    return t->n;
}

Hash_table *new_hash_table(int size)
{
    int i;
    Hash_table *t;

    t = heap_alloc(sizeof(Hash_table));
    t->n = 0;
    t->size = size;
    t->table = (Hash_entry**)heap_alloc(size * sizeof(Hash_entry*));
    for (i = 0; i < size; i++)
        t->table[i] = NULL;
    return t;
}

static Hash_entry *copy_entry(Hash_entry *he)
{
	Hash_entry *h;

   	if (he == NULL)
		return NULL;

	h = alloc_hash();
	h->key = he->key;
	h->data = he->data;
	h->next = copy_entry(he->next);

	return h;
}

Hash_table *copy_hash(Hash_table *ht)
{
    int i;
    Hash_table *t;

	t = new_hash_table(ht->size);
    for (i = 0; i < ht->size; i++)
        t->table[i] = copy_entry(ht->table[i]);
    return t;
}

void debug_hash(Hash_table *table, void (*debug_fn)(char *format, ...))
{
    int i;
    Hash_entry *e;

    debug_fn("Show Hash Table (%d)\n", table->size);

    for (i = 0; i < table->size; i++) {
        debug_fn("%d : ", i);
        e = table->table[i];
        while (e) {
            debug_fn("%d ", e->key);
            e = e->next;
        }
        debug_fn("\n");
    }
}
