/* --------------------------------------------------------------------------
 * EXEC 		Copyright (c) Simis Ltd 1993,994,1995
 * ---			All Rights Reserved
 *
 * File:		aircraft.c
 * Ver:			1.00
 * Desc:		Aircraft class - hanger generated objects
 * 
 * Aircraft are created by the Hanger class objects. Aircraft
 * are initially put on paths linked to hanger; behavoir in flight
 * will depend upon type
 * 
 * FIGHTER   - engage any aircraft on opposite team in dogfight
 * TRANSPORT - always follows path
 * BOMBER    - follows path until in vicinity of ground targets then
 *             drops bombs
 * STRAFER   - follows path until in vicinity of ground targets then 
 * 			   strafes ground objects
 * 
 * -------------------------------------------------------------------------- */
#include <stdlib.h>
#include "aircraft.h"
#include "ground.h"
#include "bangs.h"
#include "explo.h"
#include "smoke.h"
#include "bullets.h"
#include "missile.h"
#include "flare.h"
#include "sound.h"
#include "weapons.h"

#include "defs.h"

static sSymbol *resupply = NULL;

/* type argument to create_ac */
#define FIGHTER   1
#define TRANSPORT 2
#define BOMBER    3
#define STRAFER   4

#define TRANSPORT_MAX_ROLL ((DEG(1) * frame_time) >> 3)
#define FIGHTER_MAX_ROLL ((DEG(4) * frame_time) >> 3)
#define FIGHTER_MAX_YAW ((DEG(1) * frame_time) >> 3)

#define MIN_HEIGHT M(2)
#define MAX_ALT M(8000)
#define BULLET_SPEED FIXSPD(1300)

#define MAX_SPEED_CHANGE FIXSPD(2)

#define BOMB_KP 10

/* Declare Class Pointer */
AircraftClass *Aircraft_class = NULL;
AircraftClass *Fighter_class  = NULL;
AircraftClass *Strafer_class  = NULL;
AircraftClass *Escape_class   = NULL;
AircraftClass *Death_class = NULL;

static int limit_value(int val, int max)
{
	if (val > max)
		return max;
	else if (val < -max)
		return -max;
	return val;
}

#define NUM_PYLONS 7
static int32 pylon_positions[NUM_PYLONS] = {M(4),M(3),M(2),M(-2),M(-3),M(-4),M(0)};
static int pylon_angles[NUM_PYLONS] = {-60,-85,-95,-105,-75,-60,0};

static void calc_pylon(sObject *carrier, int n, Pylon *p)
{
	sVector offset;
	Mobile *o;
	sVector r, dr;
    Transdata t;

	o = carrier->data;

	p->p.x = pylon_positions[n-1];
	p->p.y = M(-1);
	p->p.z = M(1);

	/* Rotate pylon to correct posn wrt a/c */
	p->r = o->r;
	dr.x = pylon_angles[n-1]; dr.y = dr.z = 0;
	ac_axis_rotate(&p->r, &dr);
	setup_trans(&p->r, &t);
	rot3d(&p->p, &p->p, &t);

	/* Add to real world position of carrier object */
	p->p.x += o->p.x;
	p->p.y += o->p.y;
	p->p.z += o->p.z;

	p->wv = o->wv;
	p->speed = o->speed;
}

static void change_to_attack(sObject *self)
{
    Aircraft *ac;

    ac = self->data;
	ac->speed_goal = ac->max_speed;
	if (ac->type == STRAFER)
		self->class = Strafer_class;
	else
		self->class = Fighter_class;
}

static void change_to_patrol(sObject *self)
{
    Aircraft *ac;

    ac = self->data;
	ac->danger_count = 0;
	ac->last_range = M(10000);
	ac->speed_goal = ac->max_speed;
	self->class = Aircraft_class;
}

static void change_to_escape(sObject *self, int roll_goal, int time)
{
    Aircraft *ac;

    ac = self->data;
	ac->roll_goal = roll_goal;
	ac->escape_time = time;
	ac->escape_phase = 1;
	self->class = Escape_class;
}

#define CLOSE M(100)

/* --------------------
 * METHOD update_ac
 * 
 * Basic update method for Aircraft
 */ 
