#ifndef bangs_h
#define bangs_h

#include "wdefs.h"
#include "world.h"

/* Class      : Bangs
 * Superclass : World
 *
 */

/* Particles are not implemented as objects for optimisation reasons */
typedef struct Particle {
    struct Particle *next, *prev;
    sVector p, v;
    int life;
} Particle;

/* Define Class and Instance Templates */

#define Bangs_Methods World_Methods

#define Bangs_Vars World_Vars       \
        int       count;            \
        int       *cols;            \
        Particle  *particles;


/* Declare Class Structure typedefs */
typedef struct {           \
    Bangs_Methods          \
} BangsClass;              \
                           \
typedef struct {           \
    Bangs_Vars             \
} Bangs;


/* Class Externs */
extern BangsClass *Bangs_class;
extern BangsClass *GroundBang_class;

extern void register_bangs(void);

extern sObject *big_bang(sVector *pos);
extern sObject *medium_bang(sVector *pos);
extern sObject *small_bang(sVector *pos);
extern sObject *big_ground_bang(sVector *pos);
extern sObject *medium_ground_bang(sVector *pos);
extern sObject *small_ground_bang(sVector *pos);
#endif
