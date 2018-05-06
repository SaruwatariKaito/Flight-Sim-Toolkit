#ifndef rocket_h
#define rocket_h

#include "wdefs.h"
#include "mobile.h"

#include "defs.h"

/* Class      : Rocket
 * Superclass : Mobile
 *
 */

/* Define Class and Instance Templates */

#define Rocket_Methods Mobile_Methods

#define Rocket_Vars Mobile_Vars     \
		int     kp;					\
		int     fuel;				\
		int     life;				\
        sObject *firer;


/* Declare Class Structure typedefs */
typedef struct {
    Rocket_Methods
} RocketClass;

typedef struct {
    Rocket_Vars
} Rocket;


/* Class Externs */
extern RocketClass *Rocket_class;
extern void register_rocket(void);

extern void fire_rocket(sObject *from, Pylon *p, int kp);
extern void launch_bomb(sObject *from, Pylon *p, int kp);
extern void launch_cluster_bomb(sObject *from, Pylon *p, int kp);
extern void launch_torpedo(sObject *from, Pylon *p, int kp);
extern void calc_rocket_ccip(sObject *self, sVector *ccip, int32 ground_height);
extern void calc_bomb_ccip(sObject *self, sVector *ccip, int32 ground_height);
#endif
