#ifndef drone_h
#define drone_h

#include "wdefs.h"
#include "mobile.h"

/* Class      : Drone
 * Superclass : Mobile
 *
 */

/* Define Class and Instance Templates */

#define Drone_Methods Mobile_Methods

#define Drone_Vars Mobile_Vars \
	sSymbol *cpt_sym;

/* Declare Class Structure typedefs */
typedef struct {
    Drone_Methods
} DroneClass;

typedef struct {
    Drone_Vars
} Drone;


/* Class Externs */
extern DroneClass *Drone_class;
extern void register_drone(void);

extern sObject *new_drone(int32 x, int32 y, int32 z, sShape_def *sd);
#endif
