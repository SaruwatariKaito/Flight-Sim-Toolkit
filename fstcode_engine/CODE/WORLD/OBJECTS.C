/* --------------------------------------------------------------------------
 * WORLD		Copyright (c) Simis Ltd 1993,994,1995
 * ---			All Rights Reserved
 *
 * File:		objects.c
 * Ver:			1.00
 * Desc:		Implements underlying object system and Base object class
 *
 * -------------------------------------------------------------------------- */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "mem.h"
#include "hash.h"
#include "lists.h"
#include "symbol.h"
#include "objects.h"

Symbol_Table *class_symbol_table;
Symbol_Table *message_symbol_table;

static int   next_class = 1;
static Class *class_tables[MAX_CLASS];

BaseClass *Base_class = NULL;

int objects_created   = 0;
int objects_free      = 0;
int objects_allocated = 0;

static int next_free_id = 1;
Hash_table *object_table = NULL;

Class *get_class(int id);

/* ------------------------
 * free_objects
 * Linked list of unallocated objects, class field
 * set to NULL and id to 0, data field used for link
 * objects are taken from the start of the list and free'd
 * to the end.
 */
typedef struct {
	sObject *first;
	sObject *last;
} ObjectList;
ObjectList free_objects = {NULL, NULL};

/* Object Table contains references to all objects allocated within
 * system.
 * Free objects list pointed to by next_free_object,
 * if value of *next_free_object is non null then that is
 * pointer to next next_free_object else end of free list - all objects
 * above that in array are free.
 *
 */

/* ------------------------------------------------------------------------- */
/* Implementation of Object */

/* -----------------------------------
 * Objects have a unique id used for:
 * 1) Save/Load state
 * 2) Identification and linking with tools/custom code
 */

static void reset_object_table(void)
{
    object_table = new_hash_table(503);
    next_free_id = 1;
    objects_allocated = 0;
}

/* add object to object_table, if no id then allocate next free id
 * update next_free_id
 */
static void register_object(sObject *o, int id)
{
    if (id == 0)
        id = next_free_id++;
    next_free_id = MAX(id+1, next_free_id);

    /* Attempt to register object with id already allocated - not implemented
     */
    o->id = id;
    hash_add(object_table, id, o);
}

static void unregister_object(sObject *o)
{
    int i;

    hash_del(object_table, o->id);
}

int object_id(sObject *o)
{
    if (o)
        return o->id;

    return 0;
}

sObject *get_object(int id)
{
    sObject *o;

    o = hash_get(object_table, id);

    return o;
}

static sObject *block_alloc_object(int id)
{
    sObject *o;

	if (objects_allocated > 1000)
		stop("Object alloc overflow");

    o = block_alloc(sizeof(sObject));
    if (o) {
        objects_allocated += 1;
        o->class = NULL;
        o->data  = NULL;
        register_object(o, id);
    }
    return o;
}

/* Linked object list functions
 * ---------------------------- */
#define INIT_FREE_OBJECTS 100
#define OBJECT_REALLOC 32
static void add_free_obj(sObject *o)
{
	/* Add new object to free objects list */
	o->data  = free_objects.first;		/* Link new object to first element of list */
	if (free_objects.first)
		free_objects.first->class = o;  /* back pointer from first objects in list */
	o->class = NULL;					/* back pointer is NULL at head */

	free_objects.first = o;
	if (free_objects.last == NULL) 		/* If first new object then setup last pointer */
		free_objects.last = o;
	objects_free += 1;
}

/* Remove object from end of list */
static sObject *remove_free_obj(void)
{
	sObject *o;

	o = NULL;
	if (free_objects.last) {
		o = free_objects.last;
		free_objects.last = o->class; /* back pointer for last object on list */
		if (free_objects.last) {
			free_objects.last->data = NULL;
		} else {
			free_objects.first = NULL;
		}
		objects_free -= 1;
	}
	return o;
}

static void more_free_objects(int n)
{
	sObject *o;

	while (n--) {
		o = block_alloc(sizeof(sObject));
		if (o == NULL)
			break;
		objects_created += 1;
		o->id    = 0;
		o->class = NULL;
		o->data  = NULL;
		add_free_obj(o);
	}
}

/* Remove object from end of free objects list */
static sObject *alloc_object(int id)
{
	sObject *o;

	if (free_objects.last == NULL) {
		more_free_objects(OBJECT_REALLOC);
	}
	o = remove_free_obj();
	if (o) {
		o->class = NULL;
		o->data  = NULL;
        objects_allocated += 1;
        register_object(o, id);
		return o;
	}
	return NULL;
}

