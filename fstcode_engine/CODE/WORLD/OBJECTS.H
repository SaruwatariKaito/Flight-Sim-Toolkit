#ifndef objects_h
#define objects_h

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#include "mem.h"
#include "lists.h"
#include "hash.h"
#include "graph.h"
#include "wdefs.h"

/* objects implements Object and Class system and base class "Obj" */

/* Object - basic structure contains pointer to class definition
 *          and pointer to instance data.
 *
 * Class definition contains class variables and method table
 * which defines class
 *
 * Lexical conventions
 *
 * Defining a new class NC
 *
 * 1 - include superclass SC.h file
 * 2 - define NCClassDef - macro for class table structure
 *   - define NCDef      - macro for instance data structre
 * 3 - typedef CClass and C - class table and instance data typedefs
 */

#define MAX_CLASS 1024
typedef void Class;

typedef struct sObject {
    int   id;
    Class *class;
    void  *data;
} sObject;

/* Macros defining class */
/* --------------------- */
/* Class xxx:
 *
 * class:
 * xxx   - used by Macros - Method(),
 *
 * class table type:
 * xxxClass  - typedef for class table       - created by Declare_Types
 *
 * class instance type:
 * xxx       - typedef for instance variable - create by Declare_Types
 *
 * class pointer:
 * xxx_class - pointer to class structure    - created by Declare_Class,
 *                                             setup by MakeClass
 *
 * xxx_Methods and xxx_Vars - #defines for class
 */

/* Run time macros */
/* --------------- */
#define MethodValue(O,C,M) 	((C##Class*)((O)->class))->M
#define MethodExists(O,C,M) (((C##Class*)((O)->class))->M != NULL)
#define MethodXXX(O,C,M) 		((C##Class*)((O)->class))->M
#define MethodYYY(O,C,M) if (check_call((Class*)C##_class, #M, (O)) != NULL && ((((C##Class*)((O)->class))->M) != NULL)) ((C##Class*)((O)->class))->M
#define SafeMethod(O,C,M) if ((((C##Class*)((O)->class))->M) != NULL) ((C##Class*)((O)->class))->M
#define CallMethod(O,C,M) 	((C##Class*)((O)->class))->M

#define SetMethod(C, M, F) ((C)->M = F)

#define Overide(C, M, F) ((C)->M = F)

/* --------------------------------------------------------------------------
 * Symbolic Method Access
 * ----------------------
 */

#define DEFAULT_MESSAGE_HASH_SIZE 8

typedef struct {
	short int arity, id;
} sMessage;

typedef struct {
    sSymbol *id;
    void   *address;
} sMethod;

typedef long int IV;

typedef IV (*Arity0)(sObject *);
typedef IV (*Arity1)(sObject *, IV);
typedef IV (*Arity2)(sObject *, IV, IV);
typedef IV (*Arity3)(sObject *, IV, IV, IV);
typedef IV (*Arity4)(sObject *, IV, IV, IV, IV);

/* --------------------------------------------------------------------------
 * Definitions for class Base
 * ------------------------- */

/* Definitions for base class in object system "Base"
 * No superclass
 */

#define Base_Methods                        \
    /* Class Variables */                   \
    char name[16];                          \
    int  sizeof_class;                      \
    int  sizeof_data;                       \
    Class *superclass;                      \
    Hash_table *methods;                    \
    void (*class_init)(Class *c);           \
    /* Instance Methods */                  \
    void (*reset)(sObject *);                \
    void (*create)(sObject *);               \
    void (*destroy)(sObject *);              \
    void (*file_save)(sObject *, FILE *fp);  \
    void (*file_load)(sObject *, FILE *fp);  \
	void (*file_read)(sObject *, FILE *fp);  \
    void (*debug)(sObject *);

#define Base_Vars \
    sVector p;     \
    int    flags;

#define BaseMethod(O, M) SafeMethod(O, Base, M)

/* Declare Class Structure typedefs */
typedef struct {        \
    Base_Methods        \
} BaseClass;            \
                        \
typedef struct {        \
    Base_Vars           \
} Base;

#define Posn(O) (&( ( (Base*) ((O)->data) )->p))

/* Flags
 * bits 0-7  World library use
 * bits 8-16 application use
 */

/* -------------------------------------------------------------------------- */

extern int check_call(Class *class, char *method, sObject *obj);

extern Symbol_Table *class_symbol_table;
extern int   objects_allocated;
extern Class *new_class(int sizeof_class, char *name, int sizeof_data, Class *superclass);
extern Class *copy_class(Class *class, char *name);
extern int   class_id(Class *c);
extern void  init_all_classes(void);

extern sObject *new_object(Class *class);
extern void destroy_object(sObject *o);
extern void init_objects(void);
extern void setup_objects(void);

extern Symbol_Table *message_symbol_table;
extern sSymbol *add_message(Class *c, char *name, int arity, void *method_address);
extern int new_message(char *name, int arity);
extern IV message(sObject *self, sSymbol *message, ...);
extern void add_method(Class *c, int message, void *method_address);

extern void load_objects(FILE *fp);
extern void save_objects(FILE *fp);
extern void reset_objects(void);

extern int     object_id(sObject *o);
extern sObject *get_object(int id);

extern BaseClass *Base_class;
#endif
