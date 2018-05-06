#ifndef ndb_h
#define ndb_h

#include "wdefs.h"
#include "cultural.h"

/* Class      : NDB
 * Superclass : Cultural
 *
 */

/* Define Class and Instance Templates */

#define NDB_Methods Cultural_Methods

#define NDB_Vars Cultural_Vars \
	float freq;


/* Declare Class Structure typedefs */
typedef struct {
    NDB_Methods
} NDBClass;

typedef struct {
    NDB_Vars
} NDB;


/* Class Externs */
extern NDBClass *NDB_class;
extern void register_ndb(void);


extern sObject *scan_NDBs(int up);
#endif
