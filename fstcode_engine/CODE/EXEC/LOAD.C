/* --------------------------------------------------------------------------
 * EXEC 		Copyright (c) Simis Ltd 1993,994,1995
 * ---			All Rights Reserved
 *
 * File:		load.c
 * Ver:			2.00
 * Desc:		Loads contents of world.fst and creates objects and paths
 * 
 *
 * -------------------------------------------------------------------------- */
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <dos.h>

#include "shape.h"
#include "world.h"
#include "defs.h"

Symbol_Table *object_symbol_table;
Symbol_Table *value_symbol_table;
Symbol_Table *path_symbol_table;


/* EXTERN file_read_basic
 * ------
 * reads initial object data from world file (common to all classes)
 * into Basic_data structure, class file_read() methods use this
 * to read data then read class data themeselves
 */
void file_read_basic(Basic_data *bd, FILE *fp)
{
    fscanf(fp, "%ld %ld %ld %d %d ", &bd->x, &bd->y, &bd->z, &bd->ry, &bd->f);
    fscanf(fp, "%s\n", bd->shape);
    fscanf(fp, "%d", &bd->strength);
    fscanf(fp, "%s\n", bd->dead);
	bd->f &= 0xff00; /* mask of system flags used by editor and simulator for
					  * different purposes */
}


char terrain_name[32] = "T000000.FTD";

static char read_text[32];
/* World file format:
 * line 1: FSTPC Version
 * line 2:
 */

/* Reads symbol string from file, either gets or finds (creates if
 * it does not exist) symbol from symbol table
 * If fscanf fails then returns NULL
 * If symbol not found returns nil_symbol
 * else returns symbol */
static sSymbol *read_symbol(FILE *fp, Symbol_Table *st, int create)
{
    sSymbol *s;

    s = NULL;
    if (fscanf(fp, "%s", read_text) == 1)
        s = (create) ? get_symbol(st, read_text) : find_symbol(st, read_text);

    return s;
}

static int load_header_skip_sub(FILE *fp)
{
	char sym[32];

	/* read until symbol == Objects */
	if (fscanf(fp, "%s", sym) == 1) {
	  if (strcmp(sym, "Objects") == 0)
        return TRUE;
      return -1;
	}
	return FALSE;
}

static int load_header_skip(FILE *fp)
{
	int code;

	while((code = load_header_skip_sub(fp)) == -1)
		 ;
	return code;
}

static int load_header(FILE *fp)
{
	return TRUE;
}

/* Read symbols from file until a class symbol is
 * recognised - skips erroneous data and unimplemented
 * classes
 * returns NULL at EOF or an existing class symbol */
sSymbol *read_to_class_symbol(FILE *fp)
{
	sSymbol *class;

    class = read_symbol(fp, class_symbol_table, FALSE);
	while (class == nil_symbol) {
		debug("read_to_class_symbol skip - %s\n", read_text);
    	class = read_symbol(fp, class_symbol_table, FALSE);
	}
	return class;
}


/* --------------------
 * LOCAL load_objs
 * 
 * Load OBJECTS section of world.fsd
 * Reads until a registered class is recognised
 * Creates object of class read
 * Invokes object file_read method to read class IV setup
 * 
 */ 
static int load_objs(FILE *fp)
{
    char comment[32], shape[32];
    sSymbol *class, *name;
    int n, r, flags;
    long int x, y, z;
    sObject *o;
    World *w;

    fscanf(fp, "%s %d\n", comment, &n);
    if (strncmp(comment, "Objects", 32) != NULL) {
        werror("load_objects read %s", comment);
        return FALSE;
    }

    while (n--) {
        /* Read object - class, symbolic name
         *               position name
         */
        class = read_to_class_symbol(fp);
		if (class == NULL) {
        	werror("load_objects reached EOF while searching for class");
        	return FALSE;
		}
		if (class->value == NULL) {
		  stop("Impossible error - class symbol with NULL value [%s]\n", class->name);
		}
		debug("read class symbol %s\n", class->name);

        name  = read_symbol(fp, object_symbol_table, TRUE);
		if (name == NULL) {
        	werror("load_objects reached EOF while reading name");
        	return FALSE;
		}

        o = new_object(class->value);
        if (o == NULL)
	    	stop("load_objects : new_object returned NULL\n");

    	SafeMethod(o, World, file_read)(o, fp);
        attach_object(o);
        if (name != nil_symbol)
            name->value = o;
	}
    return TRUE;
}

