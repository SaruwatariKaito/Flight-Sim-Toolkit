/* --------------------------------------------------------------------------
 * EXEC 		Copyright (c) Simis Ltd 1993,994,1995
 * ---			All Rights Reserved
 *
 * File:		plane.c
 * Ver:			2.00
 * Desc:		Player controlable Plane class.
 *
 * This class implmenets a force model update which reads the
 * control.c XXXXXXin variables to control the object.
 *
 * Aircraft model parametarised by file.fmd - read by object
 * file_read method.
 *
 * Inherited by Civil and Military.
 *
 * -------------------------------------------------------------------------- */
/* Player controlled aircraft */

#include <stdlib.h>
#include <math.h>

#include "ground.h"
#include "plane.h"
#include "defs.h"
#include "sound.h"
#include "explo.h"
#include "bangs.h"
#include "eject.h"
#include "cultural.h"

/* Declare Class Pointer */
PlaneClass *Plane_class = NULL;
PlaneClass *Crashing_class = NULL;
PlaneClass *Civil_class = NULL;

static sSymbol *rearm = NULL;
extern sSymbol *gear_control;

extern int pitchin, yawin, rollin, throttlein;

extern sVector wind;

#define TAILDRAG_ANGLE DEG(6)
#define MAX_RPM 256
#define MIN_RPM 16
#define RPM_SHIFT 8
#define FUEL_SHIFT 16
#define AC_MAX_FUEL (776L << FUEL_SHIFT)
#define SOUND_SPEED1 FIXSPD(640)
#define SOUND_SPEED2 FIXSPD(740)
#define SOUND_JUMP (SOUND_SPEED2 - SOUND_SPEED1)
#define MAX_ALT 20000

#define FORCE_SHIFT 10
#define STANDARD_WEIGHT (1 << FORCE_SHIFT)
#define NORMALISE_FORCE(F,W) F = (F << FORCE_SHIFT) / W

static int add_ground_effects(Plane *o, sVector *lv, sVector *force, sVector *torque);

static int slow_atan(int32 y, int32 z)
{
    if (z == 0)
        return((int)(((180.0 * (float)DEG_1) / PI) * atan2((float)y, 1.0)));
    else
        return((int)(((180.0 * (float)DEG_1) / PI) * atan2((float)y, (float)z)));
}



/* --------------------
 * LOCAL set_globals
 *
 * Setup aircraft_XXX variables to provide link
 * to gauges.c
 *
 */
static void set_globals(Plane *o)
{
	int temp;

	aircraft_asi = KNOTS(o->speed);
    aircraft_vsi = o->wv.y * 10;
	aircraft_grounded = o->grounded;
	if (o->radalt_on)
    	aircraft_alt = (int)((((o->p.y - o->eye_height - o->ground_height) >> SPEED_SHIFT) * 19L) / 48L);
	else
		aircraft_alt = (int)((((o->p.y - o->eye_height) >> SPEED_SHIFT) * 19L) / 48L);
    aircraft_roll = (short)o->r.z;
    aircraft_pitch = (short)o->r.x;
    temp = (o->r.y >> ASHIFT);
    if (temp <= 0)
        temp = 4096 - (temp + 4096);
    else
        temp = 4096 - temp;
    aircraft_heading = MULDIV(temp, 360, 4096);
    aircraft_fuel = (o->fuel >> (FUEL_SHIFT - 10)) / o->start_fuel;

	low_fuel = ((o->fuel >> FUEL_SHIFT) < (o->start_fuel >> 3)) ? TRUE : FALSE;

	if (o->rpm == 0)
		aircraft_rpm = 0;
	else
		aircraft_rpm = ((o->rpm * 80) >> RPM_SHIFT) + 20;

    airbrake_on 		= o->air_brake_on;
    brakes_on 			= o->brakes_on;
 	gear_down 			= o->gear_down;
	aircraft_flap_angle = o->flap_angle;

	/* Damage globals */
	aircraft_damage = o->damage;
	engine_fail   	= o->engine_fail;
	hyd_fail      	= o->hyd_fail;
	hud_fail = o->hud_fail;
	radar_fail = o->rad_fail;
	electric_fail 	= o->nav_fail | o->rad_fail | o->hud_fail;
}

static void refuel_plane(sObject *self)
{
    Plane *o;

	o = self->data;
	o->fuel = o->start_fuel << FUEL_SHIFT;
	o->gear_down = TRUE;
	o->refueled = TRUE;
	o->crashed = FALSE;
	o->crash_count = 0;
	o->crash_reason = 0;
	o->damage = 0;
	o->airframe_failure = 0;
	o->hud_fail = FALSE;
	o->rad_fail = FALSE;
	o->nav_fail = FALSE;
	o->hyd_fail = FALSE;
	o->engine_fail = FALSE;
	message(self, rearm);
}

static void restart_plane(sObject *self)
{
    Plane *o;

	o = self->data;
	o->wv = zero;
	o->dr = zero;
	o->bdr = zero;
	o->last_torque = zero;
	o->bwv = zero;
	o->last_force = zero;
	o->speed = 0;
	o->op = o->p = o->start_pos;
	o->r.y = o->start_ry;
	o->r.z = 0;
	if (o->taildrag)
		o->r.x = TAILDRAG_ANGLE;
	else
		o->r.x = 0;
	refuel_plane(self);
	o->first_pass = TRUE;
	o->gear_down = TRUE;
	o->engine_on = TRUE;
	o->flap_angle = 0;
	throttlein = 0;
	o->rpm = 0;
	o->grounded = TRUE;
	o->grounded_time = 0;
	o->lowwarn_time = 0;
	if (o->gear_down_shape)
		o->shape = o->gear_down_shape;
	if (ejector_symbol->value) {
		remove_object(ejector_symbol->value);
		ejector_symbol->value = NULL;
	}
	control_set_viewer(Player);
	ground_view_changed(viewer);
}

static int32 set_runway_height(sObject *self)
{
    Plane *o;
	sList *objects, *l;
	sObject *test;
	World *w;
	sShape_def *sd;
	sShape *s;

	o = self->data;
	o->runway = NULL;
	o->runway_height = o->ground_height;
	objects = get_tp_objects(self);
	if (objects) {
		for (l = objects->next; l != objects; l = l->next) {
			test = l->item;
			w = test->data;
			if (w->flags & RUNWAY_TYPE) {
				sd = w->shape;
				while (sd->down) {
					sd = sd->down;
				}
				s = sd->s;
				if (over_shape(&o->p, s, &w->p, &w->r)) {
					o->runway_height += s->max_size.y << s->scale;
					o->runway = test;
				}
			}
		}
	}
	return o->runway_height;
}

