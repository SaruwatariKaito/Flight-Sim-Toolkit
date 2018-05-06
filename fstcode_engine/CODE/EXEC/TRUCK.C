/* --------------------------------------------------------------------------
 * FSF 			Copyright (c) Simis Ltd 1993,994,1995
 * ---			All Rights Reserved
 *
 * File:		truck.c
 * Ver:			2.00
 * Desc:		Depot generated truck object
 * 				Truck follows path supplied by creating function
 * 				At end of path truck is removed from world
 * 				If truck is agressive it fires cannon shells at
 * 				other ground objects in vicinity.
 *
 * -------------------------------------------------------------------------- */
#include <stdlib.h>
#include "truck.h"
#include "ground.h"
#include "bullets.h"
#include "explo.h"

// Externed from gun.c
extern int32 aim_gun(sObject *gun, sObject *target, int rot, int fixed);

// RESUPPLY  - method called when truck reaches end of path
static sSymbol *resupply = NULL;

/* Declare Class Pointer */
TruckClass *Truck_class = NULL;

// Speed of truck munitions, used to calculate aim-offs (knots)
#define SHELL_SPEED FIXSPD(1200)

// Range at which truck is deemed to be at next waypoint
#define CLOSE M(10)

/* -----------------------------------
 * METHOD update_truck
 * Main truck class update function
 * 1 Checks if truck is at destination waypoint, if so sets it up for next
 *   waypoint.
 * 2 Checks if truck is agressive and has targets, if so then fires
 *   cannon shells at targets
 */
static void update_truck(sObject *self)
{
    Truck *truck;
    sVector d;
	int time;
	int32 range, h, dh;
	World *tgt;
	sVector r;
    Transdata trans;
	Wpt *wpt;
	sObject *o;

    truck = self->data;

	/* Check is truck has reached wpt */
    range = get_range(&truck->p, &truck->wpt->p, &d);
	if ((range < CLOSE) || (range > truck->last_range)) {
		if (truck->wpt->spd > 0)
			truck->speed = truck->wpt->spd;
		truck->aggressive = truck->wpt->active;
		/* Set truck to next wpt */
		wpt = list_next(truck->wpt, truck->path->wpts);
		if (wpt && wpt != Car(truck->path->wpts)) {
			truck->wpt = wpt;
    		range = get_range(&truck->p, &wpt->p, &d);

    		truck->wv.x = 0;
    		truck->wv.y = 0;
    		truck->wv.z = (int)truck->speed;

			truck->r.y = fast_atan(-d.x, d.z);
			setup_trans(&truck->r,  &trans);
    		rot3d(&truck->wv, &truck->wv, &trans);

		} else {
			/* Reached end of path */
			if (truck->path->supply) {
				o = truck->path->supply->value;
				if (o && o->id) {
					message(o, resupply);
				}
			}
			remove_target(self); // remove from inter.c ground targets
			remove_object(self); // remove from main world database
		}
	}

	/* Truck motion and ground following */
	truck->last_range = range;
	truck->p.x += MUL(truck->wv.x, frame_time);
    truck->p.y += MUL(truck->wv.y, frame_time);
    truck->p.z += MUL(truck->wv.z, frame_time);
    h = get_terrain_height(truck->p.x, truck->p.z);
	truck->p.y = h - truck->y_size;

	/* If truck is on RED or BLUE team consider itself a ground target */
	range = get_range(&truck->p, Posn(Player), &d);
	if (truck->flags & TEAM_FLAGS) {
		ground_target(self, range);
	}

	/* Truck cannon shell firing code */
	if (truck->aggressive && (truck->shells > 0)) {
		if (truck->target) {
			tgt = truck->target->data;
			range = get_range(&truck->p, &tgt->p, &d);
			if (truck->predict && (tgt->flags & MOBILE)) {
				time = (int)DIV(range, SHELL_SPEED);
				/* predict target position */
				d.x += MUL(((Mobile *)tgt)->wv.x, time);
				d.y += MUL(((Mobile *)tgt)->wv.y, time);
				d.z += MUL(((Mobile *)tgt)->wv.z, time);
			}
			range >>= SPEED_SHIFT;
			d.x >>= SPEED_SHIFT;
			d.y >>= SPEED_SHIFT;
			d.z >>= SPEED_SHIFT;
			if ((range > Meter(2500)) || (total_time > truck->next_target_time)) {
				truck->target = NULL;
			} else {
				if (total_time > truck->next_fire_time) {
					truck->next_fire_time = total_time + truck->fire_delay;
					truck->shells--;
					r.x = fast_atan(d.y, range) + ((DEG_1 * range) / Meter(600));
					r.y = fast_atan(-d.x, d.z);
					r.z = 0;
					fire_bullet(self, &r, M(10), SHELL_SPEED, 500, FALSE);
				}
			}
		} else if ((truck->flags & TEAM_FLAGS) &&
				(total_time > truck->next_target_time)) {
			truck->next_target_time = total_time + SECS(5);
			if (truck->aagun) {
				truck->target = get_nearest_target(self, TRUE, M(3500), TRUE);
			}
			if (truck->target == NULL) {
				truck->target = get_nearest_target(self, FALSE, M(2500), FALSE);
			}
		}
	}
}

