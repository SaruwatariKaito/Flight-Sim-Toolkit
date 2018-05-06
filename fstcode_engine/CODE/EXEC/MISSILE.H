#ifndef missile_h
#define missile_h

#include "wdefs.h"
#include "rocket.h"

/* Class      : Missile
 * Superclass : Rocket
 *
 */

/* Define Class and Instance Templates */

#define Missile_Methods Rocket_Methods

#define Missile_Vars Rocket_Vars \
	sObject *target;				 \
	int turn_rate;				 \
	int count;					 \
	long int fire_time; 	 	 \
	long int last_range;


/* Declare Class Structure typedefs */
typedef struct {
    Missile_Methods
} MissileClass;

typedef struct {
    Missile_Vars
} Missile;


/* Class Externs */
extern MissileClass *Missile_class;
extern void register_missile(void);

extern void add_sam(sObject *site, sObject *tgt, int kp);
extern void add_ag(sObject *site, Pylon *p, sObject *tgt, int kp);
extern void add_aa(sObject *site, Pylon *p, sObject *tgt, int kp);
#endif
