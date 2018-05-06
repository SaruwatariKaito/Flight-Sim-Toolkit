#ifndef bullets_h
#define bullets_h

#include "wdefs.h"
#include "mobile.h"

/* Class      : Bullets
 * Superclass : Mobile
 *
 */

/* Define Class and Instance Templates */

#define Bullets_Methods Mobile_Methods

#define Bullets_Vars Mobile_Vars    \
        int     type;               \
        int     life;               \
        sObject *firer;


/* Declare Class Structure typedefs */
typedef struct {             \
    Bullets_Methods          \
} BulletsClass;              \
                             \
typedef struct {             \
    Bullets_Vars             \
} Bullets;


/* Class Externs */
extern BulletsClass *Bullets_class;
extern void register_bullets(void);

extern sObject *bullet_scoring_firer;
extern void fire_bullet(sObject *o, sVector *aspect, int32 offset, int speed, int life, int flag);

#endif