/* Replace object on head of list */
static sObject *free_object(sObject *o)
{
    o->id    = 0;
	o->class = NULL;
	o->data  = NULL;
	add_free_obj(o);
	return o;
}


/* Remove object from object database and free its memory
 * All existing references to the object must have been removed
 * before this call is made */
void destroy_object(sObject *o)
{
    BaseMethod(o, destroy)(o);

    unregister_object(o);
    o->id    = 0;
    o->class = NULL;
    o->data  = NULL;
	free_object(o); /* Return object to free list */
    /* block_free(o, sizeof(sObject)); */
    objects_allocated -= 1;
}

sObject *make_object(void *class, int id)
{
    sObject   *o;
    BaseClass *c;
    int      *word_ptr;

    o = alloc_object(id);
    if (o) {
        o->class = class;
        c = (BaseClass*)class;
        debug("Make object %d %s\n", object_id(o), c->name);

        BaseMethod(o, create)(o);
        /* Set instace data to all 0's */
        for (word_ptr = (int*)(o->data); word_ptr < (int*)((int)(o->data) + c->sizeof_data); word_ptr++)
            *word_ptr = (int)0;

        /* Reset instance data for new object */
        BaseMethod(o, reset)(o);
    }
    return o;
}

/* Save/Load State
 * ---------------
 */
static void save_object_map(long int id, void *o, void *fp)
{
    sObject *obj;

    obj = o;
    fprintf((FILE*)fp, "%ld %d\n", id, class_id(obj->class));
}

static void save_object_data(long int id, void *o, void *fp)
{
    sObject *obj;

    obj = o;
    fprintf((FILE*)fp, "%ld\n", id);
    BaseMethod(obj, file_save)(obj, ((FILE *)fp));
}

void save_objects(FILE *fp)
{
    int i;
    sObject *o;

    /* Save object map */
    fprintf(fp, "ObjectMap\n");
    map_hash_table(object_table, save_object_map, fp);
    fprintf(fp, "0\n");

    fprintf(fp, "ObjectData\n");
    map_hash_table(object_table, save_object_data, fp);
    fprintf(fp, "0\n");
}

static void load_object_map(FILE *fp)
{
    char comment[16];
    int    id, class_id;

    /* Load object map */
    fscanf(fp, "%s", comment);
    if (strcmp(comment, "ObjectMap") != 0)
        stop("Error loading ObjectMap (%s found)", comment);

    fscanf(fp, "%d", &id);
    while (id) {
        fscanf(fp, "%d", &class_id);
        make_object(get_class(class_id), id);
        fscanf(fp, "%d", &id);
    }
}

void load_objects(FILE *fp)
{
    char comment[16];
    sObject *o;
    int    id;

    load_object_map(fp);

    fscanf(fp, "%s", comment);
    if (strcmp(comment, "ObjectData") != 0)
        stop("Error loading ObjectData (%s found)", comment);

    fscanf(fp, "%d", &id);
    while (id) {
        o = get_object(id);
        if (o == NULL)
            stop("Error load_objects\n");

        BaseMethod(o, file_load)(o, fp);

        fscanf(fp, "%d", &id);
    }
}

static void hash_destroy_object(long int id, void *o, void *handle)
{
    destroy_object((sObject*)o);
}

void reset_objects(void)
{
    map_hash_table(object_table, hash_destroy_object, NULL);
}

/* ------------------------------------------------------------------------- */
/* Implementation of class system */

/* Copy the superclass table into the top of the subclass table */
static void copy_class_table(Class *superclass, Class *subclass)
{
    BaseClass *c;
    char *byte_ptr1, *byte_ptr2;
    int   nbytes;

    c = superclass;
    nbytes = c->sizeof_class;
    byte_ptr1 = superclass;
    byte_ptr2 = subclass;
    while (nbytes--) {
        *byte_ptr2++ = *byte_ptr1++;
    }
}

int class_id(Class *c)
{
    int i;

    for (i = 1; i < MAX_CLASS; i++) {
        if (class_tables[i] == c)
            return i;
    }
    return 0;
}

Class *get_class(int id)
{
    if (id > 0 && id < MAX_CLASS)
        return class_tables[id];

    return NULL;
}

