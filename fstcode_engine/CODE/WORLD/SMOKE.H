#ifndef smoke_h
#define smoke_h

#include "wdefs.h"
#include "mobile.h"

/* Class      : Smoke
 * Superclass : Mobile
 *
 */

/* Define Class and Instance Templates */

#define Smoke_Methods Mobile_Methods

#define Smoke_Vars Mobile_Vars \
  int life, life_time;


/* Declare Class Structure typedefs */
typedef struct {           \
    Smoke_Methods          \
} SmokeClass;              \
                           \
typedef struct {           \
    Smoke_Vars             \
} Smoke;


/* Class Externs */
extern int c_smoke1, c_smoke2, c_smoke3;
extern SmokeClass *Smoke_class;
extern void register_smoke(void);
extern void create_smoke(sVector *p, int life);

#endif