static void update_ac(sObject *self)
{
    int bearing, elev, angle, dspd;
    int32 range, ahead_height, ground_height, offset;
    Aircraft *ac;
	World *tgt;
    sVector d, tgt_p, terrain_follow_pt;
    sVector v, a;
    Transdata trans;
	sObject *o;
	Wpt *wpt;
	Pylon  pylon;

    ac = self->data;
    setup_trans(&ac->r, &trans);

    /* Ground avoidance */
    ground_height = get_terrain_height(ac->p.x, ac->p.z);

	if (ac->smoking && ((ac->smoke_delay -= frame_time) <= 0)) {
		if (ac->smoke_count-- < 0)
			ac->smoking = FALSE;
		ac->smoke_delay = 50;
		create_smoke(&ac->p, 400);
	}
	if (ac->aggressive && (ac->type == BOMBER) && (ac->bombs > 0)) {
		if (ac->target) {
			tgt = ac->target->data;
			calc_bomb_ccip(self, &d, ground_height);
			range = get_range(&d, &tgt->p, &d);
			d.x >>= SPEED_SHIFT;
			d.y >>= SPEED_SHIFT;
			d.z >>= SPEED_SHIFT;
			invrot3d(&d, &d, &trans);
			if ((d.z < Meter(-50)) || (range > M(2000))) {
				ac->target = NULL;
			} else {
				if (range < M(50)) {
					calc_pylon(self, 2, &pylon);
					launch_bomb(self, &pylon, BOMB_KP);
					ac->bombs--;
					ac->target = NULL;
					ac->next_target_time = total_time + 40;
				}
			}
		} else if (ac->airborne && (ac->flags & TEAM_FLAGS) &&
				(total_time > ac->next_target_time)) {
			ac->next_target_time = total_time + SECS(5);
			ac->target = get_nearest_target(self, FALSE, M(1000) + ac->p.y, FALSE);
		}
	}

	if (ac->target && (ac->type == BOMBER) && (ac->bombs > 0)) {
		tgt = ac->target->data;
		tgt_p.x = tgt->p.x;
		tgt_p.y = ac->p.y;
		tgt_p.z = tgt->p.z;
		range = get_range(&ac->p, &tgt_p, &d);
	} else {
		/* Has ac reached wpt */
    	range = get_range(&ac->p, &ac->wpt->p, &d);
		if (range < CLOSE || ((range < M(1000)) && (range > ac->last_range))) {
			if (ac->wpt->spd > 0)
				ac->speed_goal = ac->wpt->spd;
			if (ac->speed_goal < ac->min_speed)
				ac->speed_goal = ac->min_speed;
			if (ac->speed_goal > ac->max_speed)
				ac->speed_goal = ac->max_speed;
			ac->aggressive = ac->wpt->active;
			if (ac->speed > ac->min_speed)
				ac->airborne = TRUE;
			/* Set up ac next wpt */
			wpt = list_next(ac->wpt, ac->path->wpts);
			if (wpt && (wpt != Car(ac->path->wpts))) {
				ac->wpt = wpt;
    			range = get_range(&ac->p, &wpt->p, &d);
			} else {
				/* Reached end of path */
				if (ac->path->supply) {
					o = ac->path->supply->value;
					if (o && o->id) {
						message(o, resupply);
					}
				}
				set_viewer(Player);
				remove_target(self);
				remove_object(self);
				return;
			}
		}
	}

    ac->last_range = range;
    bearing = fast_atan(-d.x, d.z);
	if (d.y < M(20))
    	elev = fast_atan(d.y, range);
	else
    	elev = fast_atan(d.y, ABS(d.y) << 3);
	bearing = SUBANGLE(ac->r.y, bearing);
    if (bearing > DEG(42)) {
        ac->roll_goal = DEG(50);
    } else if (bearing < DEG(-42)) {
        ac->roll_goal = DEG(-50);
	} else {
        ac->roll_goal = bearing << 1;
	}

    ac->dr.z = (int)SR(MUL(ac->roll_goal - ac->r.z, frame_time), 5);
	ac->dr.z = limit_value(ac->dr.z, TRANSPORT_MAX_ROLL);
    ac->r.z += ac->dr.z;

    ac->r.x += (int)SR(MUL(elev - ac->r.x, frame_time), 6);

	angle = ((int32)SIN(ac->r.z) << 10) >> SSHIFT;
	if (ABS(ac->r.z) < DEG(76)) {
		angle = MULDIV(angle, frame_time, ac->roll_couple);
	} else {
        angle = MULDIV(angle, frame_time, (2 * ac->roll_couple) / 3);
	}
	ac->r.y = SUBANGLE(ac->r.y, angle);

    dspd = limit_value(ac->speed_goal - ac->speed, MAX_SPEED_CHANGE);
	ac->big_speed += MUL(dspd, frame_time);
	ac->speed = SR(ac->big_speed, 4);

	v.x = 0; v.y = 0; v.z = ac->speed;
    rot3d(&v, &ac->wv, &trans);
    ac->op = ac->p;
    ac->p.x += MUL(ac->wv.x, frame_time);
    ac->p.y += MUL(ac->wv.y, frame_time);
    ac->p.z += MUL(ac->wv.z, frame_time);

	if (ac->p.y < ground_height+MIN_HEIGHT) {
    	ac->p.y = ground_height+MIN_HEIGHT;
		if ((short)ac->r.x < 0)
			ac->r.x = 0;
	}

	if (self == selected) {
		if ((ac->flare_delay -= frame_time) <= 0) {
			if ((rand() & 0x03) == 0)
				drop_flare(&ac->op, &ac->wv);
			ac->flare_delay = SECS(5);
		}
	}

	if (Player) {
		tgt = Player->data;
		range = get_range(&ac->p, &tgt->p, &d);
		air_target(self, range);
	}
	if (ac->type == FIGHTER) {
		if (Player) {
			tgt = Player->data;
			if (ac->airborne && (ac->flags & TEAM_FLAGS) &&
					(range < M(8000))) {
				if ((tgt->flags & TEAM_FLAGS) &&
					((ac->flags & TEAM_FLAGS) != (tgt->flags & TEAM_FLAGS))) {
            		/* can it see target or is it being attacked */
            		d.x >>= SPEED_SHIFT;
            		d.y >>= SPEED_SHIFT;
            		d.z >>= SPEED_SHIFT;
					invrot3d(&d, &d, &trans);
            		bearing = fast_atan(-d.x, d.z);
            		if ((ABS(bearing) < DEG(120)) || (ac->danger_count > 0)) {
						ac->target = Player;
						change_to_attack(self);
            		}
				}
			}
		}
		if (ac->airborne && (ac->flags & TEAM_FLAGS) &&
				(ac->target == NULL) && (total_time > ac->next_target_time)) {
			ac->next_target_time = total_time + SECS(5);
			o = get_nearest_target(self, TRUE, M(8000), FALSE);
			if (o) {
				ac->target = o;
				change_to_attack(self);
			}
		}
	}

	if (ac->aggressive && (ac->type == STRAFER)) {
		if (ac->airborne && (ac->flags & TEAM_FLAGS) &&
				(ac->target == NULL) && (total_time > ac->next_target_time)) {
			ac->next_target_time = total_time + SECS(5);
			o = get_nearest_target(self, FALSE, M(5000), FALSE);
			if (o) {
				ac->target = o;
				change_to_attack(self);
			}
		}
	}

	if ((ac->rear_gun) && (ac->flags & TEAM_FLAGS)) {
		if (Player) {
			if (total_time > ac->next_rear_time) {
				ac->next_rear_time = total_time + SECS(3);
				ac->rear_target = get_nearest_target(self, TRUE, range, TRUE);
			}
			if (ac->rear_target) {
				tgt = ac->rear_target->data;
				range = get_range(&ac->p, &tgt->p, &d);
				if (range > M(400)) {
					ac->rear_target = NULL;
				} else {
					if ((ac->rear_gun_time < total_time) &&
						(tgt->flags & TEAM_FLAGS) &&
						((ac->flags & TEAM_FLAGS) != (tgt->flags & TEAM_FLAGS))) {
            			d.x >>= SPEED_SHIFT;
            			d.y >>= SPEED_SHIFT;
            			d.z >>= SPEED_SHIFT;
            			range >>= SPEED_SHIFT;
						invrot3d(&d, &tgt_p, &trans);
            			bearing = fast_atan(-tgt_p.x, tgt_p.z);
						elev = fast_atan(tgt_p.y, range);
            			if ((ABS(bearing) > DEG(135)) &&
			 				(elev < DEG(30)) && (elev > DEG(-10))) {
							ac->rear_gun_time = total_time + 25L;
							v.x = ADDANGLE(fast_atan(d.y, range), range / 30);
							v.y = fast_atan(-d.x, d.z);
							v.z = 0;
							offset = M(5);
							fire_bullet(self, &v, offset, BULLET_SPEED, 200, FALSE);
            			}
					}
				}
			}
		}
	}
}

