#ifndef brick_h
#define brick_h

#include "wdefs.h"
#include "mobile.h"

/* Class      : Brick
 * Superclass : Mobile
 *
 */

/* Define Class and Instance Templates */

#define Brick_Methods Mobile_Methods

#define Brick_Vars Mobile_Vars \
	int32 ground_height;	\
	int autostab_on;	\
	sSymbol *cockpit;


/* Declare Class Structure typedefs */
typedef struct {           \
    Brick_Methods          \
} BrickClass;              \
                           \
typedef struct {           \
    Brick_Vars             \
} Brick;


/* Class Externs */
extern BrickClass *Brick_class;
extern void register_brick(void);

extern sObject *new_brick(int32 x, int32 y, int32 z, sShape_def *sd, sSymbol *cpt);
#endif