#define LOWWARN M(100)
#define ALT_TOP 13200
/* Air density halves every 20000 ft (6600 M) */
/* Returns fraction relative to ALT_TOP */
static int calc_alt_factor(int32 alt)
{
	int alt_factor;

	if (alt > 19800L) {
        alt_factor = ALT_TOP >> 3;
	} else if (alt < 6600L) {
        alt_factor = 6600 - (int)alt;
        alt_factor = MULDIV(alt_factor, 6600, 6600);
		alt_factor += ALT_TOP >> 1;
	} else if (alt < 13200L) {
        alt_factor = 13200 - (int)alt;
        alt_factor = MULDIV(alt_factor, 3300, 6600);
		alt_factor += ALT_TOP >> 2;
	} else if (alt < 19800L) {
        alt_factor = 19800 - (int)alt;
        alt_factor = MULDIV(alt_factor, 1650, 6600);
		alt_factor += ALT_TOP >> 3;
    }
	return alt_factor;
}

static void set_aircraft_rpm(Plane *o)
{
	int dr, rpm_goal;

	if (throttlein > MAX_RPM)
        throttlein = MAX_RPM;
    else if (throttlein < MIN_RPM)
        throttlein = MIN_RPM;
    if (o->engine_on && (o->fuel > 0)) {
		if (o->engine_fail)
			rpm_goal = MAX_RPM >> 2;
		else
        	rpm_goal = throttlein;
    } else {
        rpm_goal = 0;
        o->engine_on = FALSE;
	}
    if (o->rpm != rpm_goal) {
		if (o->rpm < rpm_goal) {
			dr = rpm_goal - o->rpm;
			if (dr > frame_time)
				dr = frame_time;
    		o->rpm += dr;
		} else {
			dr = o->rpm - rpm_goal;
			if (dr > frame_time)
				dr = frame_time;
    		o->rpm -= dr;
		}
    }
    if (o->rpm > 0)
		o->fuel -= ((o->weight >> 7) + 1) * ((o->rpm >> 5) + 4) * frame_time;
}

static int calc_lift(Plane *o, int aoa, int speed_2)
{
	int lift_coeff, lift;

	if (ABS(aoa) < o->stall_angle) {
		if (aoa >= 0) {
			lift_coeff = o->wing_lift_coeff;
			if (o->flap_angle > 0)
				lift_coeff += MULDIV(lift_coeff, o->flap_angle, 90);
			if (o->p.y < (o->runway_height + M(3)))
				lift_coeff += lift_coeff >> 3;
		} else {
			lift_coeff = o->wing_lift_coeff >> 1;
		}
		lift = (int)(((int32)speed_2 * MUL(aoa, lift_coeff)) >> 14);
	} else if (ABS(aoa) < (2 * o->stall_angle)) {
		if (aoa > 0)
			lift = (int)(((int32)speed_2 * MUL(DEG(4), o->wing_lift_coeff)) >> 14);
		else
			lift = (int)(((int32)speed_2 * MUL(DEG(-4), o->wing_lift_coeff)) >> 14);
	} else {
		if (aoa > 0)
			lift = (int)(((int32)speed_2 * MUL(DEG(2), o->wing_lift_coeff)) >> 14);
		else
			lift = (int)(((int32)speed_2 * MUL(DEG(-2), o->wing_lift_coeff)) >> 14);
	}
	return lift;
}