static void check_speed_goal(Aircraft *ac)
{
	int max_speed;

	if (ac->speed_goal < ac->min_speed)
		ac->speed_goal = ac->min_speed;
	max_speed = ac->max_speed - MULDIV(ac->max_speed, ABS(ac->r.z), DEG(90) * 4);
	if (ac->r.x > DEG(20)) {
		ac->climb_time += frame_time * ((ac->r.x - DEG(20)) / DEG_1);
	} else {
		if (ac->climb_time > 0) {
			ac->climb_time -= frame_time * ((DEG(20) - ac->r.x) / DEG_1);
		}
	}
	max_speed = max_speed - MULDIV(max_speed, ac->climb_time, 1000 * 70);
	if (max_speed < FIXSPD(20))
		max_speed = FIXSPD(20);
	if (ac->speed_goal > max_speed)
		ac->speed_goal = max_speed;
}

static void update_escape(sObject *self)
{
    int roll_error, dspd;
    int32 ground_height;
    Aircraft *ac;
	sVector v;
    Transdata trans;

    ac = self->data;

	if (ac->smoking && ((ac->smoke_delay -= frame_time) <= 0)) {
		if (ac->smoke_count-- < 0)
			ac->smoking = FALSE;
		ac->smoke_delay = 50;
		create_smoke(&ac->p, 400);
	}
    /* Ground avoidance */
    ground_height = get_terrain_height(ac->p.x, ac->p.z);

	if (ac->escape_time < 0) {
		ac->escape_time = 0;
		change_to_attack(self);
	}

	ac->dr.x = ac->dr.y = ac->dr.z = 0;
	if (ac->speed < ac->min_speed) {
		ac->escape_time -= frame_time;
		ac->dr.z = (int)SR(MUL(-ac->r.z, frame_time), 4);
		if (ac->r.x > DEG(-45)) {
			ac->r.x -= (int)SR(MUL(DEG(5), frame_time), 4);
		}
	} else {
		if (ac->escape_phase == 1) {
			ac->dr.x = (ac->max_pitch_rate * frame_time) / 8;
			roll_error = SUBANGLE(ac->roll_goal, ac->r.z);
			ac->dr.z = (int)SR(MUL(roll_error, frame_time), 4);
			if (ABS(roll_error) < DEG(15)) {
				ac->escape_phase = 2;
			}
		} else if (ac->escape_phase == 2) {
			ac->escape_time -= frame_time;
			ac->dr.x = ac->max_pitch_rate * frame_time;
			ac->speed_goal = ac->max_speed;
		}
	}
	ac->dr.x = limit_value(ac->dr.x, ac->max_pitch_rate * frame_time);
	ac->dr.y = limit_value(ac->dr.y, FIGHTER_MAX_YAW);
	ac->dr.z = limit_value(ac->dr.z, FIGHTER_MAX_ROLL);
    ac_axis_rotate(&ac->r, &ac->dr);

	check_speed_goal(ac);
	dspd = limit_value(ac->speed_goal - ac->speed, MAX_SPEED_CHANGE);
	ac->big_speed += MUL(dspd, frame_time);
	ac->speed = SR(ac->big_speed, 4);

	setup_trans(&ac->r, &trans);
	v.x = 0; v.y = 0; v.z = ac->speed;
	rot3d(&v, &ac->wv, &trans);
	ac->op = ac->p;
	ac->p.x += MUL(ac->wv.x, frame_time);
	ac->p.y += MUL(ac->wv.y, frame_time);
	ac->p.z += MUL(ac->wv.z, frame_time);
	if (ac->p.y < ground_height+MIN_HEIGHT) {
    	ac->p.y = ground_height+MIN_HEIGHT;
		ac->r.z = 0;
		if ((short)ac->r.x < 0)
			ac->r.x = 0;
	}
	if (ac->p.y > MAX_ALT) {
  		SafeMethod(self, World, kill)(self);
	}
}


