/* --------------------------------------------------------------------------
 * EXEC 		Copyright (c) Simis Ltd 1993,994,1995
 * ---			All Rights Reserved
 *
 * File:		missile.c
 * Ver:			2.00
 * Desc:        Missile class, AG class and AA class
 * 
 * Implements generic guided missile class and sibling Air to ground
 * and Air to Air classes.
 *
 * -------------------------------------------------------------------------- */
#include <stdlib.h>
#include "aircraft.h"
#include "missile.h"
#include "defs.h"
#include "sound.h"
#include "world.h"
#include "ground.h"

#include "flare.h"

/* Declare Class Pointer */
MissileClass *Missile_class = NULL;
MissileClass *AG_class = NULL;
MissileClass *AA_class = NULL;

/* SAM code */
#define SAM_LAUNCH M(8000)
#define SAM_THRUST 30
#define SAM_SPEED FIXSPD(20)
#define SAM_MAX_SPEED FIXSPD(1500)
#define SAM_TURN_RATE (DEG(1) / 12)
#define SAM_LIFE 1500

#define MISSILE_THRUST 50
#define MISSILE_SPEED FIXSPD(10)
#define MISSILE_MAX_SPEED FIXSPD(1500)

#define SW_TURN_RATE (DEG(1) / 4)
#define SW_LIFE 1500
#define SW_SPEED FIXSPD(10)

#define MAVERICK_TURN_RATE (DEG(1) / 16)
#define MAVERICK_LIFE 2000
#define MAVERICK_SPEED FIXSPD(10)

int chaff_value = 0;
extern void ac_axis_rotate(sVector *r, sVector *dr);

static void fly_sam(sObject *self)
{
	Missile *o;
    int32 range, height;
    int bearing, abs_bearing, elev, abs_elev, turn_rate, time;
    Mobile *target;
    sVector d, v;
    sVector sv;
    Transdata trans;

	o = self->data;
    if (o->fuel > 0) {
    	if (o->speed < SAM_MAX_SPEED) {
        	o->speed += SAM_THRUST * frame_time;
			if ((o->count -= frame_time) <= 0) {
				o->count = 20;
	        	/* trail_smoke(&s->op); */
			}
		} else {
        	o->fuel = 0;
		}
	}
    setup_trans(&o->r, &trans);
	if (o->target) {
		target = o->target->data;
    	if (target->flags & ATTACHED) {
			if ((simulation_time - o->fire_time) > 60L) {
    			v.x = target->p.x;
    			v.y = target->p.y + ((int32)(target->shape->s->max_size.y + target->shape->s->min_size.y) << (target->shape->s->scale - 1));
    			v.z = target->p.z;
				if (chaff_value <= 0) {
    				d.x = v.x - o->p.x;
    				d.y = v.y - o->p.y;
    				d.z = v.z - o->p.z;
    				range = magnitude(&d);
					time = DIV(range, SAM_MAX_SPEED);
    				v.x += MUL(target->wv.x, time);
    				v.y += MUL(target->wv.y, time);
    				v.z += MUL(target->wv.z, time);
				}
    			d.x = (v.x - o->p.x) >> SPEED_SHIFT;
    			d.y = (v.y - o->p.y) >> SPEED_SHIFT;
    			d.z = (v.z - o->p.z) >> SPEED_SHIFT;
    			range = magnitude(&d);

    			if (((range > o->last_range) && (range < Meter(50))) || (range > Meter(30000))) {
					remove_object(self);
				} else {
					o->last_range = range;
        			invrot3d(&d, &d, &trans);

        			bearing       = fast_atan(-d.x, d.z);
        			abs_bearing   = ABS(bearing);
        			elev          = fast_atan(d.y, d.z);
        			abs_elev      = ABS(elev);

        			turn_rate = o->turn_rate * frame_time;
    				if (chaff_value > 0)
        				turn_rate = SR(turn_rate, 1);

    				if (abs_elev < DEG(2)) {
        				sv.x = (elev * frame_time) >> 3;
    				} else if (abs_elev < DEG(80)) {
        				if (d.y > 0)
            				sv.x = turn_rate * frame_time;
        				else
            				sv.x = -turn_rate * frame_time;
    				} else {
        				sv.x = turn_rate * frame_time;
    				}
    				if (abs_bearing < DEG(2)) {
        				sv.y = (bearing * frame_time) >> 3;
    				} else if (abs_bearing < DEG(80)) {
        				if (d.x < 0)
            				sv.y = turn_rate * frame_time;
        				else
            				sv.y = -turn_rate * frame_time;
    				} else {
        				sv.y = 0;
    				}
    				sv.z = 0;

        			ac_axis_rotate(&o->r, &sv);
    			}
    		}
		}
	}

	sv.x = 0; sv.y = 0; sv.z = o->speed;
    rot3d(&sv, &o->wv, &trans);
    o->op = o->p;
    o->p.x += MUL(o->wv.x, frame_time);
    o->p.y += MUL(o->wv.y, frame_time);
    o->p.z += MUL(o->wv.z, frame_time);

	height = get_terrain_height(o->p.x, o->p.z);
	if (((simulation_time - o->fire_time) > SAM_LIFE) || (o->p.y < height)) {
		remove_object(self);
    }
}

