#ifndef mobile_h
#define mobile_h

#include "wdefs.h"
#include "world.h"

/* Class      : Mobile
 * Superclass : World
 *
 */

/* Define Class and Instance Templates */

#define Mobile_Methods World_Methods

#define Mobile_Vars World_Vars \
    sVector    op;              \
    sVector   dr;              \
    sVector   wv;              \
    long int speed;


/* Declare Class Structure typedefs */
typedef struct {            \
    Mobile_Methods          \
} MobileClass;              \
                            \
typedef struct {            \
    Mobile_Vars             \
} Mobile;


/* Class Externs */
extern MobileClass *Mobile_class;
extern void register_mobile(void);

extern int check_collision(sObject *obj, sShape *s, sVector *p, sVector *r, int fine_check);
#endif
