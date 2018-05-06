#ifndef hanger_h
#define hanger_h

#include "wdefs.h"
#include "cultural.h"

/* Class      : Hanger
 * Superclass : Cultural
 *
 */

/* Define Class and Instance Templates */

#define Hanger_Methods Cultural_Methods

#define Hanger_Vars Cultural_Vars \
  sShape_def *ac_shape;            \
  sSymbol    *ac_cockpit;          \
  int       behaviour;   		  \
  int       in_hanger;			  \
  int       rear_gun, aircraft_strength, aircraft_bombs;	  \
  int		plane_min_speed, plane_max_speed, plane_turn_rate;	\
  int		plane_jet;	\
  sList      *paths;



/* Declare Class Structure typedefs */
typedef struct {
    Hanger_Methods
} HangerClass;

typedef struct {
    Hanger_Vars
} Hanger;


/* Class Externs */
extern HangerClass *Hanger_class;
extern void register_hanger(void);
#endif