static sObject *new_sam(void)
{
    sObject *o;
	Missile *m;

	o = new_object(Missile_class);
    if (o) {
		m = o->data;
        m->shape  = get_shape("ROCKET.FSD");
        m->speed  = FIXSPD(1000);
        m->fuel   = 50; /* ~ 5 sec burn */
		m->count  = 0;
        m->target = NULL;
    }
    return o;
}

void add_sam(sObject *site, sObject *tgt, int kp)
{
    int32 range;
    sObject *o;
	Missile *m;
    sVector d;
	sShape *s;

    o = new_sam();
    if (o) {
		m = o->data;
		get_range(Posn(tgt), Posn(site), &d);
        d.x >>= SPEED_SHIFT;
        d.y >>= SPEED_SHIFT;
        d.z >>= SPEED_SHIFT;
        /* range in sam coords  >> SPEED_SHIFT */
        range = magnitude(&d);
        m->r.y = fast_atan(-d.x, d.z);
        m->r.x = DEG(50);
        m->r.z = 0;

		/* s = site->gnode; */
		m->p = *Posn(site);
        m->p.y += /*((int32)g->s->max_size.y << g->s->scale)+*/ M(20);
        m->op = m->p;
        m->speed = SAM_SPEED;
        m->count = SAM_LIFE;
		m->turn_rate = SAM_TURN_RATE;
		m->fire_time = simulation_time;
		m->last_range = Meter(100000);
        m->firer = site;
        m->target = tgt;
		m->kp = kp;

        attach_object(o);

		if (tgt == Player)
        	maw_sound();
    }
}


/* Missile code
 * ------------
 */
void check_flares(sObject *self)
{
	sObject *flare;
	Missile *missile;
	int bearing, elev, min_bearing, min_elev;
	int32 range;
	sList *l;
	sVector d;
    Transdata trans;

	min_bearing = DEG(45);
	min_elev 	= DEG(45);
	missile 	= self->data;
	for (l = Flare_class->active_flares->next; l != Flare_class->active_flares; l = l->next) {
		flare = l->item;
    	d.x = (Posn(flare)->x - missile->p.x) >> SPEED_SHIFT;
    	d.y = (Posn(flare)->y - missile->p.y) >> SPEED_SHIFT;
    	d.z = (Posn(flare)->z - missile->p.z) >> SPEED_SHIFT;
    	range = magnitude(&d);

        setup_trans(&missile->r, &trans);
        invrot3d(&d, &d, &trans);
        bearing	= fast_atan(-d.x, d.z);
		bearing = ABS(bearing);
        elev 	= fast_atan(d.y, d.z);
		elev 	= ABS(elev);
        if ((bearing < DEG(20)) && (elev < DEG(20))) {
			if ((bearing < min_bearing) && (elev < min_elev)) {
				min_bearing 	= bearing;
				min_elev 		= elev;
				missile->target = flare;
			}
		}
	}
}

void fly_missile(sObject *self)
{
	Missile *o;
    int32 range, height;
    int bearing, abs_bearing, elev, abs_elev, turn_rate;
    sObject *target;
	World  *tgt_data;
	Aircraft *air_tgt;

	sShape_def *ts;
    sVector d, v;
    sVector sv;
    Transdata trans;

	o = self->data;
    if (o->fuel > 0) {
    	if (o->speed < MISSILE_MAX_SPEED) {
        	o->speed += MISSILE_THRUST * frame_time;
		} else {
        	o->fuel = 0;
		}
		/* if ((o->count -= frame_time) <= 0) {
			o->count = 10;
	        trail_smoke(&o->op);
		}*/
	}
    setup_trans(&o->r, &trans);
    target = o->target;
	if (target) {
		/* Check flares if missile is SW and target is not flare */
		if (self->class == AA_class && o->target->class != Flare_class) {
			check_flares(self);
			target = o->target;
		}
		tgt_data = target->data;

    	if (!(tgt_data->flags & DETACHING)) {
			if ((simulation_time - o->fire_time) > 30L) {
				ts = tgt_data->shape;
				if (target->class == Aircraft_class) {
					air_tgt = target->data;
					v.x = air_tgt->p.x + MUL(air_tgt->wv.x, frame_time << 2);
    				v.y = air_tgt->p.y + MUL(air_tgt->wv.y, frame_time << 2);
    				v.z = air_tgt->p.z + MUL(air_tgt->wv.z, frame_time << 2);
				} else {
    				v.x = tgt_data->p.x;
    				v.y = tgt_data->p.y;
    				v.z = tgt_data->p.z;
				}

    			d.x = (v.x - o->p.x) >> SPEED_SHIFT;
    			d.y = (v.y - o->p.y) >> SPEED_SHIFT;
    			d.z = (v.z - o->p.z) >> SPEED_SHIFT;
    			/* range in world co-ords  >> SPEED_SHIFT */
    			range = magnitude(&d);

    			if (((range > o->last_range) && (range < Meter(50))) || (range > Meter(30000))) {
					SafeMethod(self, Missile, kill)(self);
				} else {
					o->last_range = range;
        			invrot3d(&d, &d, &trans);

        			bearing       = fast_atan(-d.x, d.z);
        			abs_bearing   = ABS(bearing);
        			elev          = fast_atan(d.y, d.z);
        			abs_elev      = ABS(elev);

        			if ((abs_bearing < DEG(30)) && (abs_elev < DEG(30))) {
            			turn_rate = o->turn_rate * frame_time;
            			if (abs_elev < DEG(5)) {
                			sv.x = (int)SR(MUL(elev, frame_time), 3);
            			} else {
                			if (d.y > 0)
                    			sv.x = turn_rate;
                			else
                    			sv.x = -turn_rate;
            			}
            			if (abs_bearing < DEG(5)) {
                			sv.y = (int)SR(MUL(bearing, frame_time), 3);
            			} else {
                			if (d.x < 0)
                    			sv.y = turn_rate;
                			else
                    			sv.y = -turn_rate;
            			}
            			sv.z = 0;
              			ac_axis_rotate(&o->r, &sv);
    				}
    			}
    		}
		}
	}

	if (!(o->flags & DETACHING)) {
		sv.x = 0; sv.y = 0; sv.z = o->speed;
    	rot3d(&sv, &o->wv, &trans);
    	o->op = o->p;
    	o->p.x += MUL(o->wv.x, frame_time);
    	o->p.y += MUL(o->wv.y, frame_time);
    	o->p.z += MUL(o->wv.z, frame_time);

		height = get_terrain_height(o->p.x, o->p.z);
		if (((simulation_time - o->fire_time) > SW_LIFE) || (o->p.y < height)) {
			SafeMethod(self, Missile, kill)(self);
		}
    }
}