/* Realistic aircraft model */
void update_plane(sObject *self)
{
    int fusrx, fusry, aoa_l, aoa_r, aoa, tail_angle, rudder_angle;
    int thrust, lift, lift_l, lift_r, weight, drag_z;
    int speed_2, speed, alt_factor, slip_stream, control_speed;
    int speed_x2, speed_y2, speed_z2, abs_speed_2;
    int32 alt, desired_torque, max_torque;
    Plane *o;
    sObject *obj;
    sVector v, torque, force;
    sVector lv, r;
    Transdata trans;

	if (self != Player) {
		remove_object(self);
		return;
	}

	o = self->data;

    setup_trans(&o->r, &trans);

	o->ground_height = get_terrain_height(o->p.x, o->p.z);
	set_runway_height(self);

	if (o->first_pass) {
		o->first_pass = FALSE;
		if (o->runway) {
        	o->op.y = o->p.y = o->runway_height + o->eye_height;
			o->start_pos.y = o->p.y;
		} else {
			o->landed = FALSE;
			o->grounded = FALSE;
			message(self, gear_control);
			throttlein = MAX_RPM;
			o->speed = (o->max_speed * 3) / 4;
			r.x = 0; r.y = 0; r.z = o->speed;
			rot3d(&r, &o->wv, &trans);
			o->bwv.x = o->wv.x << FORCE_SHIFT;
			o->bwv.y = o->wv.y << FORCE_SHIFT;
			o->bwv.z = o->wv.z << FORCE_SHIFT;
		}
	}

	speed 		= o->speed;
    alt 		= o->p.y >> (SPEED_SHIFT + 3);
    alt_factor 	= calc_alt_factor(alt);
	set_aircraft_rpm(o);

    thrust = (int)(MUL(o->rpm, o->max_thrust) >> RPM_SHIFT);
	if (o->jet)
		thrust += MULDIV(thrust, speed, FIXSPD(1400));
	else
		thrust -= MULDIV(thrust, speed, o->max_speed * 2);

    if (speed > SOUND_SPEED2)
        thrust -= MULDIV(thrust, speed - SOUND_SPEED2, FIXSPD(750));

    thrust = MULDIV(thrust, alt_factor, ALT_TOP);
    if (alt > MAX_ALT) {
        thrust = 0;
        o->rpm = 0;
    }

    invrot3d(&o->wv, &lv, &trans);
	aircraft_slip = lv.x;

    fusrx = slow_atan(-lv.y, lv.z);
    fusry = slow_atan(lv.x, lv.z);
	r = lv;
	normalise(&r);
	aoa_l = (MUL(r.x, o->left_wing_normal.x) + MUL(r.y, o->left_wing_normal.y) + MUL(r.z, o->left_wing_normal.z)) >> SSHIFT;
	aoa_l = ACOS(aoa_l);
	aoa_r = (MUL(r.x, o->right_wing_normal.x) + MUL(r.y, o->right_wing_normal.y) + MUL(r.z, o->right_wing_normal.z)) >> SSHIFT;
	aoa_r = ACOS(aoa_r);
    aoa = SR(aoa_l + aoa_r, 1);
    aircraft_aoa = aoa / (DEG_1 / 10);

	if (lv.z > 0)
		speed_z2 = (int)SR(MUL(lv.z, lv.z), 10);
	else
		speed_z2 = -(int)SR(MUL(lv.z, lv.z), 10);
    speed_z2 = MULDIV(speed_z2, alt_factor, ALT_TOP);
	speed_2 = (int)((MUL(lv.x, lv.x) + MUL(lv.y, lv.y)) >> 10);
    speed_2 = MULDIV(speed_2, alt_factor, ALT_TOP) + speed_z2;
	abs_speed_2 = ABS(speed_2);
	speed_x2 = (int)SR(MUL(speed_2, SIN(fusry)), SSHIFT);
    speed_y2 = (int)SR(MUL(-(int)speed_2, SIN(fusrx)), SSHIFT);
	if (o->jet) {
		slip_stream = 0;
	} else {
		slip_stream = (o->rpm * o->rpm * o->max_speed) / FIXSPD(50000);
	}

	drag_z = o->clean_drag;
    drag_z += (int)(MUL((MAX_RPM - o->rpm), o->engine_drag) >> RPM_SHIFT);
    if (o->gear_down)
        drag_z += o->gear_drag;
    if (o->air_brake_on)
        drag_z += o->air_brake_drag;
    if (o->flap_angle > 0)
        drag_z += MULDIV(o->clean_drag, o->flap_angle, 90);
    if (speed > SOUND_SPEED1) {
        if (speed < SOUND_SPEED2)
            drag_z += MULDIV(drag_z, speed - SOUND_SPEED1, SOUND_JUMP);
        else
            drag_z <<= 1;
    }
	if (ABS(aoa) < o->stall_angle) {
		aircraft_stalled = FALSE;
	} else if (ABS(aoa) < (2 * o->stall_angle)) {
		drag_z <<= 1;
		aircraft_stalled = TRUE;
	} else {
		drag_z <<= 2;
		aircraft_stalled = TRUE;
	}

	if (o->airframe_failure == 1)
		lift_l = 0;
	else
		lift_l = calc_lift(o, aoa_l, speed_z2 - SR(o->dr.y, 1));
	if (o->airframe_failure == 2)
		lift_r = 0;
	else
		lift_r = calc_lift(o, aoa_r, speed_z2 + SR(o->dr.y, 1));
	lift = lift_l + lift_r;
	weight = o->weight + (o->fuel >> FUEL_SHIFT);
	if (lift > 0)
		aircraft_g = MULDIV(lift + (STANDARD_WEIGHT / 20), 10, weight);
	else
		aircraft_g = MULDIV(lift - (STANDARD_WEIGHT / 20), 10, weight);

	if (o->jet)
		control_speed = o->stall_speed << 1;
	else
		control_speed = o->stall_speed + (o->stall_speed >> 1);
	/* 540 is max input */
    tail_angle = MULDIV(pitchin, o->elevator_max, 540);
	rudder_angle = MULDIV(yawin, o->rudder_max, 540);
	rudder_angle += MULDIV(rollin, o->adverse_yaw * 50, 540);
	if (o->speed > control_speed) {
		if (o->jet) {
    		tail_angle = MULDIV(tail_angle, o->max_speed, o->max_speed + ((o->speed - control_speed) * 3));
    		rudder_angle = MULDIV(rudder_angle, o->max_speed, o->max_speed + ((o->speed - control_speed) * 3));
		} else {
    		tail_angle = MULDIV(tail_angle, o->max_speed, o->max_speed + o->speed - control_speed);
    		rudder_angle = MULDIV(rudder_angle, o->max_speed, o->max_speed + o->speed - control_speed);
		}
	}
/*	vector_number(tail_angle / DEG_1, 100, 6, WHITE);
	vector_number(rudder_angle / DEG_1, 80, 6, WHITE);*/
	torque.x = (int32)MULDIV(tail_angle - fusrx, speed_2, 4000);
	torque.x += (int32)MULDIV(tail_angle, slip_stream, 4000);
	torque.x = (int32)MULDIV(torque.x, o->pitch_stability, 400);
	torque.x = (int32)MULDIV(torque.x, o->pitch_control, 40);

	torque.y = (int32)MULDIV(rudder_angle - fusry, speed_2, 1500);
	torque.y += (int32)MULDIV(rudder_angle, slip_stream, 1000);
	torque.y = (int32)MULDIV(torque.y, o->yaw_stability, 400);
	torque.y = (int32)MULDIV(torque.y, o->yaw_control, 30);

    desired_torque = MULDIV(rollin, o->roll_control, 10);
	if (o->jet)
		max_torque = MULDIV(rollin, speed_z2, 120);
	else
		max_torque = MULDIV(rollin, speed_z2, 120);
    if (desired_torque >= 0) {
        torque.z = MIN(desired_torque, max_torque);
    } else {
        torque.z = MAX(desired_torque, max_torque);
    }
/*	vector_number(torque.x, 240, 6, WHITE);
	vector_number(torque.y, 270, 6, WHITE);
	vector_number(torque.z, 300, 6, WHITE);*/

	if (aircraft_stalled)
		torque.x += speed_y2;
/*	torque.y += SR(MUL(rollin, o->adverse_yaw) * (int32)speed_z2, 16);*/
	torque.y += SR((lift_l - lift_r), 6);
	torque.z += (lift_l - lift_r) / 2;
/*	if (o->wing_dihedral != 0) {
		if (ABS((short)o->r.z) <= o->wing_dihedral)
			torque.z -= MULDIV(SR((short)o->r.z, 2), o->wing_dihedral, DEG(30));
	}*/

	torque.x -= MULDIV(o->pitch_drag, o->dr.x, 15);
	torque.y -= MULDIV(o->yaw_drag, o->dr.y, 15);
	torque.z -= MULDIV(o->roll_drag, o->dr.z, 15);

	/* Force in aircraft coords */
    force.x = SR(MUL(-fusry, speed_z2), 14);
    force.y = SR(MUL(lift, COS(aoa)), SSHIFT);
    force.z = thrust - SR(MUL(lift, SIN(aoa)), SSHIFT);

    force.x -= MULDIV(o->x_drag, speed_x2, 300);
    force.y -= MULDIV(o->y_drag, speed_y2, 300);
    force.z -= MULDIV(drag_z, speed_z2, 300);

	add_ground_effects(o, &lv, &force, &torque);

	/* Force in world coords */
    rot3d(&force, &force, &trans);
    force.y -= weight;

    /* Torques in aircraft axes */

    /*
	 * v.y = ((torque.y + o->last_torque.y) / 2) * (int32)frame_time;
    v.z = ((torque.z + o->last_torque.z) / 2) * (int32)frame_time;
	o->last_torque = torque;
	 */

    v.x = torque.x * (int32)frame_time;
    v.y = torque.y * (int32)frame_time;
    v.z = torque.z * (int32)frame_time;
    o->bdr.x += v.x;
    o->bdr.y += v.y;
    o->bdr.z += v.z;
    o->dr.x = DIV(o->bdr.x, o->pitch_inertia);
    o->dr.y = DIV(o->bdr.y, o->yaw_inertia);
    o->dr.z = DIV(o->bdr.z, o->roll_inertia);

    r.x = (int)MUL(o->dr.x, frame_time);
    r.y = (int)MUL(o->dr.y, frame_time);
    r.z = (int)MUL(o->dr.z, frame_time);
    ac_axis_rotate(&o->r, &r);

    /* Acceleration, Velocity in world coords */
	v.x = force.x * (int32)frame_time * 2;
    v.y = force.y * (int32)frame_time * 2;
    v.z = force.z * (int32)frame_time * 2;
    o->bwv.x += v.x;
    o->bwv.y += v.y;
    o->bwv.z += v.z;
    o->wv.x = DIV(o->bwv.x, weight);
    o->wv.y = DIV(o->bwv.y, weight);
    o->wv.z = DIV(o->bwv.z, weight);

    /* Position in world coords */
    o->op = o->p;
    o->p.x += MUL(o->wv.x, frame_time);
    o->p.y += MUL(o->wv.y, frame_time);
    o->p.z += MUL(o->wv.z, frame_time);
	if (!o->grounded) {
		o->p.x += MUL(wind.x, frame_time);
		o->p.y += MUL(wind.y, frame_time);
		o->p.z += MUL(wind.z, frame_time);
	}
    o->speed = lv.z;

	if (o->p.y < LOWWARN && !o->gear_down) {
		if ((total_time - o->lowwarn_time) > 50) {
			o->lowwarn_time = total_time;
			lowwarn_sound();
		}
	}
	if ((o->p.y <= (o->runway_height + o->eye_height))) {
		if (!o->grounded && (total_time - o->grounded_time) > SECS(5)) {
			o->grounded_time = total_time;
			landing_sound();
		}
		o->grounded = TRUE;
		aircraft_g = 10;
		/* Slow speed */
		if (ABS(lv.z) <= 20) {
			o->landed = TRUE;
			if (!o->refueled)
				refuel_plane(self);
        	o->op.y = o->p.y = o->runway_height + o->eye_height;
			if (o->rpm <= MIN_RPM) {
				o->bwv.x = o->wv.x = 0;
				o->bwv.z = o->wv.z = 0;
			}
			o->bwv.y = o->wv.y = 0;
			o->last_force = zero;
			o->last_torque = zero;
			aircraft_vsi = 0;

			force.y = 0;
			lv.y = 0;

			o->r.z = 0;
			if (o->taildrag)
				o->r.x = TAILDRAG_ANGLE;
			else
				o->r.x = 0;
		}
	} else {
		o->grounded = FALSE;
		o->landed = FALSE;
		o->refueled = FALSE;
	}

    set_globals(o);
	if ((o->crashed) || (o->landed && (o->runway_height == 0))) {
		o->crash_reason = 1;
		SafeMethod(self, World, kill)(self);
	}
}