Class *new_class(int sizeof_class, char *name, int sizeof_data, Class *superclass)
{
    BaseClass *c;
    BaseClass *sc;
    sSymbol *s;

    c = block_alloc(sizeof_class);
    if (c) {
        if (superclass != NULL)
            copy_class_table(superclass, c);
        strncpy(c->name, name, 16);
        c->sizeof_data = sizeof_data;
        c->sizeof_class = sizeof_class;
        c->superclass = superclass;
        if (superclass != NULL) {
            sc = superclass;
            c->methods = copy_hash(sc->methods);
        } else {
            c->methods = new_hash_table(DEFAULT_MESSAGE_HASH_SIZE);
        }

        class_tables[next_class++] = c;

        /* Create a symbol - classname and assign the class to it */
        s = get_symbol(class_symbol_table, name);
        if (s)
            s->value = c;
    }
    return c;
}

Class *copy_class(Class *class, char *name)
{
    BaseClass *c, *copy;
    sSymbol *s;

    c = class;
    copy = block_alloc(c->sizeof_class);

    if (copy) {
        copy_class_table(class, copy);
        strncpy(copy->name, name, 16);
        copy->methods = copy_hash(c->methods);
        class_tables[next_class++] = copy;

        /* Create a symbol - classname and assign the class to it */
        s = get_symbol(class_symbol_table, name);
        if (s)
            s->value = copy;
    }
    return copy;
}

void print_class_tables(void)
{
    FILE *fp;
    int  i;
    BaseClass *c;

    fp = fopen("class", "w");
    if (fp) {
        for (i = 0; i < MAX_CLASS; i++) {
            c = class_tables[i];
            if (c) {
                fprintf(fp, "%3d : %-16s %3d", i, c->name, c->sizeof_data);
                if (c->superclass) {
                    fprintf(fp, " [%s]", ((BaseClass*)(c->superclass))->name);
                }
                fprintf(fp, "\n");
            }
        }
        fclose(fp);
    }
}

void init_all_classes(void)
{
    int i;
    BaseClass *c;

	for (i = 0; i < MAX_CLASS; i++) {
        c = class_tables[i];
        if (c && c->class_init)
            (*c->class_init)((Class*)c);
    }
}

/* ------------------------------------------------------------------------- */
int check_call(Class *class, char *method, sObject *obj)
{
    BaseClass *c, *oc;
    Base *b;
    int id;

	return 1;

    c  = class;
    if (obj == NULL) {
        debug("Check call %-10s %-10s %d - NULL object\n", c->name, method);
        return FALSE;
    }

    oc = obj->class;
    b  = obj->data;

    if (oc == NULL) {
        debug("Check call %-10s %-10s %d - object class pointer NULL\n", c->name, method, obj->id);
        return FALSE;
    }

    /* Check object is in object table */
    id = object_id(obj);
    if (get_object(id) == NULL) {
        debug("Check call %-10s %-10s %d [%s] - object not in object table\n", c->name, method, obj->id, oc->name);
        return FALSE;
    }

/*
    if (addr == NULL) {
        debug("Check call %-10s %-10s %d [%s] - address of method is NULL\n", c->name, method, obj->id, oc->name);
        return FALSE;
    }
 */
    /* debug("Check call %-10s %-10s %d [%s] - ok\n", c->name, method, obj->id, oc->name); */
    return TRUE;
}

/* ------------------------------------------------------------------------- */
/* Implementation of base class */

void base_class_init(Class *c)
{
}

void data_alloc(sObject *o)
{
    BaseClass *c;

    c = o->class;
    o->data = block_alloc(c->sizeof_data);
}

static void data_dealloc(sObject *self)
{
    BaseClass *c;

    c = self->class;
    block_free(self->data, c->sizeof_data);
}

static void file_save(sObject *self, FILE *fp)
{
    Base *o;

    o = self->data;
    fprintf(fp, "%d\n", o->flags);
    SaveVector(fp, o->p);
}

static void file_load(sObject *self, FILE *fp)
{
    Base *o;

    o = self->data;
    fscanf(fp, "%d\n", &o->flags);
    LoadVector(fp, o->p);
}

/* Method used to read in an inital world state */
static void file_read(sObject *self, FILE *fp)
{
    Base *o;

    o = self->data;
    LoadVector(fp, o->p);
}

/* ---------------------------------------------------------------------------
 * Symbolic Message Method
 *
 */

static int next_free_message = 1;

