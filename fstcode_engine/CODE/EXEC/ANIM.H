#ifndef anim_h
#define anim_h

#include "wdefs.h"
#include "shape.h"
#include "cultural.h"

/* Class      : Anim
 * Superclass : Cultural
 *
 */

/* Define Class and Instance Templates */

#define MAX_ANIM 32
#define Anim_Methods Cultural_Methods

#define Anim_Vars Cultural_Vars \
  int dt, next_dt;					\
  int nframes, frame;               \
  Shape_def *animation[MAX_ANIM];


/* Declare Class Structure typedefs */
typedef struct {
    Anim_Methods
} AnimClass;

typedef struct {
    Anim_Vars
} Anim;


/* Class Externs */
extern AnimClass *Anim_class;
extern void register_anim(void);
#endif