static int get_ground_intersection(sVector *p, sVector *v)
{
	int contact;
	int32 gh;

	contact = FALSE;
	gh = get_terrain_height(p->x, p->z);
	v->x = 0;
	v->z = 0;
	if (p->y >= gh) {
		v->y = 0;
	} else {
		v->y = gh - p->y;
		contact = TRUE;
	}
	return contact;
}

static int get_runway_intersection(Plane *o, sVector *p, sVector *v)
{
	int contact;
	int32 gh;

	contact = FALSE;
	gh = o->runway_height;
	v->x = 0;
	v->z = 0;
	if (p->y >= gh) {
		v->y = 0;
	} else {
		v->y = gh - p->y;
		contact = TRUE;
	}
	return contact;
}

#define MAIN_FORCE 500
#define NOSE_FORCE 700
#define TAIL_FORCE 700
static int calc_force(Plane *o, sVector *p, sVector *lv, sVector *force, int size)
{
	int contact;
	int32 max_force;
	sVector v;

	if (frame_time > 10)
		size *= 2;
	if (o->runway)
		contact = get_runway_intersection(o, p, &v);
	else
		contact = get_ground_intersection(p, &v);
	if (contact) {
		force->x = (v.x * o->weight) / size;
		force->z = (v.z * o->weight) / size;
		force->y = ((v.y * o->weight) - (lv->y * o->weight)) / size;
    	if (ABS((short)o->r.z) > DEG(30))
			max_force = o->weight / 2;
    	else if ((short)o->r.x > DEG(30))
			max_force = o->weight / 2;
    	else if ((short)o->r.x < DEG(-10))
			max_force = o->weight / 2;
		else if (o->runway_height == 0)
			max_force = o->weight / 2;
		else if (o->gear_down)
			max_force = o->weight * 8;
		else
			max_force = o->weight * 2;
		if ((ABS(force->y) > max_force) || (ABS(force->x) > max_force) || (ABS(force->z) > max_force)) {
			o->crash_count++;
			if (o->crash_count > 1) {
				o->crashed = TRUE;
				o->crash_count = 0;
			}
		} else {
			o->crash_count = 0;
		}
	} else {
		*force = zero;
		o->crash_count = 0;
	}
	return contact;
}

