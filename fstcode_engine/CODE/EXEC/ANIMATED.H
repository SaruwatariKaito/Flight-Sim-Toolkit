#ifndef animated_h
#define animated_h

#include "wdefs.h"
#include "cultural.h"

/* Class      : Animated
 * Superclass : Cultural
 *
 */

/* Define Class and Instance Templates */

#define MAX_ANIM 32
#define Animated_Methods Cultural_Methods

#define Animated_Vars Cultural_Vars \
  int dt, next_dt;					\
  int nframes, frame;               \
  Shape_def *animation[MAX_ANIM];


/* Declare Class Structure typedefs */
typedef struct {
    Animated_Methods
} AnimatedClass;

typedef struct {
    Animated_Vars
} Animated;


/* Class Externs */
extern AnimatedClass *Animated_class;
extern void register_animated(void);
#endif
