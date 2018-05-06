/* --------------------------------------------------------------------------
 * WORLD		Copyright (c) Simis Ltd 1993,994,1995
 * ---			All Rights Reserved
 *
 * File:		symbol.c
 * Ver:			1.00
 * Desc:		Implements run time symbols.
 *
 * -------------------------------------------------------------------------- */
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "symbol.h"

sSymbol *nil_symbol = NULL;


/* --------------------
 * LOCAL alloc_symbol
 * 
 * Allocate symbol data structure and setup contents
 */ 
static sSymbol *alloc_symbol(char *name)
{
    sSymbol *s;

    s = block_alloc(sizeof(sSymbol));
    if (s) {
        strncpy(s->name, name, 31);
        s->value = NULL;
    }

    return s;
}


/* --------------------
 * LOCAL str_eq
 * 
 * Check for string quality - ignore case
 */ 
static int str_eq(char *s1, char *s2, int max)
{
    int eq;
	int c1, c2;

	eq = TRUE;
	while (*s1 && *s2 && (max-- > 0)) {
	   c1 = tolower(*s1);
	   c2 = tolower(*s2);
	   if (c1 != c2) {
		 eq = FALSE;
		 break;
	   }
	   s1 += 1;
	   s2 += 1;
	}

    return ((eq == TRUE) && (*s1 == NULL) && (*s2 == NULL));
}


/* -------------------
 * GLOBAL get_symbol
 * 
 * Find symbol from symbol table,  if not found then
 * create new symbol.
 * 
 * Always returns a symbol.
 * 
 */
sSymbol *get_symbol(Symbol_Table *symbol_table, char *name)
{
    sList *l;
    sSymbol *s;
    int i;

    /* Check message does not already exist */
    s = nil_symbol;
    for (l = symbol_table->next; l != symbol_table; l = l->next) {
        s = l->item;
        if (str_eq(s->name, name, 31)) {
            return s;
        }
    }
    /* Symbol not found - create new */
    s = alloc_symbol(name);
    if (s) {
        list_add_last(s, symbol_table);
    }

    return s;
}



/* -------------------
 * GLOBAL find_symbol
 * 
 * Finds symbol from symbol table,  if not found return cNIL.
 * 
 */
sSymbol *find_symbol(Symbol_Table *symbol_table, char *name)
{
    sList *l;
    sSymbol *s;
    int i;

    /* Check symbol does not already exist */
    s = nil_symbol;
    for (l = symbol_table->next; l != symbol_table; l = l->next) {
        s = l->item;
        if (str_eq(s->name, name, 31)) {
            return s;
        }
    }

    return nil_symbol;
}


/* -------------------
 * GLOBAL apply_to_symbol_table
 * 
 * Apply fn to all symbols in symbol table.
 * 
 */
void apply_to_symbol_table(Symbol_Table *symbol_table, void (*fn)(sSymbol *s, void *handle), void *handle)
{
    sList *l;
    sSymbol *s;

    for (l = symbol_table->next; l != symbol_table; l = l->next) {
    	s = l->item;
		(*fn)(s, handle);
    }
}



/* -------------------
 * GLOBAL new_symbol_table
 * 
 * Create new symbol table.
 * 
 */
Symbol_Table *new_symbol_table(void)
{
	Symbol_Table *symbol_table;

	if (nil_symbol == NULL)
		stop("Call to new_symbol_table() before setup_symbols()\n");
    symbol_table = new_list();
    list_add_last(nil_symbol, symbol_table);

	return symbol_table;
}

void init_symbols(void)
{

}

void setup_symbols(void)
{

    /* Setup nil symbol */
    nil_symbol = alloc_symbol("nil");
    nil_symbol->value = NULL;
}

void tidyup_symbols(void)
{
}