static int add_ground_effects(Plane *o, sVector *lv, sVector *force, sVector *torque)
{
	int contact;
	sVector p, f;
	Transdata trans;

    setup_trans(&o->r, &trans);
	contact = 0;

	if (o->taildrag) {
		/* Tail wheel */
		p.x = MM(0);
		p.y = -(o->eye_height - MM(700));
		p.z = MM(-4000);
		rot3d(&p, &p, &trans);
		p.x += o->p.x;
		p.y += o->p.y;
		p.z += o->p.z;
		contact |= calc_force(o, &p, lv, &f, TAIL_FORCE);
		if (contact) {
			force->x += f.x;
			force->y += f.y;
			force->z += f.z;
			torque->x -= f.y / 2;
			torque->y += yawin / 2;
		}

		/* Left main wheel */
		p.x = MM(-3000);
		p.y = -o->eye_height;
		p.z = MM(500);
		rot3d(&p, &p, &trans);
		p.x += o->p.x;
		p.y += o->p.y;
		p.z += o->p.z;
		contact |= calc_force(o, &p, lv, &f, MAIN_FORCE);
		if (contact) {
			force->x += f.x;
			force->y += f.y;
			force->z += f.z;
			torque->x += f.y / 30;
			torque->z += f.y / 2;
		}

		/* Right main wheel */
		p.x = MM(3000);
		p.y = -o->eye_height;
		p.z = MM(500);
		rot3d(&p, &p, &trans);
		p.x += o->p.x;
		p.y += o->p.y;
		p.z += o->p.z;
		contact |= calc_force(o, &p, lv, &f, MAIN_FORCE);
		if (contact) {
			force->x += f.x;
			force->y += f.y;
			force->z += f.z;
			torque->x += f.y / 30;
			torque->z -= f.y / 2;
		}
	} else {
		/* Nose wheel */
		p.x = MM(0);
		p.y = -o->eye_height;
		p.z = MM(3000);
		rot3d(&p, &p, &trans);
		p.x += o->p.x;
		p.y += o->p.y;
		p.z += o->p.z;
		contact |= calc_force(o, &p, lv, &f, NOSE_FORCE);
		if (contact) {
			force->x += f.x;
			force->y += f.y;
			force->z += f.z;
			torque->x += f.y / 3;
			torque->y += yawin;
		}

		/* Left main wheel */
		p.x = MM(-3000);
		p.y = -o->eye_height;
		p.z = MM(-1000);
		rot3d(&p, &p, &trans);
		p.x += o->p.x;
		p.y += o->p.y;
		p.z += o->p.z;
		contact |= calc_force(o, &p, lv, &f, MAIN_FORCE);
		if (contact) {
			force->x += f.x;
			force->y += f.y;
			force->z += f.z;
			torque->x -= f.y / 15;
			torque->z += f.y / 2;
		}

		/* Right main wheel */
		p.x = MM(3000);
		p.y = -o->eye_height;
		p.z = MM(-1000);
		rot3d(&p, &p, &trans);
		p.x += o->p.x;
		p.y += o->p.y;
		p.z += o->p.z;
		contact |= calc_force(o, &p, lv, &f, MAIN_FORCE);
		if (contact) {
			force->x += f.x;
			force->y += f.y;
			force->z += f.z;
			torque->x -= f.y / 15;
			torque->z -= f.y / 2;
		}
	}

	if (contact) {
		force->x -= (lv->x * o->weight) / 50;
    	if (ABS((short)o->r.z) > DEG(30)) {
			torque->y -= (short)o->r.z / 5;
		}
		if (o->gear_down) {
			if (o->brakes_on) {
				force->z -= (lv->z * o->weight) / 400;
				if ((ABS(lv->z) > 10) && (ABS(lv->z) < 200))
					force->z -= o->weight / 4;
			} else {
				if (lv->z < 200)
					force->z -= (lv->z * o->weight) / 4000;
				else
					force->z -= (lv->z * o->weight) / 20000;
			}
		} else {
			force->z -= (lv->z * o->weight) / 200;
			if ((ABS(lv->z) > 10) && (ABS(lv->z) < 200))
				force->z -= o->weight / 2;
		}
	}
	return contact;
}

/* Method used to read in an inital world state */
static void calc_wing_normals(Plane *o)
{
	sVector r;
	Transdata trans;

	r.x = o->wing_incidence;
	r.y = 0;
	r.z = o->wing_dihedral;
	setup_trans(&r, &trans);
	rot3d(&o->left_wing_normal, &o->left_wing_normal, &trans);
	r.z = -o->wing_dihedral;
	setup_trans(&r, &trans);
	rot3d(&o->right_wing_normal, &o->right_wing_normal, &trans);
}

#define FIN_BASE 200
#define NUM_POINTS 19
static sVector2 points[NUM_POINTS];

static int triangle_area(sVector2 *pts, int p1, int p2, int p3)
{
	int area;
	double dx, dy, s, a, b, c;

	dx = (double)pts[p2].x - (double)pts[p1].x;
	dy = (double)pts[p2].y - (double)pts[p1].y;
	a = sqrt((dx * dx) + (dy * dy));
	dx = (double)pts[p3].x - (double)pts[p2].x;
	dy = (double)pts[p3].y - (double)pts[p2].y;
	b = sqrt((dx * dx) + (dy * dy));
	dx = (double)pts[p1].x - (double)pts[p3].x;
	dy = (double)pts[p1].y - (double)pts[p3].y;
	c = sqrt((dx * dx) + (dy * dy));
	s = (a + b + c) / 2.0;
	area = (int)(sqrt(s * (s - a) * (s - b) * (s - c)));
	return area;
}

static int wing_area(sVector2 *pts)
{
	int tri1, tri2;

	tri1 = triangle_area(pts, 1, 7, 2);
	tri2 = triangle_area(pts, 8, 2, 7);
	return(tri1 + tri2);
}

static int tail_area(sVector2 *pts)
{
	int tri1, tri2;

	tri1 = triangle_area(pts, 5, 11, 6);
	tri2 = triangle_area(pts, 12, 6, 11);
	return(tri1 + tri2);
}

static int fin_area(sVector2 *pts)
{
	int tri1, tri2;

	tri1 = triangle_area(pts, 18, 15, 17);
	tri2 = triangle_area(pts, 15, 18, 16);
	return(tri1 + tri2);
}

static int fin_height(sVector2 *pts)
{
	if (pts[15].y < pts[16].y)
		return (FIN_BASE - pts[15].y);
	return (FIN_BASE - pts[16].y);
}

static int tail_span(sVector2 *pts)
{
	if (pts[11].y > pts[12].y)
		return pts[11].y;
	return pts[12].y;
}

static int tail_distance(sVector2 *pts)
{
	if (pts[5].x > 0)
		return pts[5].x;
	return -pts[5].x;
}

static int wing_span(sVector2 *pts)
{
	if (pts[7].y > pts[8].y)
		return pts[7].y;
	return pts[8].y;
}

static int root_chord(sVector2 *pts)
{
	return(pts[1].x- pts[2].x);
}

static int aspect_ratio(sVector2 *pts)
{
	int span;

	span = wing_span(pts);
	return ((10 * span * span) / wing_area(pts));
}

static int calc_lift_coeff(sVector2 *pts)
{
	int wa, lc;

	wa = wing_area(pts) / 20;
	lc = (wa * 4) + (wa * wing_span(pts)) / root_chord(pts);
	return (lc / 2);
}

int calc_stall_speed(sVector2 *points, float wing_eff, float stall_angle, int real_weight)
{
	int speed, lift_coeff;
	float weight, lift;

	lift_coeff = (int)((float)calc_lift_coeff(points) * wing_eff);
	NORMALISE_FORCE(lift_coeff, real_weight);
	lift = (float)lift_coeff * (stall_angle - 1.0);
	weight = (float)(STANDARD_WEIGHT + (STANDARD_WEIGHT / 5));
	speed = (int)sqrt((double)((weight * 1200.0) / lift));
	return FIXSPD(speed);
}

int calc_top_speed(sVector2 *points, int real_thrust, float drag_factor, int real_weight)
{
	int speed, drag;

	NORMALISE_FORCE(real_thrust, real_weight);
	drag = (int)((float)(wing_span(points) + tail_span(points) + fin_height(points)) * drag_factor);
	NORMALISE_FORCE(drag, real_weight);
	speed = (int)sqrt((double)(real_thrust * 4096) / (double)drag);
	return FIXSPD(speed);
}


static void save_params(char *file, Plane *o);

