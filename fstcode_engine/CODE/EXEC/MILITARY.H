#ifndef military_h
#define military_h

#include "wdefs.h"
#include "plane.h"
#include "weapons.h"

/* Class      : Military
 * Superclass : Plane
 *
 */

/* Define Class and Instance Templates */

#define Military_Methods Plane_Methods

#define Military_Vars Plane_Vars \
	sShape_def *fast_shape; \
	int rear_gun;	\
	int32 rear_gun_time, target_check_time;	\
	sObject *nearest_target;	\
	int    rearm_data[MAX_WEAPON_TYPE];  \
	sObject *weapon_system;


/* Declare Class Structure typedefs */
typedef struct {
    Military_Methods
} MilitaryClass;

typedef struct {
    Military_Vars
} Military;


/* Class Externs */
extern MilitaryClass *Military_class;
extern void register_military(void);
#endif
