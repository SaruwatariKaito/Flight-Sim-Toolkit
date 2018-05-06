#ifndef ils_h
#define ils_h

#include "wdefs.h"
#include "ndb.h"

/* Class      : ILS
 * Superclass : NDB
 *
 */

/* Define Class and Instance Templates */

#define ILS_Methods NDB_Methods

#define ILS_Vars NDB_Vars

/* Declare Class Structure typedefs */
typedef struct {
    ILS_Methods
} ILSClass;

typedef struct {
    ILS_Vars
} ILS;


/* Class Externs */
extern ILSClass *ILS_class;
extern void register_ils(void);

extern void select_nearest_ils(void);
#endif