void read_model_params(char *file, Plane *o)
{
    FILE *fp;
	int version, i;
	float airframe_d, uc_d, airbrake_d;
	float dihedral, incidence, stall, wing_eff;
	float c_power, roll_i, pitch_i;
	int units, grid, centre;
	sVector2 v;
	char s[60];

	fp = fopen(file, "r");
	if (fp != NULL) {
		fscanf(fp, "%s %d\n", s, &version);
		if (version != 1) {
			stop("Model file version is not 1");
		}
		fscanf(fp, "%s %d\n", s, &o->real_weight);
		fscanf(fp, "%s %f\n", s, &airframe_d);
		fscanf(fp, "%s %f\n", s, &uc_d);
		fscanf(fp, "%s %f\n", s, &airbrake_d);
		fscanf(fp, "%s %f\n", s, &dihedral);
		fscanf(fp, "%s %f\n", s, &incidence);
		fscanf(fp, "%s %f\n", s, &stall);
		fscanf(fp, "%s %f\n", s, &wing_eff);
		fscanf(fp, "%s %f\n", s, &c_power);
		fscanf(fp, "%s %f\n", s, &roll_i);
		fscanf(fp, "%s %f\n", s, &pitch_i);

		fscanf(fp, "%s %d\n", s, &o->retracts);
		fscanf(fp, "%s %d\n", s, &o->flaps);
		fscanf(fp, "%s %d\n", s, &o->airbrakes);
		fscanf(fp, "%s %d\n", s, &o->biplane);
		fscanf(fp, "%s %d\n", s, &o->taildrag);
		fscanf(fp, "%s %d\n", s, &o->ejector);

		fscanf(fp, "%s %d\n", s, &o->max_thrust);
		fscanf(fp, "%s %d\n", s, &o->start_fuel);
		fscanf(fp, "%s %d\n", s, &o->num_engines);
		fscanf(fp, "%s %d\n", s, &o->jet);

		fscanf(fp, "%s %d\n", s, &units);
		fscanf(fp, "%s %d\n", s, &grid);
		fscanf(fp, "%s %d\n", s, &centre);

		for (i = 0; i < NUM_POINTS; i++) {
			fscanf(fp, "%d %d\n", &v.x, &v.y);
			points[i].x = (v.x * 10) / units;
			points[i].y = (v.y * 10) / units;
		}
		fclose(fp);

		o->weight = STANDARD_WEIGHT;

		o->wing_lift_coeff = (int)((float)calc_lift_coeff(points) * wing_eff);
		o->wing_incidence = (int)(incidence * (float)DEG_1);
		o->wing_dihedral = (int)(dihedral * (float)DEG_1);
		o->stall_angle = (int)(stall * (float)DEG_1);;
		o->adverse_yaw = aspect_ratio(points) / 5;
		o->elevator_max = DEG(35);
		o->rudder_max = DEG(20);

		o->roll_control = (int)((float)wing_span(points) * c_power) * 2;
		o->pitch_control = tail_area(points) / 3;
		o->yaw_control = fin_area(points) / 5;

		o->pitch_stability = (tail_area(points) * tail_distance(points)) / 40;
		o->yaw_stability = (fin_area(points) * tail_distance(points)) / 60;

		o->pitch_inertia = (int)((float)((o->real_weight * tail_distance(points)) / 900) * pitch_i);
		o->yaw_inertia = (int)((float)((o->real_weight * wing_span(points)) / 600) * roll_i);
		o->roll_inertia = (int)((float)((o->real_weight * wing_span(points)) / 700) * roll_i);

		o->pitch_drag = tail_area(points) / 2;
		o->yaw_drag = (fin_area(points) / 3) + (wing_span(points) / 2);
		o->roll_drag = wing_area(points) / 10;

		o->x_drag = fin_area(points);
		o->y_drag = wing_area(points) + tail_area(points);
		o->clean_drag = (int)((float)(wing_span(points) + tail_span(points) + fin_height(points)) * airframe_d);
		o->air_brake_drag = (int)((float)(o->clean_drag / 2) * airbrake_d);
		o->gear_drag = (int)((float)(o->clean_drag / 2) * uc_d);

		if (o->jet) {
			o->engine_drag = o->clean_drag / 2;
		} else {
			o->engine_drag = o->clean_drag / 5;
		}
		if (o->biplane) {
			o->wing_lift_coeff += o->wing_lift_coeff / 2;
			o->clean_drag += o->clean_drag / 2;
		}

		o->max_speed = calc_top_speed(points, o->max_thrust, airframe_d, o->real_weight);
		o->stall_speed = calc_stall_speed(points, wing_eff, stall, o->real_weight);

		NORMALISE_FORCE(o->max_thrust, o->real_weight);
		NORMALISE_FORCE(o->start_fuel, o->real_weight);
		NORMALISE_FORCE(o->wing_lift_coeff, o->real_weight);
		NORMALISE_FORCE(o->roll_control, o->real_weight);
		NORMALISE_FORCE(o->pitch_control, o->real_weight);
		NORMALISE_FORCE(o->yaw_control, o->real_weight);
		NORMALISE_FORCE(o->pitch_stability, o->real_weight);
		NORMALISE_FORCE(o->yaw_stability, o->real_weight);
		NORMALISE_FORCE(o->pitch_inertia, o->real_weight);
		NORMALISE_FORCE(o->yaw_inertia, o->real_weight);
		NORMALISE_FORCE(o->roll_inertia, o->real_weight);
		NORMALISE_FORCE(o->pitch_drag, o->real_weight);
		NORMALISE_FORCE(o->yaw_drag, o->real_weight);
		NORMALISE_FORCE(o->roll_drag, o->real_weight);
		NORMALISE_FORCE(o->x_drag, o->real_weight);
		NORMALISE_FORCE(o->y_drag, o->real_weight);
		NORMALISE_FORCE(o->clean_drag, o->real_weight);
		NORMALISE_FORCE(o->air_brake_drag, o->real_weight);
		NORMALISE_FORCE(o->gear_drag, o->real_weight);

	} else {
		o->wing_lift_coeff = 40;
		o->wing_incidence = 150;
		o->wing_dihedral = DEG(1);
		o->adverse_yaw = 5;
		o->stall_angle = DEG(15);
		o->max_thrust = 800;
		o->max_speed = FIXSPD(250);
		o->stall_speed = FIXSPD(50);
		o->real_weight = 3800;
		o->weight = 1042;
		o->start_fuel = 100;
		o->elevator_max = DEG(35);
		o->rudder_max = DEG(25);
		o->pitch_control = 17;
		o->roll_control = 30;
		o->yaw_control = 18;
		o->yaw_stability = 80;
		o->pitch_stability = 70;
		o->pitch_inertia = 50;
		o->roll_inertia = 75;
		o->yaw_inertia = 80;
		o->pitch_drag = 25;
		o->roll_drag = 25;
		o->yaw_drag = 50;
		o->x_drag = 100;
		o->y_drag = 300;
		o->clean_drag = 50;
		o->air_brake_drag = 20;
		o->gear_drag = 25;
		o->engine_drag = 40;
		o->jet = FALSE;
	}
	calc_wing_normals(o);
	o->fuel = o->start_fuel << FUEL_SHIFT;
	o->start_pos = o->p;
	o->start_ry = o->r.y;
	save_params("PARAMS", o);
}

