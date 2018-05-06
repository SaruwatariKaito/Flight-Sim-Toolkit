#ifndef vor_h
#define vor_h

#include "wdefs.h"
#include "ndb.h"

/* Class      : VOR
 * Superclass : NDB
 *
 */

/* Define Class and Instance Templates */

#define VOR_Methods NDB_Methods

#define VOR_Vars NDB_Vars


/* Declare Class Structure typedefs */
typedef struct {
    VOR_Methods
} VORClass;

typedef struct {
    VOR_Vars
} VOR;


/* Class Externs */
extern VORClass *VOR_class;
extern void register_vor(void);

extern sObject *scan_VORs(int n, int up);
#endif