static Path *alloc_path(void)
{
	Path *p;

	p = block_alloc(sizeof(Path));
	if (p == NULL)
		stop("No memory for new path");
	p->n = 0;
	p->wpts = new_list();

	return p;
}

static Wpt *new_wpt(int32 x, int32 y, int32 z, int spd, int active)
{
	Wpt *w;

	w = block_alloc(sizeof(Wpt));
	if (w == NULL)
		stop("No memory for new waypoint");
	w->p.x   = M(x);
	w->p.y   = M(y);
	w->p.z   = M(z);
	w->spd	 = FIXSPD(spd);
	w->active = active;
	return w;
}

static int load_path(FILE *fp)
{
	sSymbol *name, *supply;
	Path *p;
	Wpt *w;
	long int x, y, z;
	int      n, spd, active;

    name  = read_symbol(fp, path_symbol_table, TRUE);
	if (name == NULL) {
        werror("load_path reached EOF while reading name");
        return FALSE;
	}
    supply  = read_symbol(fp, object_symbol_table, FALSE);
	p = alloc_path();
	fscanf(fp, "%d", &n);
	p->n = n;
	while (n--) {
		if (world_file_version >= 4) {
			fscanf(fp, "%ld %ld %ld %d %d", &x, &y, &z, &spd, &active);
		} else {
			fscanf(fp, "%ld %ld %ld %d", &x, &y, &z, &spd);
			active = TRUE;
		}
		w = new_wpt(x, y, z, spd, active);
		list_add_last(w, p->wpts);
	}
    if (name != nil_symbol)
        name->value = p;
	p->supply = supply;

	return TRUE;
}


/* --------------------
 * LOCAL load_paths
 * 
 * Load PATHS section of world.fst
 * 
 */ 
static int load_paths(FILE *fp)
{
    char comment[32], symbol_name[32];
    sSymbol *symbol;
    int n, next;

	/* Read Symbols section header */
    fscanf(fp, "%s %d %d\n", comment, &n, &next);
    if (strncmp(comment, "Paths", 32) != NULL) {
        werror("load_objects read %s expecting Paths", comment);
        return FALSE;
    }

    while (n--) {
		if (load_path(fp) == FALSE) {
			return FALSE;
		}
	}

    return TRUE;
}


/* --------------------
 * LOCAL load_symbols
 * 
 * Load symbol section of world.fst
 */ 
static int load_symbols(FILE *fp)
{
    char comment[32], symbol_name[32];
    sSymbol *symbol;
    int symbol_value;

	/* Read Symbols section header */
    fscanf(fp, "%s\n", comment);
    if (strncmp(comment, "Symbols", 32) != NULL) {
        werror("load_objects read %s expecting Symbols", comment);
        return FALSE;
    }

    while (fscanf(fp, "%s %d\n", symbol_name, &symbol_value) == 2) {
		symbol = get_symbol(value_symbol_table, symbol_name);
        if (symbol && (symbol != nil_symbol))
			symbol->value = (void*)symbol_value;
	}
    return TRUE;
}


/* -------------------
 * GLOBAL load_worldfile
 * Called after initialisation and setup OR after reset
 * 
 */
int load_worldfile(char *name)
{
    FILE *fp;
    char type[80], s[16];
    int  version, ok;

	object_symbol_table  = new_symbol_table();
	value_symbol_table   = new_symbol_table();
	path_symbol_table   = new_symbol_table();

    ok = TRUE;
    fp = fopen(name, "r");
    if (fp) {
        /* Check file is correct type and version */
        if (fscanf(fp, "%s %d", type, &version) != 2) {
            werror("load_worldfile - first line");
            ok = FALSE;
        }
        if (ok && strncmp(type, "FSTPC", 32) != 0) {
            werror("load_worldfile - first token %s", type);
            ok = FALSE;
        }
        if (ok && version < 1 || version > 4) {
            werror("load_worldfile - wrong version %d", version);
            ok = FALSE;
        } else {
			world_file_version = version;
        }
        if (ok) {
            ok = load_header(fp);
			strcpy(s, "header");
		}
        if (ok) {
			ok = load_objs(fp);
			strcpy(s, "objects");
		}
        if (ok) {
            ok = load_paths(fp);
			strcpy(s, "paths");
		}
        if (ok) {
            ok = load_symbols(fp);
			strcpy(s, "symbols");
		}
		if (ok == FALSE) {
			stop("Error during load %s", s);
		}
        fclose(fp);
    } else {
        stop("World file %s not found", name);
    }
    return ok;
}

