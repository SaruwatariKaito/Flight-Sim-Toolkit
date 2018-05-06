#ifndef eject_h
#define eject_h

#include "wdefs.h"
#include "mobile.h"

/* Class      : Eseat
 * Superclass : Mobile
 *
 */

/* Define Class and Instance Templates */

#define Eseat_Methods Mobile_Methods

#define Eseat_Vars Mobile_Vars


/* Declare Class Structure typedefs */
typedef struct {
    Eseat_Methods
} EseatClass;

typedef struct {
    Eseat_Vars
} Eseat;


/* Class Externs */
extern EseatClass *Eseat_class;
extern void register_eject(void);

extern sObject *start_eject(sVector *p);
#endif