/* --------------------
 * METHOD update_fighter
 * 
 * Update method for fighter
 */ 
static void update_fighter(sObject *self)
{
	int bearing, elev, angle, dspd, max_speed, time;
	int dest_rx, dest_ry, dest_rz;
	int32 range, ground_height, offset;
	int srange;
	Aircraft *ac;
	sVector d;
	sVector v;
	Transdata trans, trans1;
	Mobile *tgt;
	Pylon pylon;

	ac = self->data;
	if (ac->target == NULL) {
		change_to_patrol(self);
		return;
	}
	tgt = ac->target->data;
	if (tgt == NULL) {
		change_to_patrol(self);
		return;
	}

	if (ac->smoking && ((ac->smoke_delay -= frame_time) <= 0)) {
		if (ac->smoke_count-- < 0)
			ac->smoking = FALSE;
		ac->smoke_delay = 50;
		create_smoke(&ac->p, 400);
	}
	/* Ground avoidance */
	ground_height = get_terrain_height(ac->p.x, ac->p.z);

	setup_trans(&ac->r, &trans);

	d.x = tgt->p.x - ac->p.x;
	d.y = tgt->p.y - ac->p.y;
	d.z = tgt->p.z - ac->p.z;
	range = magnitude(&d);
	time = range / BULLET_SPEED;
	d.x += MUL(tgt->wv.x, time);
	d.y += MUL(tgt->wv.y, time);
	d.z += MUL(tgt->wv.z, time);
	d.x >>= SPEED_SHIFT;
	d.y >>= SPEED_SHIFT;
	d.z >>= SPEED_SHIFT;
	srange = range >> SPEED_SHIFT;
	/* target relative to aircraft */
	invrot3d(&d, &d, &trans);
	dest_rx = fast_atan(d.y, srange);
	dest_ry = fast_atan(-d.x, d.z);
	dest_rz = fast_atan(d.x, d.y);

	/* Weapon aiming */
	if ((ABS(dest_ry) < DEG(30)) && (ABS(dest_rx) < DEG(30))) {
		ac->danger_count = 0;
    	if (ac->jet && (ac->missile_time < total_time) && (ac->num_missiles > 0)) {
        	if ((ABS(dest_ry) < DEG(20)) && (ABS(dest_rx) < DEG(20))) {
            	/* is target showing back end */
            	d.x = (ac->p.x - tgt->p.x) >> SPEED_SHIFT;
            	d.y = (ac->p.y - tgt->p.y) >> SPEED_SHIFT;
            	d.z = (ac->p.z - tgt->p.z) >> SPEED_SHIFT;
            	setup_trans(&tgt->r, &trans1);
            	invrot3d(&d, &d, &trans1);
            	bearing = fast_atan(-d.x, d.z);
            	elev = fast_atan(d.y, srange);
            	if ((ABS(bearing) > DEG(130)) && (ABS(elev) < DEG(50))) {
                	ac->missile_time = total_time + 1500L;
                	ac->num_missiles--;
					calc_pylon(self, 1, &pylon);
					add_aa(self, &pylon, ac->target, 5);
                	maw_sound();
            	}
        	}
    	}
    	if ((srange < Meter(800)) && (ac->cannon_time < total_time) &&
            	BETWEEN(dest_ry, DEG(1), DEG(-1)) && BETWEEN(dest_rx, DEG(1), DEG(-3))) {
        	ac->cannon_time = total_time + 25L;
			v.x = ADDANGLE(ac->r.x, srange / 17);
			v.y = ac->r.y;
			v.z = 0;
			offset = M(5);
			fire_bullet(self, &v, offset, BULLET_SPEED, 200, FALSE);
			if ((range < M(450)) && ((ac->last_range - range) > (ac->speed * frame_time))) {
				change_to_escape(self, (fst_random() >= 50) ? DEG(70) : DEG(-70), 200 + (2 * fst_random()));
			}
    	}
	} else if ((srange < Meter(1500)) && (ABS(dest_ry) > DEG(100)) || (ABS(dest_rx) > DEG(100))) {
		ac->danger_count += frame_time;
	}

	if (range > M(400))
		ac->speed_goal = tgt->speed + FIXSPD(100);
	else if (range > M(200))
		ac->speed_goal = tgt->speed + FIXSPD(30);
	else if (range > M(90))
		ac->speed_goal = tgt->speed + FIXSPD(10);
	else
		ac->speed_goal = tgt->speed - FIXSPD(10);
	check_speed_goal(ac);

	ac->dr.x = ac->dr.y = ac->dr.z = 0;
	if (ac->speed < ac->min_speed) {
       	ac->roll_goal = 0;
		ac->dr.z = (int)SR(MUL(ac->roll_goal - ac->r.z, frame_time), 4);
		if (ac->r.x > DEG(-45)) {
			ac->r.x -= (int)SR(MUL(DEG(5), frame_time), 4);
		}
	} else if (ac->danger_count > 500) {
		ac->danger_count = 0;
		change_to_escape(self, (fst_random() >= 50) ? DEG(45) : DEG(-45), 500L);
	} else {
    	if ((ABS(dest_ry) < DEG(5)) && (ABS(dest_rx) < DEG(5))) {
			ac->dr.x = (int)SR(MUL(dest_rx, frame_time), 5);
			ac->dr.y = (int)SR(MUL(dest_ry, frame_time), 5);
			if ((ac->last_range - range) > (ac->speed * frame_time))
        		ac->roll_goal = -(dest_ry << 1);
			else
        		ac->roll_goal = tgt->r.z - (dest_ry << 1);
			ac->dr.z = (int)SR(MUL(ac->roll_goal - ac->r.z, frame_time), 4);
		} else {
			ac->dr.z = (int)SR(MUL(dest_rz, frame_time), 5);
		}
		if (ABS(dest_rz) < DEG(15)) {
			ac->dr.x = (int)SR(MUL(dest_rx, frame_time), 5);
		}
	}

	ac->dr.x = limit_value(ac->dr.x, ac->max_pitch_rate * frame_time);
	ac->dr.y = limit_value(ac->dr.y, FIGHTER_MAX_YAW);
	ac->dr.z = limit_value(ac->dr.z, FIGHTER_MAX_ROLL);
    ac_axis_rotate(&ac->r, &ac->dr);

	dspd = limit_value(ac->speed_goal - ac->speed, MAX_SPEED_CHANGE);
	ac->big_speed += MUL(dspd, frame_time);
	ac->speed = SR(ac->big_speed, 4);

	v.x = 0; v.y = 0; v.z = ac->speed;
	rot3d(&v, &ac->wv, &trans);
	ac->op = ac->p;
	ac->p.x += MUL(ac->wv.x, frame_time);
	ac->p.y += MUL(ac->wv.y, frame_time);
	ac->p.z += MUL(ac->wv.z, frame_time);

	if (ac->p.y < ground_height + M(10)) {
		if ((short)ac->r.x > DEG(-10)) {
			ac->p.y += M(1) / 4;
			if ((short)ac->r.x < DEG(0))
				ac->r.x = DEG(0);
		}
		if (ac->p.y < ground_height + M(2)) {
			SafeMethod(self, World, kill)(self);
		}
	}
	if (ac->p.y > MAX_ALT) {
  		SafeMethod(self, World, kill)(self);
	}

	if (self == selected) {
		if ((ac->flare_delay -= frame_time) <= 0) {
			if ((rand() & 0x03) == 0)
				drop_flare(&ac->op, &ac->wv);
			ac->flare_delay = SECS(5);
		}
	}

	air_target(self, range);
	if (range > M(12000)) {
		change_to_patrol(self);
	}
    ac->last_range = range;
}