/* ----------------------------------------------------
 * METHOD detaching
 * If truck has target which is detaching itself from the
 * world database set trucks target to NULL
 */
static void truck_detaching(sObject *self, sObject *old)
{
    Truck *o;

    o = self->data;
	if (o->target == old) {
		o->target = NULL;
	}
}

/* ----------------------------------------------------
 * METHOD hit
 * Decrement truck strength, if truck strength falls
 * bellow zero kill truck
 */
static int hit(sObject *self, sObject *collide, int kp)
{
    Truck *o;

    o = self->data;
    if (o->strength > 0) {
		o->strength -= kp;
		if (o->strength <= 0) {
			target_scoring(self, collide);
			SafeMethod(self, World, kill)(self);
		}
	}
	return FALSE;
}


/* ----------------------------------------------------
 * METHOD kill
 * create explosion where truck dies, remove truck from
 * inter.c database and world database.
 */
static void kill(sObject *self)
{
    Truck *o;

    o = self->data;
	create_explo(o->p, zero, o->shape, FIXSPD(8));
	remove_target(self);
	remove_object(self);
}


/* ----------------------------------------------------
 * GLOBAL register_truck
 * Create truck class and setup methods
 */
void register_truck(void)
{
    Truck_class = new_class(sizeof(TruckClass), "Truck", sizeof(Truck), Mobile_class);

    /* Class methods */
    Truck_class->hit       = hit;
    Truck_class->kill      = kill;
    Truck_class->update    = update_truck;
	Truck_class->detaching = truck_detaching;

	SetupMessage(resupply);
}

/* ----------------------------------------------------
 * GLOBAL create_truck
 * Create a new truck object and place it on the path supplied,
 * attach object to world database.
 */
void create_truck(sShape_def *sd, int speed, Path *p, int flags, int strength, int shells, int fire_delay, int predict, int aagun)
{
    sObject *o;
    Truck *t;
    Wpt *w;
	sVector min, max;

    if (sd && p) {
		o = new_object(Truck_class);
		if (o) {
			t = o->data;
			w = Car(p->wpts);
			if (w) {
				t->strength = strength;
				t->shape = sd;
				set_object_radius(o);
				t->p = w->p;
				t->path = p;
				t->wpt = w;
				t->speed = speed;
				t->last_range = M(1000);
				find_size(t->shape, &min, &max);
				t->y_size = min.y;
				t->flags |= flags & TEAM_FLAGS;
				t->shells = shells;
				t->aggressive = FALSE;
				t->fire_delay = fire_delay;
				t->predict = predict;
				t->aagun = aagun;
				t->next_target_time = 0L;
				t->next_fire_time = 0L;
				t->target = NULL;
	  			attach_object(o);
			}
		}
    }
}

