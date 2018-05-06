#ifndef cultural_h
#define cultural_h

#include "wdefs.h"
#include "world.h"

/* Class      : Cultural
 * Superclass : World
 *
 */

/* Define Class and Instance Templates */

#define Cultural_Methods World_Methods

#define Cultural_Vars World_Vars \
    sVector dr;					 \
	sShape_def *alive;		     \
	sShape_def *dead;		     \
	int     init_strength;


/* Declare Class Structure typedefs */
typedef struct {
    Cultural_Methods
} CulturalClass;

typedef struct {
    Cultural_Vars
} Cultural;


/* Class Externs */
extern CulturalClass *Cultural_class;
extern void register_cultural(void);

extern sObject *new_cultural(long int x, long int y, long int z, int dry, sShape_def *sd);

#endif
