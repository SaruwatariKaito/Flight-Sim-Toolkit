#ifndef aircraft_h
#define aircraft_h

#include "wdefs.h"
#include "defs.h"
#include "mobile.h"

/* Class      : Aircraft
 * Superclass : Mobile
 *
 */

/* Define Class and Instance Templates */

#define Aircraft_Methods Mobile_Methods

#define Aircraft_Vars Mobile_Vars \
	int type, airborne, start_strength, aggressive;		\
	int rear_gun, bombs, smoking, smoke_count, smoke_delay;	\
	sObject *target, *rear_target;	\
	Path *path; 				\
	Wpt  *wpt;					\
    int32 last_range, big_speed;	\
	int speed_goal, start_max_speed, max_speed, min_speed, jet;	\
	int32 missile_time, cannon_time, rear_gun_time;	\
	int num_missiles, num_cannon;	\
	int32 next_target_time, next_rear_time;		\
	int danger_count, escape_phase;	\
	int32 escape_time, climb_time;	\
	int effective_roll, roll_goal, roll_couple, max_pitch_rate; \
	int flare_delay;


/* Declare Class Structure typedefs */
typedef struct {
    Aircraft_Methods
} AircraftClass;

typedef struct {
    Aircraft_Vars
} Aircraft;


/* Class Externs */
extern AircraftClass *Aircraft_class;
extern void register_aircraft(void);
extern sObject *create_ac(sShape_def *sd, int jet, int min_speed, int max_speed, Path *p, int type, int rear_gun, int strength, int bombs, int team_flag);
#endif