static void save_params(char *file, Plane *o)
{
	FILE *fp;

	fp = fopen(file, "w");
	if (fp != NULL) {
		fprintf(fp, "wing_lift_coeff %d\n", o->wing_lift_coeff);
		fprintf(fp, "wing_incidence %d\n", o->wing_incidence);
		fprintf(fp, "wing_dihedral %d\n", o->wing_dihedral);
		fprintf(fp, "adverse_yaw %d\n", o->adverse_yaw);
		fprintf(fp, "stall_angle %d\n", o->stall_angle);
		fprintf(fp, "ac_max_thrust %d\n", o->max_thrust);
		fprintf(fp, "ac_max_speed %d\n", o->max_speed);
		fprintf(fp, "ac_stall_speed %d\n", o->stall_speed);
		fprintf(fp, "real_weight %d\n", o->real_weight);
		fprintf(fp, "standard_weight %d\n", o->weight);
		fprintf(fp, "start_fuel %d\n", o->start_fuel);
		fprintf(fp, "elevator_max %d\n", o->elevator_max);
		fprintf(fp, "rudder_max %d\n", o->rudder_max);
		fprintf(fp, "pitch_control %d\n", o->pitch_control);
		fprintf(fp, "roll_control %d\n", o->roll_control);
		fprintf(fp, "yaw_control %d\n", o->yaw_control);
		fprintf(fp, "pitch_stability %d\n", o->pitch_stability);
		fprintf(fp, "yaw_stability %d\n", o->yaw_stability);
		fprintf(fp, "pitch_inertia %d\n", o->pitch_inertia);
		fprintf(fp, "roll_inertia %d\n", o->roll_inertia);
		fprintf(fp, "yaw_inertia %d\n", o->yaw_inertia);
		fprintf(fp, "pitch_drag %d\n", o->pitch_drag);
		fprintf(fp, "roll_drag %d\n", o->roll_drag);
		fprintf(fp, "yaw_drag %d\n", o->yaw_drag);
		fprintf(fp, "x_drag %d\n", o->x_drag);
		fprintf(fp, "y_drag %d\n", o->y_drag);
		fprintf(fp, "clean_drag %d\n", o->clean_drag);
		fprintf(fp, "air_brake_drag %d\n", o->air_brake_drag);
		fprintf(fp, "gear_drag %d\n", o->gear_drag);
		fprintf(fp, "engine_drag %d\n", o->engine_drag);
		fclose(fp);
	}
}

static void init_aircraft_data(sObject *self)
{
	Plane *o;

	o = self->data;
	o->r = zero;
	o->wv = zero;
	o->bdr = zero;
	o->last_torque = zero;
	o->bwv = zero;
	o->last_force = zero;
	o->fuel = AC_MAX_FUEL;
	o->eye_height = MM(2300);
	o->start_pos.x = M(0);
	o->start_pos.y = o->eye_height;
	o->start_pos.z = M(0);
	o->start_ry = 0;

	o->wing_lift_coeff = 30;
	o->wing_incidence = 150;
	o->wing_dihedral = DEG(2);
	o->stall_angle = DEG(15);
	o->max_thrust = 2000;
	o->max_speed = FIXSPD(600);
	o->stall_speed = FIXSPD(100);
	o->real_weight = 1380;
	o->weight = 2000;
	o->start_fuel = 500;
	o->elevator_max = DEG(35);
	o->rudder_max = DEG(25);
	o->pitch_stability = 2400;
	o->yaw_stability = 2000;
	o->adverse_yaw = 5;
	o->pitch_control = 17;
	o->roll_control = 30;
	o->yaw_control = 8;
	o->pitch_inertia = 250;
	o->roll_inertia = 200;
	o->yaw_inertia = 250;
	o->pitch_drag = 60;
	o->roll_drag = 60;
	o->yaw_drag = 60;
	o->x_drag = 200;
	o->y_drag = 400;
	o->clean_drag = 100;
	o->air_brake_drag = 70;
	o->gear_drag = 40;
	o->engine_drag = 90;
	o->retracts = TRUE;
	o->flaps = TRUE;
	o->airbrakes = TRUE;
	o->biplane = FALSE;
	o->taildrag = FALSE;
	o->ejector = TRUE;
	o->num_engines = 1;
	o->jet = TRUE;

	o->left_wing_normal.x = 0;
	o->left_wing_normal.y = -(1 << SSHIFT);
	o->left_wing_normal.z = 0;
	o->right_wing_normal.x = 0;
	o->right_wing_normal.y = -(1 << SSHIFT);
	o->right_wing_normal.z = 0;

	o->rpm = 10;
	o->engine_on = TRUE;
	o->autostab_on = FALSE;
	o->stalled = FALSE;
	o->flap_angle = 0;
	o->ground_height = 0;
	o->runway_height = 0;
	o->runway = NULL;
	o->brakes_on = FALSE;
	o->air_brake_on = FALSE;
	o->gear_down = TRUE;
	o->radalt_on = FALSE;
	o->crashed = FALSE;
	o->crash_reason = 0;
	o->crash_count = 0;
	o->airframe_failure = 0;
	o->landed = TRUE;
	o->refueled = TRUE;
	o->first_pass = TRUE;
	o->grounded = TRUE;
	o->grounded_time = 0;
	o->lowwarn_time = 0;

	o->hud_fail = FALSE;
	o->rad_fail = FALSE;
	o->nav_fail = FALSE;
	o->hyd_fail = FALSE;
	o->engine_fail = FALSE;
}

/* Method used to read in an inital world state */
static void civil_file_read(sObject *self, FILE *fp)
{
	Basic_data data;
    Plane *o;
	char cockpit[32];
	char model[32];
	char gear_down_shape[32];
	sVector min, max;

    o = self->data;

	file_read_basic(&data, fp);

    fscanf(fp, "%s\n", cockpit);
    fscanf(fp, "%s\n", model);
    fscanf(fp, "%s\n", gear_down_shape);

	o->p.x = M(data.x); o->p.y = M(data.y); o->p.z = M(data.z);
	o->flags = data.f;
	o->r.y = DEG(data.ry);
    o->default_shape = o->shape = get_shape(data.shape);
	o->speed = FIXSPD(0);
	set_object_radius(self);
	find_size(o->shape, &min, &max);

	o->strength = data.strength;

	o->cockpit = read_gauges(cockpit);
    o->gear_down_shape = get_shape(gear_down_shape);

	if (o->gear_down_shape)
		o->shape = o->gear_down_shape;

	o->eye_height = -min.y;
	if (o->eye_height < MM(800))
		o->eye_height = MM(800);
	read_model_params(model, self->data);
	o->start_class = self->class;
}

