#ifndef flare_h
#define flare_h

#include "wdefs.h"
#include "mobile.h"

/* Class      : Flare
 * Superclass : Mobile
 *
 */

/* Define Class and Instance Templates */

#define Flare_Methods Mobile_Methods \
	sList *active_flares;

#define Flare_Vars Mobile_Vars \
		int32 life;


/* Declare Class Structure typedefs */
typedef struct {
    Flare_Methods
} FlareClass;

typedef struct {
    Flare_Vars
} Flare;


/* Class Externs */
extern FlareClass *Flare_class;
extern void register_flare(void);

extern sObject *drop_flare(sVector *posn, sVector *wv);
#endif
