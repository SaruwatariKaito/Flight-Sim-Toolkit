#ifndef plane_h
#define plane_h

#include "wdefs.h"
#include "mobile.h"

/* Class      : Plane
 * Superclass : Mobile
 *
 */

/* Define Class and Instance Templates */

#define Plane_Methods Mobile_Methods

#define Plane_Vars Mobile_Vars \
	int damage;	   			   \
	sSymbol *cockpit;		   \
	sSymbol *model;             \
	sShape_def *default_shape;  \
	sShape_def *gear_down_shape;\
	sVector bdr, bwv;	\
	sVector last_torque, last_force;	\
	int32 fuel;	\
	sVector start_pos;	\
	int start_ry;	\
	void *start_class;	\
	int wing_lift_coeff, wing_incidence, wing_dihedral, stall_angle;	\
	int max_thrust, max_speed, stall_speed, real_weight, weight, start_fuel;	\
	int elevator_max, pitch_stability, rudder_max, yaw_stability, adverse_yaw;	\
	int pitch_control, roll_control, yaw_control;	\
	int pitch_inertia, roll_inertia, yaw_inertia;	\
	int pitch_drag, roll_drag, yaw_drag, x_drag, y_drag;	\
	int clean_drag, air_brake_drag, gear_drag, engine_drag;	\
	int retracts, flaps, airbrakes, biplane, taildrag, ejector;	\
	int num_engines, jet;	\
	sVector left_wing_normal;	\
	sVector right_wing_normal;	\
	int rpm;	\
	int engine_on, autostab_on;	\
	int stalled, flap_angle;	\
	int32 ground_height, eye_height, runway_height;	\
	sObject *runway;		\
	int brakes_on, air_brake_on, gear_down, radalt_on;	\
	int crashed, crash_reason, crash_count, airframe_failure;	\
	int landed, refueled, first_pass, grounded, grounded_time, lowwarn_time;	\
	int hud_fail, rad_fail, nav_fail;	\
	int hyd_fail, engine_fail;


/* Declare Class Structure typedefs */
typedef struct {
    Plane_Methods
} PlaneClass;

typedef struct {
    Plane_Vars
} Plane;


/* Class Externs */
extern PlaneClass *Plane_class;
extern void register_plane(void);
#endif
