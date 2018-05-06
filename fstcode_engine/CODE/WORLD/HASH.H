#ifndef hash_h
#define hash_h

#include <string.h>
#include <ctype.h>

typedef struct Hash_entry {
    struct Hash_entry* next;
    long int key;
    void *data;
} Hash_entry;

typedef struct {
    int  n, size;
    Hash_entry** table;
} Hash_table;

extern void        hash_add(Hash_table *table, long key, void *data);
extern void        hash_del(Hash_table *table, long key);
extern void *      hash_get(Hash_table *table, long key);
extern long int    hash_size(Hash_table *table);
extern Hash_table *new_hash_table(int size);
extern Hash_table *copy_hash(Hash_table *t);
extern void        map_hash_table(Hash_table *table, void (*map)(long int key, void *data, void *handle), void *handle);
extern void debug_hash(Hash_table *table, void (*debug_fn)(char *format, ...));

#endif