static int draw_cockpit(sObject *o)
{
	Plane *b;

	b = o->data;
	if (look_direction == 0 && look_elevation == 0) {
      draw_gauges(b->cockpit);
	  return TRUE;
	}
	return FALSE;
}

void plane_viewer(sObject *self)
{
  Plane *o;

  o = self->data;
  if (look_direction == 0 && look_elevation == 0) {
	set_cockpit_state(o->cockpit, draw_full_cockpit);
  	change_cockpit(o->cockpit);
  } else {
    change_cockpit(NULL);
  }
  set_viewer(self);
}

int plane_gear(sObject *o)
{
  Plane *p;

  p = o->data;
  if (p->retracts) {
  	if (!p->landed && !p->hyd_fail) {
		gear_sound();
   		p->gear_down = !p->gear_down;
  		if (p->gear_down_shape) {
	  		p->shape   = (p->gear_down) ? p->gear_down_shape : p->default_shape;
  		}
  	}
  }
  return TRUE;
}

int plane_airbrake(sObject *o)
{
  Plane *p;

  p = o->data;
  if (p->airbrakes) {
  	p->air_brake_on = !p->air_brake_on;
  }
  return TRUE;
}

int plane_wheelbrake(sObject *o)
{
  Plane *p;

  p = o->data;
  p->brakes_on = !p->brakes_on;
  return TRUE;
}

int plane_flaps(sObject *self)
{
  Plane *p;

	p = self->data;
	if (p->flaps) {
		p->flap_angle += 10;
		if (p->flap_angle > 30)
			p->flap_angle = 0;
	}
	return TRUE;
}

int plane_engine(sObject *self)
{
  Plane *p;

  p = self->data;
  p->engine_on = !p->engine_on;

  return TRUE;
}

int plane_eject(sObject *self)
{
	Plane *o;

	o = self->data;
	if (ejector_symbol->value == NULL) {
		if (!o->landed && o->ejector) {
			start_eject(&o->p);
			if (rand() > (RAND_MAX / 2))
				o->airframe_failure = 1;
			else
				o->airframe_failure = 2;
			self->class = Crashing_class;
			eject_sound();
		}
	}

	return TRUE;
}

int plane_autopilot(sObject *self)
{
	Plane *o;

	o = self->data;
	o->autostab_on = !o->autostab_on;

	return TRUE;
}

/* Returns number of kp caused by collision between self and
 * object c - default 1
 * called from check_collide method
 *  */
static int plane_collide(sObject *self, sObject *c)
{
	Plane  *p;
	World  *o;
	sShape_def *sd;

	p = self->data;
	o = c->data;
	if (p->shape && o->shape && o->strength > 0) {
		sd = o->shape;
		while (sd->down)
			sd = sd->down;
		if (check_collision(self, sd->s, &o->p, &o->r, TRUE)) {
			if (c->class == Cultural_class)
				SafeMethod(self, World, hit)(self, c, 10);
			else
				SafeMethod(self, World, hit)(self, c, 1);
			return 1;
		}
	}
	return 0;
}

#define CheckFail(V) if (!V && fst_random() > 30) {V = TRUE; return;}

static void cause_fail(Plane *o)
{
    if (o->damage > (o->strength - 20)) {
		CheckFail(o->hud_fail)
		CheckFail(o->rad_fail)
		CheckFail(o->nav_fail)
		CheckFail(o->hyd_fail)
		CheckFail(o->engine_fail)
	}
}

static int plane_hit(sObject *self, sObject *collide, int kp)
{
    Plane *o;

	o = self->data;
    if (o->damage < o->strength) {
		o->damage += kp;
		explo2_sound();
		cause_fail(o);
		if (o->damage >= o->strength) {
  			SafeMethod(self, World, kill)(self);
			return TRUE;
		}
	}
	return FALSE;
}

static void plane_kill(sObject *self)
{
    Plane *o;

    o = self->data;

	if (rand() > (RAND_MAX / 2))
		o->airframe_failure = 1;
	else
		o->airframe_failure = 2;
    create_explo(o->p, zero, o->shape, FIXSPD(8));
	explo2_sound();
	self->class = Crashing_class;
}

static void update_crashing(sObject *self)
{
	Plane *o;

	o = self->data;
	update_plane(self);
}

static void dead_plane_kill(sObject *self)
{
    Plane *o;

	if (player_die) {
		remove_object(self);
		end_fst();
	} else {
		o = self->data;
		explo3_sound();
		self->class = o->start_class;
		restart_plane(self);
	}
}

static void civil_detaching(sObject *self, sObject *old)
{
	Plane *o;

    o = self->data;
	if (o->runway == old) {
		o->runway = NULL;
	}
}

static int draw_plane(sObject *self)
{
	Plane *o;
	sVector p;

	o = self->data;
    if (o->shape) {
		if (o->p.y < (o->runway_height + M(150))) {
			p.x = o->p.x;
			p.y = o->runway_height;
			p.z = o->p.z;
        	draw_shadow((sShape_def *)o->shape, &p, &o->r);
		}
        return(draw_shape_def((sShape_def *)o->shape, &o->p, &o->r, TRUE));
    }
	return FALSE;
}

void register_plane(void)
{
    Plane_class = new_class(sizeof(PlaneClass), "Plane", sizeof(Plane), Mobile_class);

	Plane_class->reset     = init_aircraft_data;
	Plane_class->update    = update_plane;
	Plane_class->draw_2d   = draw_cockpit;
	Plane_class->collide   = plane_collide;
	Plane_class->hit   	   = plane_hit;
	Plane_class->kill      = plane_kill;
	Plane_class->draw      = draw_plane;


	add_message(Plane_class, "become_viewer", 0, plane_viewer);

	/* Control inputs */
	add_message(Plane_class, "gear_control",  		0, plane_gear);
	add_message(Plane_class, "airbrake_control",  	0, plane_airbrake);
	add_message(Plane_class, "wheelbrake_control",  0, plane_wheelbrake);
	add_message(Plane_class, "flaps_control",  		0, plane_flaps);
	add_message(Plane_class, "engine_control",  	0, plane_engine);
	add_message(Plane_class, "eject",  				0, plane_eject);
	add_message(Plane_class, "autopilot_control",  	0, plane_autopilot);

	Civil_class = copy_class(Plane_class, "Civil");
	Civil_class->file_read = civil_file_read;
	Civil_class->detaching = civil_detaching;

	Crashing_class = copy_class(Plane_class, "Crashing");
	Crashing_class->update = update_crashing;
	Crashing_class->kill   = dead_plane_kill;

	SetupMessage(rearm);
}