static void update_strafer(sObject *self)
{
	int bearing, elev, angle, dspd, max_speed, time;
	int dest_rx, dest_ry, dest_rz;
	int32 range, ground_height, offset;
	int srange;
	Aircraft *ac;
    sVector d, tgt_p;
	sVector v;
	Transdata trans;
	World *tgt;
	sObject *o;
	Pylon pylon;

	ac = self->data;
	if (ac->target == NULL) {
		change_to_patrol(self);
		return;
	}
	tgt = ac->target->data;
	if (tgt == NULL) {
		change_to_patrol(self);
		return;
	}

	if (ac->smoking && ((ac->smoke_delay -= frame_time) <= 0)) {
		if (ac->smoke_count-- < 0)
			ac->smoking = FALSE;
		ac->smoke_delay = 50;
		create_smoke(&ac->p, 400);
	}
	/* Ground avoidance */
	ground_height = get_terrain_height(ac->p.x, ac->p.z);

	d.x = tgt->p.x - ac->p.x;
	d.y = tgt->p.y - ac->p.y;
	d.z = tgt->p.z - ac->p.z;
	range = magnitude(&d);
	if (tgt->flags & MOBILE) {
		time = range / BULLET_SPEED;
		/* predict target position */
		d.x += MUL(((Mobile *)tgt)->wv.x, time);
		d.y += MUL(((Mobile *)tgt)->wv.y, time);
		d.z += MUL(((Mobile *)tgt)->wv.z, time);
	}
	d.x >>= SPEED_SHIFT;
	d.y >>= SPEED_SHIFT;
	d.z >>= SPEED_SHIFT;
	srange = range >> SPEED_SHIFT;
	/* target relative to aircraft */
	setup_trans(&ac->r, &trans);
	invrot3d(&d, &d, &trans);
	dest_rx = fast_atan(d.y, srange);
	dest_ry = fast_atan(-d.x, d.z);
	dest_rz = fast_atan(d.x, d.y);

	/* Weapon aiming */
	if ((ABS(dest_ry) < DEG(30)) && (ABS(dest_rx) < DEG(30))) {
		ac->danger_count = 0;
    	if ((srange < Meter(1500)) && (ac->cannon_time < total_time) &&
            	BETWEEN(dest_ry, DEG(1), DEG(-1)) && BETWEEN(dest_rx, DEG(1), DEG(-3))) {
        	ac->cannon_time = total_time + 25L;
			v.x = ADDANGLE(ac->r.x, srange / 25);
			v.y = ac->r.y;
			v.z = 0;
			offset = M(5);
			fire_bullet(self, &v, offset, BULLET_SPEED, 200, FALSE);
    	}
	}

	ac->speed_goal = ac->max_speed - FIXSPD(10);
	check_speed_goal(ac);

	ac->dr.x = ac->dr.y = ac->dr.z = 0;
	if (ac->speed < ac->min_speed) {
       	ac->roll_goal = 0;
		ac->dr.z = (int)SR(MUL(ac->roll_goal - (short)ac->r.z, frame_time), 4);
		if ((short)ac->r.x > DEG(-45)) {
			ac->r.x -= (int)SR(MUL(DEG(5), frame_time), 4);
		}
	} else if (ac->danger_count > 500) {
		ac->danger_count = 0;
		change_to_escape(self, (fst_random() >= 50) ? DEG(45) : DEG(-45), 500L);
	} else {
    	if ((ABS(dest_ry) < DEG(5)) && (ABS(dest_rx) < DEG(5))) {
			ac->dr.x = (int)SR(MUL(dest_rx, frame_time), 5);
			ac->dr.y = (int)SR(MUL(dest_ry, frame_time), 5);
			if ((ac->last_range - range) > (ac->speed * frame_time))
        		ac->roll_goal = -(dest_ry << 1);
			else
        		ac->roll_goal = tgt->r.z - (dest_ry << 1);
			ac->dr.z = (int)SR(MUL(ac->roll_goal - (short)ac->r.z, frame_time), 4);
		} else {
			if (range > M(300)) {
				ac->dr.z = (int)SR(MUL(dest_rz, frame_time), 5);
			} else {
        		ac->roll_goal = DEG(0);
				ac->dr.z = (int)SR(MUL(ac->roll_goal - (short)ac->r.z, frame_time), 4);
			}
		}
		if (ABS(dest_rz) < DEG(15)) {
			ac->dr.x = (int)SR(MUL(dest_rx, frame_time), 5);
		}
    	if ((ac->last_range > range) && (range < M(300))) {
			change_to_escape(self, (fst_random() >= 50) ? DEG(30) : DEG(-30), 200 + (2 * fst_random()));
		}
    	if ((ac->last_range < range) && (range < M(2000))) {
			if (dest_ry > 0)
				ac->roll_goal = DEG(-10);
			else
				ac->roll_goal = DEG(10);
			ac->dr.z = (int)SR(MUL(ac->roll_goal - (short)ac->r.z, frame_time), 4);
			ac->dr.x = (int)SR(MUL(DEG(10) - (short)ac->r.x, frame_time), 4);
		}
	}

	ac->dr.x = limit_value(ac->dr.x, ac->max_pitch_rate * frame_time);
	ac->dr.y = limit_value(ac->dr.y, FIGHTER_MAX_YAW);
	ac->dr.z = limit_value(ac->dr.z, FIGHTER_MAX_ROLL);
    ac_axis_rotate(&ac->r, &ac->dr);

	if (ABS((short)ac->r.z) < DEG(30)) {
		angle = ((int32)SIN(ac->r.z) << 10) >> SSHIFT;
		ac->r.y = SUBANGLE(ac->r.y, MULDIV(angle, frame_time, ac->roll_couple >> 1));
	}

	dspd = limit_value(ac->speed_goal - ac->speed, MAX_SPEED_CHANGE);
	ac->big_speed += MUL(dspd, frame_time);
	ac->speed = SR(ac->big_speed, 4);

	v.x = 0; v.y = 0; v.z = ac->speed;
	rot3d(&v, &ac->wv, &trans);
	ac->op = ac->p;
	ac->p.x += MUL(ac->wv.x, frame_time);
	ac->p.y += MUL(ac->wv.y, frame_time);
	ac->p.z += MUL(ac->wv.z, frame_time);

	if (ac->p.y < (ground_height + M(10))) {
		if ((short)ac->r.x > DEG(-10)) {
			ac->p.y += M(1) / 4;
			if ((short)ac->r.x < DEG(0))
				ac->r.x = DEG(0);
		}
		if (ac->p.y < (ground_height + M(2))) {
			SafeMethod(self, World, kill)(self);
		}
	}
	if (ac->p.y > MAX_ALT) {
  		SafeMethod(self, World, kill)(self);
	}

	if (self == selected) {
		if ((ac->flare_delay -= frame_time) <= 0) {
			if ((rand() & 0x03) == 0)
				drop_flare(&ac->op, &ac->wv);
			ac->flare_delay = SECS(5);
		}
	}

	air_target(self, range);
	if (range > M(6000)) {
		change_to_patrol(self);
	}
    ac->last_range = range;

	if ((ac->rear_gun) && (ac->flags & TEAM_FLAGS)) {
		if (total_time > ac->next_rear_time) {
			ac->next_rear_time = total_time + SECS(3);
			ac->rear_target = get_nearest_target(self, TRUE, range, TRUE);
		}
		if (ac->rear_target) {
			tgt = ac->rear_target->data;
			range = get_range(&ac->p, &tgt->p, &d);
			if (range > M(400)) {
				ac->rear_target = NULL;
			} else {
				if ((ac->rear_gun_time < total_time) &&
					(tgt->flags & TEAM_FLAGS) &&
					((ac->flags & TEAM_FLAGS) != (tgt->flags & TEAM_FLAGS))) {
            		d.x >>= SPEED_SHIFT;
            		d.y >>= SPEED_SHIFT;
            		d.z >>= SPEED_SHIFT;
            		range >>= SPEED_SHIFT;
					invrot3d(&d, &tgt_p, &trans);
            		bearing = fast_atan(-tgt_p.x, tgt_p.z);
					elev = fast_atan(tgt_p.y, range);
            		if ((ABS(bearing) > DEG(135)) &&
			 			(elev < DEG(30)) && (elev > DEG(-10))) {
						ac->rear_gun_time = total_time + 25L;
						v.x = ADDANGLE(fast_atan(d.y, range), range / 30);
						v.y = fast_atan(-d.x, d.z);
						v.z = 0;
						fire_bullet(self, &v, M(5), BULLET_SPEED, 200, FALSE);
            		}
				}
			}
		}
	}
}


