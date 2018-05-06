#ifndef truck_h
#define truck_h

#include "wdefs.h"
#include "defs.h"
#include "mobile.h"

/* Class      : Truck
 * Superclass : Mobile
 *
 */

/* Define Class and Instance Templates */

#define Truck_Methods Mobile_Methods

#define Truck_Vars Mobile_Vars 	\
	int32 last_range, y_size;	\
	int32 next_target_time, next_fire_time, fire_delay;	\
	int predict, aagun;			\
	sObject *target;				\
	int shells, aggressive;		\
	Path *path; 				\
	Wpt  *wpt;


/* Declare Class Structure typedefs */
typedef struct {
    Truck_Methods
} TruckClass;

typedef struct {
    Truck_Vars
} Truck;


/* Class Externs */
extern TruckClass *Truck_class;
extern void register_truck(void);

extern void create_truck(sShape_def *sd, int speed, Path *p, int flags, int strength, int shells, int fire_delay, int predict, int aagun);
#endif