static sObject *new_missile(MissileClass *class)
{
    sObject *o;
	Missile *m;

	o = new_object(class);
    if (o) {
		m = o->data;
        m->shape  = get_shape("ROCKET.FSD");
        m->speed  = FIXSPD(1000);
        m->fuel   = 50; /* ~ 5 sec burn */
		m->count  = 0;
        m->target = NULL;
    }
    return o;
}

void add_ag(sObject *from, Pylon *p, sObject *tgt, int kp)
{
    sObject  *o;
	Missile *m;
	sShape   *s;
	sVector v;
    Transdata trans;

    o = new_missile(AG_class);
    if (o) {
		m = o->data;

		m->p  			= p->p;
        m->op 			= p->p;
		m->r  			= p->r;
		m->wv 			= p->wv;
        m->speed 		= p->speed + MAVERICK_SPEED;
        m->count 		= MAVERICK_LIFE;
		m->turn_rate    = MAVERICK_TURN_RATE;
		m->fire_time 	= simulation_time;
		m->last_range 	= Meter(100000);
        m->firer 		= from;
        m->target 		= tgt;
		m->kp 			= kp;

		/* Add initial missile speed */
        setup_trans(&p->r, &trans);
        v.x = v.y = 0;
        v.z = MAVERICK_SPEED;
        rot3d(&v, &v, &trans);
		m->wv.x += v.x;
		m->wv.y += v.y;
		m->wv.z += v.z;

		if (missile_symbol->value != viewer)
			missile_symbol->value = o;

        attach_object(o);
    }
}

void add_aa(sObject *from, Pylon *p, sObject *tgt, int kp)
{
    sObject  *o;
	Missile *m;
	sShape   *s;
	sVector v;
    Transdata trans;

    o = new_missile(AA_class);
    if (o) {
		m = o->data;

		m->p  			= p->p;
        m->op 			= p->p;
		m->r  			= p->r;
		m->wv 			= p->wv;
        m->speed 		= p->speed + SW_SPEED;
        m->count 		= SW_LIFE;
		m->turn_rate    = SW_TURN_RATE;
		m->fire_time 	= simulation_time;
		m->last_range 	= Meter(100000);
        m->firer 		= from;
        m->target 		= tgt;
		m->kp 			= kp;

		/* Add initial missile speed */
        setup_trans(&p->r, &trans);
        v.x = v.y = 0;
        v.z = SW_SPEED;
        rot3d(&v, &v, &trans);
		m->wv.x += v.x;
		m->wv.y += v.y;
		m->wv.z += v.z;

		if (missile_symbol->value != viewer)
			missile_symbol->value = o;

        attach_object(o);
    }
}

static void missile_detaching(sObject *self, sObject *old)
{
	Missile *o;

    o = self->data;
	if (o->target == old) {
		o->target = NULL;
	}
}

void register_missile(void)
{
    Missile_class = new_class(sizeof(MissileClass), "Missile", sizeof(Missile), Rocket_class);

	Missile_class->update = fly_sam;
	Missile_class->detaching = missile_detaching;

    AG_class = copy_class(Missile_class, "AG");
    AG_class->update = fly_missile;

    AA_class = copy_class(Missile_class, "AA");
    AA_class->update = fly_missile;
}