static void aircraft_detaching(sObject *self, sObject *old)
{
    Aircraft *o;

    o = self->data;
	if (o->rear_target == old) {
		o->rear_target = NULL;
	}
	if (o->target == old) {
		o->target = NULL;
		change_to_patrol(self);
	}
}

static int hit(sObject *self, sObject *collide, int kp)
{
    Aircraft *o;

    o = self->data;
	o->danger_count += 200;
    if (o->strength < (o->start_strength >> 1)) {
		o->smoking = TRUE;
		o->smoke_count = 200;
		o->max_speed = (o->start_max_speed * 3) / 4;
	}
    if (o->strength > 0) {
		o->strength -= kp;
		if (o->strength <= 0) {
			target_scoring(self, collide);
  			SafeMethod(self, World, kill)(self);
		}
	}
	return FALSE;
}

static void kill(sObject *self)
{
	Aircraft *o;

	if (rand() > (RAND_MAX / 4)) {
		remove_target(self);
		broadcast_detaching(self);
		self->class = Death_class;
	} else {
		o = self->data;
		remove_target(self);
		create_explo(o->p, zero, o->shape, FIXSPD(8));
		remove_object(self);
	}
}

static void death_spiral(sObject *self)
{
	int32 ground_height;
	int roll;
	sVector v;
	Transdata t;
	Aircraft *o;

	o = self->data;
	if (fast_cycle == 3)
		small_bang(&o->op);
	if (fast_cycle == 1)
		create_smoke(&o->op, 300);
	if (o->speed < FIXSPD(50))
		o->speed = FIXSPD(50);
	roll = (short)o->r.z;
	if (roll > 0)
		o->dr.z = ((DEG_180 - roll) * frame_time) / 200;
	else
		o->dr.z = -(((DEG_180 + roll) * frame_time) / 200);
	limit_value(o->dr.z, FIGHTER_MAX_ROLL);
	o->dr.x = 25 * frame_time;
	o->dr.y = 0;
	if (o->speed < FIXSPD(600))
		o->speed += GRAVITY * frame_time;

	ac_axis_rotate(&o->r, &o->dr);

	setup_trans(&o->r, &t);
	v.x = 0; v.y = 0; v.z = o->speed;
	rot3d(&v, &o->wv, &t);
	o->op = o->p;
	o->p.x += MUL(o->wv.x, frame_time);
	o->p.y += MUL(o->wv.y, frame_time);
	o->p.z += MUL(o->wv.z, frame_time);
	ground_height = get_terrain_height(o->p.x, o->p.z);
	if ((o->p.y < (ground_height + M(5))) || (o->p.y > M(20000))) {
		SafeMethod(self, World, kill)(self);
	}
}

