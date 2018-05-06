#ifndef depot_h
#define depot_h

#include "wdefs.h"
#include "cultural.h"

/* Class      : Depot
 * Superclass : Cultural
 *
 */

/* Define Class and Instance Templates */

#define Depot_Methods Cultural_Methods

#define Depot_Vars Cultural_Vars \
  sShape_def *vehicle_shape;							\
  int		in_depot;                               \
  int		truck_strength, shells, fire_delay;		\
  int		predict, aagun;							\
  int       speed, interval, size, sub_int; 		\
  int       count;						 			\
  sList      *paths;


/* Declare Class Structure typedefs */
typedef struct {
    Depot_Methods
} DepotClass;

typedef struct {
    Depot_Vars
} Depot;


/* Class Externs */
extern DepotClass *Depot_class;
extern void register_depot(void);
#endif
