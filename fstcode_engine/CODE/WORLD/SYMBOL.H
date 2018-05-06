#ifndef symbol_h
#define symbol_h

#include "mem.h"
#include "lists.h"

typedef struct {
    char name[32];
    void *value;
} sSymbol;

typedef sList Symbol_Table;

#define symbol_name(S) ((S)->name)
#define symbol_value(S) ((S)->value)

extern sSymbol *nil_symbol;

extern void init_symbols(void);
extern void setup_symbols(void);
extern void tidyup_symbols(void);

extern Symbol_Table *new_symbol_table(void);

/* find_symbol()
 * search symbol table for existing symbol,
 * returns nil_symbol if not found
 */
extern sSymbol *find_symbol(Symbol_Table *st, char *name);

/* get_symbol()
 * search symbol table for existing symbol,
 * if not found creates symbol and sets value to NULL
 */
extern sSymbol *get_symbol(Symbol_Table *st, char *name);

extern void apply_to_symbol_table(Symbol_Table *symbol_table, void (*fn)(sSymbol *s, void *handle), void *handle);

#endif