static void crashed_kill(sObject *self)
{
	Aircraft *o;

	o = self->data;
	medium_ground_bang(&o->p);
	create_explo(o->p, zero, o->shape, FIXSPD(8));
	remove_target(self);
	remove_object(self);
}

void register_aircraft(void)
{
    Aircraft_class = new_class(sizeof(AircraftClass), "Aircraft", sizeof(Aircraft), Mobile_class);

    /* Class methods */
    Aircraft_class->hit  = hit;
    Aircraft_class->kill = kill;
    Aircraft_class->update = update_ac;
	Aircraft_class->detaching = aircraft_detaching;

	Fighter_class = copy_class(Aircraft_class, "Fighter");
	Fighter_class->update = update_fighter;

	Strafer_class = copy_class(Aircraft_class, "Strafer");
	Strafer_class->update = update_strafer;

	Escape_class = copy_class(Aircraft_class, "Escape");
	Escape_class->update = update_escape;

	Death_class = copy_class(Aircraft_class, "Death");
	Death_class->update = death_spiral;
	Death_class->kill   = crashed_kill;

	SetupMessage(resupply);
}


/* -------------------
 * GLOBAL create_ac
 * 
 * Called from hanger.c - creates aircraft and sets up
 * default behaviour.
 */
sObject *create_ac(sShape_def *sd, int jet, int min_speed, int max_speed, Path *p, int type, int rear_gun, int strength, int bombs, int team_flag)
{
    sObject *o;
    Aircraft *t;
	sVector d;
	sList *l;
    Wpt *w1, *w2;

	o = NULL;
	if (sd && (max_speed > 0) && p) {
		o = new_object(Aircraft_class);
		if (o) {
			t = o->data;
			w1 = w2 = NULL;

			l = p->wpts->next;
			if (l != p->wpts)
				w1 = l->item;

			l = l->next;
			if (l != p->wpts)
				w2 = l->item;

			if (w1 && w2) {
				t->flags |= team_flag;
				t->start_strength = t->strength = strength;
				t->smoke_count = 0;
				t->smoke_delay = 0;
				t->smoking = FALSE;
				t->type = type;
				t->rear_gun = rear_gun;
				t->bombs = bombs;
				t->airborne = FALSE;
				t->aggressive = FALSE;
				t->target = NULL;
				t->rear_target = NULL;
				t->shape = sd;
				t->p = w1->p;
				t->path = p;
				t->wpt = w2;
				t->speed = 0;
				t->big_speed = t->speed << 4;
				t->effective_roll = 0;
				t->roll_goal = 0;
				t->jet = jet;
				t->start_max_speed = t->max_speed = max_speed;
				t->min_speed = min_speed;
				t->speed_goal = t->max_speed;
				t->roll_couple = 120000L / t->max_speed;
				t->max_pitch_rate = DEG(1) / 8;
				t->last_range  = M(10000);
				t->num_missiles = 4;
				t->num_cannon = 400;
				t->missile_time = 0;
				t->cannon_time = 0;
				t->rear_gun_time = 0;
				t->danger_count = 0;
				t->escape_time = 0;
				t->climb_time = 0;
				t->next_target_time = 0;
				t->next_rear_time = 0;

				/* Rotate a/c towards second waypoint */
    			get_range(&w1->p, &w2->p, &d);
    			t->r.y = fast_atan(-d.x, d.z);

				attach_object(o);
			}
		}
    }
	return o;
}