static long int next_free_mess(int arity)
{
  sMessage m;
  union {sMessage m; long int l;} u;

  m.arity = arity;
  m.id	  = next_free_message++;
  u.m     = m;

  return u.l;
}

static void init_messages(void)
{
	next_free_message = 1;
}

static void *find_method_address(Hash_table *methods, sSymbol *s)
{
    /* List *l; */
    sMethod *m;

	m = hash_get(methods, (long int)s);
/*    for (l = methods->next; l != methods; l = l->next) {
        m = l->item;
        if (m->id == s)
            return m->address;
    }*/
    return (m) ? m->address : NULL;
}

sSymbol *add_message(Class *c, char *name, int arity, void *method_address)
{
    BaseClass *class;
    sMethod    *m;
	sSymbol    *s;

	s = get_symbol(message_symbol_table, name);
	if (s != nil_symbol) {
		if (s->value == 0) {
			/* New message - set arity */
			s->value = (void*)arity;
		}
		class = c;
    	m = block_alloc(sizeof(sMethod));
    	if (m) {
        	m->id = s;
        	m->address = method_address;
			hash_add(class->methods, (long int)s, (void*)m);
    	}
		return s;
	}
	return NULL;
}

IV message(sObject *self, sSymbol *message, ...)
{
    BaseClass *class;
    va_list ap;
    IV a1,a2,a3,a4,r;
    void *method_address;

    debug("message [%d] %s\n", object_id(self), message->name);
    if (self == NULL || message == NULL) {
       return (IV)NULL;
    }

    class = self->class;
    method_address = find_method_address(class->methods, message);
    if (method_address == NULL) {
         /* Method not implemented for class */
        return (IV)NULL;
    }
    debug("method address %x\n", method_address);

    va_start(ap, message);
    switch ((long int)message->value) {
        case 0:
            va_end(ap);
            debug("send %s (%d)\n", message->name, 0);
            r = (*((Arity0)method_address))(self);
            break;
        case 1:
            a1 = va_arg(ap, long int);
            va_end(ap);
            debug("send %s (%ld) %ld\n", message->name, (long int)message->value, a1);
            r = (*((Arity1)method_address))(self, a1);
            break;
        case 2:
            a1 = va_arg(ap, long int);
            a2 = va_arg(ap, long int);
            va_end(ap);
            debug("send %s (%d)\n", message->name, message->value);
            r = (*((Arity2)method_address))(self, a1, a2);
            break;
        case 3:
            a1 = va_arg(ap, long int);
            a2 = va_arg(ap, long int);
            a3 = va_arg(ap, long int);
            va_end(ap);
            debug("send %s (%d)\n", message->name, message->value);
            r = (*((Arity3)method_address))(self, a1, a2, a3);
            break;
        case 4:
            a1 = va_arg(ap, long int);
            a2 = va_arg(ap, long int);
            a3 = va_arg(ap, long int);
            a4 = va_arg(ap, long int);
            va_end(ap);
            debug("send %s (%d)\n", message->name, message->value);
            r = (*((Arity4)method_address))(self, a1, a2, a3, a4);
            break;
        default:
            r = NULL;
            break;
    }

    return r;
}

/* ------------------------------------------------------------------------- */


static void base_debug(sObject *self)
{
    debug("Object %d %s\n", object_id(self), ((BaseClass*)(self->class))->name);
}

static IV test(sObject *self, IV a1)
{
    debug("Test %d %ld\n", object_id(self), a1);
	return a1;
}

sObject *new_object(void *class)
{
    return make_object(class, 0);
}

void init_objects(void)
{
    int i;

	reset_object_table();
    init_messages();

    /* Reset class system */
    for (i = 0; i < MAX_CLASS; i++) {
        class_tables[i] = NULL;
    }
    next_class = 1;

}


void setup_objects(void)
{
    int t;

	/* Setup free objects list */
	more_free_objects(INIT_FREE_OBJECTS);

	class_symbol_table   = new_symbol_table();
	message_symbol_table = new_symbol_table();

    /* Create base class */
    Base_class = new_class(sizeof(BaseClass), "Base", sizeof(Base), NULL);

    Base_class->class_init = base_class_init;
    Base_class->reset      = NULL;
    Base_class->create     = data_alloc;
    Base_class->destroy    = data_dealloc;
    Base_class->file_save  = file_save;
    Base_class->file_load  = file_load;
    Base_class->file_read  = file_read;
    Base_class->debug      = base_debug;
}

