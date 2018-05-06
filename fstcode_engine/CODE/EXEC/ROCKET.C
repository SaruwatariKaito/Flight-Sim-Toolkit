/* --------------------------------------------------------------------------
 * EXEC 		Copyright (c) Simis Ltd 1993,994,1995
 * ---			All Rights Reserved
 *
 * File:		rocket.c
 * Ver:			2.00
 * Desc:		Rocket, Bomb, Cluster and Torpedo classes
 *
 * Rocket is unguided object with 15 seconds of thrust directed
 * down axis - simple flight modeled
 * 
 * Bomb is unguided object which free falls under gravity - no flight
 * model
 * 
 * Cluster is large number of small bombs
 * 
 * Torpedo - modeled in two sections 1 - free fall to zero height, 2 - 
 * "fly" through sea checking for impact with ships.
 * 
 * -------------------------------------------------------------------------- */
#include <stdlib.h>
#include "rocket.h"
#include "ground.h"
#include "bangs.h"
#include "explo.h"
#include "sound.h"

#include "defs.h"

/* Declare Class Pointer */
RocketClass *Rocket_class = NULL;
RocketClass *Bomb_class = NULL;
RocketClass *Cluster_class = NULL;
RocketClass *Torpedo_class = NULL;

#define ROCKET_SPEED  FIXSPD(50)
#define ROCKET_MAX    FIXSPD(1500)
#define ROCKET_THRUST 50
#define ROCKET_TIMEOUT SECS(15)

#define TORPEDO_TIMEOUT SECS(200)

extern void ac_axis_rotate(sVector *r, sVector *dr);


/* --------------------
 * GLOBAL calc_rocket_ccip 
 * 
 * Calculate predicted impact point of object with
 * ground
 */ 
void calc_rocket_ccip(sObject *self, sVector *ccip, int32 ground_height)
{
    Rocket *o;
	int speed, fuel, count, gravity;
	int32 wvx, wvz;
	sVector p;
    sVector v, wv, dr;
    Transdata trans;

	o = self->data;
    setup_trans(&o->r, &trans);
    v = o->r;
    dr.x = 0 /* pylon_angles[2] */;
    dr.y = dr.z = 0;
    ac_axis_rotate(&v, &dr);
    setup_trans(&v, &trans);
    p = o->p;

	wv = o->wv;
    v.x = v.y = 0;
    speed = v.z = o->speed + ROCKET_SPEED;
    rot3d(&v, &v, &trans);
	wv.x += v.x;
	wv.y += v.y;
	wv.z += v.z;

	gravity = (GRAVITY * frame_time);
	count = ROCKET_TIMEOUT;
    fuel = 20; /* ~ 0.2 sec burn */
    v.x = v.y = 0;
	v.z = ROCKET_THRUST * frame_time;
    rot3d(&v, &v, &trans);
    while ((fuel > 0) && (speed < ROCKET_MAX) && (p.y > ground_height) && ((count -= frame_time) >= 0)) {
        speed += v.z;
        wv.x += v.x;
		wv.y += v.y;
		wv.z += v.z;

		wv.y -= gravity;
    	p.x += MUL(wv.x, frame_time);
    	p.y += MUL(wv.y, frame_time);
    	p.z += MUL(wv.z, frame_time);
    }
	wvx = MUL(wv.x, frame_time);
	wvz = MUL(wv.z, frame_time);
    while ((p.y > ground_height) && ((count -= frame_time) >= 0)) {
		wv.y -= gravity;
    	p.x += wvx;
    	p.y += MUL(wv.y, frame_time);
    	p.z += wvz;
    }
	*ccip = p;
}


/* --------------------
 * METHOD update_rocket
 * 
 * Models rocket under gravity
 */ 
static void update_rocket(sObject *self)
{
    Rocket *o;
    sVector v;
    Transdata trans;

	o = self->data;
    if (o->fuel > 0) {
    	if (o->speed < ROCKET_MAX) {
        	setup_trans(&o->r, &trans);
        	v.x = 0; v.y = 0; v.z = ROCKET_THRUST * frame_time;
        	o->speed += v.z;
        	rot3d(&v, &v, &trans);
        	o->wv.x += v.x;
			o->wv.y += v.y;
			o->wv.z += v.z;
		} else {
        	o->fuel = 0;
		}
	}
    o->wv.y -= (GRAVITY * frame_time);
    o->op = o->p;
    o->p.x += MUL(o->wv.x, frame_time);
    o->p.y += MUL(o->wv.y, frame_time);
    o->p.z += MUL(o->wv.z, frame_time);

    if ((o->life -= frame_time) < 0) {
		SafeMethod(self, Rocket, kill)(self);
    }

    if (o->p.y < get_terrain_height(o->p.x, o->p.z)) {
		SafeMethod(self, Rocket, kill)(self);
    }
}

static sObject *new_rocket(void)
{
    sObject *o;
	Rocket *r;

	o = new_object(Rocket_class);
    if (o) {
		r = o->data;
        r->shape  = get_shape("ROCKET.FSD");
        r->life   = ROCKET_TIMEOUT;
		r->kp     = 5;
        r->fuel   = 20; /* ~ 2 sec burn */
    }
    return o;
}


/* -------------------
 * GLOBAL fire_rocket
 * 
 * Create rocket object in world
 */
void fire_rocket(sObject *from, Pylon *p, int kp)
{
    sObject *o;
	Rocket *r;
	sVector v;
    Transdata t;

    o = new_rocket();
    if (o) {
		r = o->data;

		r->p  = p->p;
        r->op = p->p;
        r->r  = p->r;

		r->wv = p->wv;
        v.x = v.y = 0;
        r->speed = v.z = p->speed + ROCKET_SPEED;
        setup_trans(&p->r, &t);
        rot3d(&v, &v, &t);
		r->wv.x += v.x;
		r->wv.y += v.y;
		r->wv.z += v.z;

        r->firer = from;
		r->kp = kp;

		/* Global symbol referencing most recently created missile */
		if (missile_symbol->value != viewer)
			missile_symbol->value = o;
        attach_object(o);
    }
}


/* --------------------
 * METHOD collide
 * 
 * Called by rocket check_collide method,
 * Returns number of kp caused by collision between self and
 * object c - default 1
 */ 
static int collide(sObject *self, sObject *c)
{
	Rocket *r;
	World  *o;
	sShape  *s;

	r = self->data;
	o = c->data;

	if (r->firer == c || (c->class == Bomb_class))
		 return 0;

	if (r->shape && o->shape) {
		s = o->shape->s;
		if (check_collision(self, s, &o->p, &o->r, FALSE)) {
			if (r->firer == Player)
				player_scored_hit(self);
			SafeMethod(self, Rocket, kill)(self);
			return r->kp;
		}
	}
	return 0;
}


/* --------------------
 * METHOD hit
 * 
 * A moving object has collided with the rocket 
 * 
 */ 
static int hit(sObject *self, sObject *collide, int kp)
{
    Rocket *o;

    o = self->data;

	if (o->firer == collide)
		  return 0;

	SafeMethod(self, Rocket, kill)(self);

	return o->kp;
}


/* --------------------
 * METHOD kill
 * 
 * Kills rocket object
 */ 
static void kill(sObject *o)
{
    Rocket *r;

    r = o->data;
	if ((r->flags & DETACHING) == 0) {
		medium_ground_bang(Posn(o));
		explo1_sound();
    	remove_object(o);
	}
}



/* --------------------
 * METHOD detach
 * 
 * sets missile_symbol to NULL and resets viewer
 */ 
static int detach(sObject *self)
{
	Rocket *r;

	if (self == missile_symbol->value)
			missile_symbol->value = NULL;
	if (self == viewer)
			control_set_viewer(Player);

	return world_detach(self);
}

/* -------------------
 * GLOBAL rocket_viewer
 * 
 * Called when the tracker receives the view focus or
 * when look_XXX changes
 */
void rocket_viewer(sObject *self)
{
  change_cockpit(NULL);
  set_viewer(self);
}

static void rocket_detaching(sObject *self, sObject *old)
{
	Rocket *o;

    o = self->data;
	if (o->firer == old) {
		o->firer = NULL;
	}
}

static void update_bomb(sObject *self);
static void update_cluster(sObject *self);
static void update_torpedo(sObject *self);


/* -------------------
 * GLOBAL register_rocket
 * 
 */
void register_rocket(void)
{
    Rocket_class = new_class(sizeof(RocketClass), "Rocket", sizeof(Rocket), Mobile_class);

	Rocket_class->update = update_rocket;
	Rocket_class->collide = collide;
	Rocket_class->kill    = kill;
	Rocket_class->detach = detach;
	Rocket_class->detaching = rocket_detaching;

	add_message(Rocket_class, "become_viewer", 0, rocket_viewer);

	Bomb_class = copy_class(Rocket_class, "Bomb");
	Bomb_class->update = update_bomb;

	Cluster_class = copy_class(Bomb_class, "Cluster");
	Cluster_class->update = update_cluster;

	Torpedo_class = copy_class(Rocket_class, "Torpedo");
	Torpedo_class->update = update_torpedo;
}

void calc_bomb_ccip(sObject *self, sVector *ccip, int32 ground_height)
{
	Rocket *o;
	int32 vx, vy, vz;
	sVector p;
    sVector wv;

	o = self->data;
    p  = o->p;
    wv = o->wv;
    vx = (int32)wv.x;
    vy = (int32)wv.y;
    vz = (int32)wv.z;
    while (p.y > ground_height) {
		vy -= GRAVITY;
        p.x += vx;
        p.y += vy;
        p.z += vz;
    }
    *ccip = p;
}

static void update_bomb(sObject *self)
{
	Rocket *o;
	sObject *target;

	o = self->data;
	o->wv.y -= (GRAVITY * frame_time);
    o->op = o->p;
    o->p.x += MUL(o->wv.x, frame_time);
    o->p.y += MUL(o->wv.y, frame_time);
    o->p.z += MUL(o->wv.z, frame_time);
    o->r.x = fast_atan(o->wv.y, o->speed);

    if (o->p.y < get_terrain_height(o->p.x, o->p.z)) {
        big_ground_bang(&o->op);
        SafeMethod(self, Rocket, kill)(self);
    }
}

static sObject *new_bomb(void)
{
    sObject *o;
	Rocket *r;

	o = new_object(Bomb_class);
    if (o) {
		r = o->data;
        r->shape = get_shape("BOMB.FSD");
        r->life   = ROCKET_TIMEOUT;
		r->kp     = 5;
        r->fuel   = 20; /* ~ 2 sec burn */
    }
    return o;
}

void launch_bomb(sObject *from, Pylon *p, int kp)
{
    sObject *bomb;
	Rocket *o;
    Transdata t;

    bomb = new_bomb();
    if (bomb) {
		o = bomb->data;
        o->p  		= p->p;
        o->op 		= p->p;
        o->r.x 		= p->r.x;
        o->r.y 		= p->r.y;
		o->r.z 		= 0;
        o->wv 		= p->wv;
        o->speed 	= p->speed;
		o->kp 		= kp;

        o->firer    = from;
        attach_object(bomb);
		if (missile_symbol->value != viewer)
			missile_symbol->value = bomb;
    }
}

#define BOMB_SPACE M(3L)
void new_cluster_bombs(sVector *p, sVector *wv, int32 speed, int32 dz, int wave_size)
{
	sObject *bomb;
	int32  offset;
	sVector d;
	Rocket *o;

	offset = -(wave_size-1)*BOMB_SPACE/2;
	while (wave_size--) {
    	bomb = new_bomb();
    	if (bomb) {
			d.x = offset;
			d.y = 0;
			d.z = dz;

			o = bomb->data;
        	o->shape = get_shape("CLUSTER.FSD");
	        o->p.x  	= p->x + d.x;
	        o->p.y  	= p->y + d.y;
	        o->p.z  	= p->z + d.z;
        	o->op 		= o->p;
        	o->wv 		= *wv;
        	o->speed 	= speed;

        	o->firer    = NULL;
        	attach_object(bomb);
			if (missile_symbol->value != viewer)
				missile_symbol->value = bomb;
    	}
		offset += BOMB_SPACE;
	}
}


static void update_cluster(sObject *self)
{
	Rocket *o;
	sObject *target;

	o = self->data;
	o->wv.y -= (GRAVITY * frame_time);
    o->op = o->p;
    o->p.x += MUL(o->wv.x, frame_time);
    o->p.y += MUL(o->wv.y, frame_time);
    o->p.z += MUL(o->wv.z, frame_time);
    o->r.x = fast_atan(o->wv.y, o->speed);

    if (o->p.y < 0) {
        big_ground_bang(&o->p);
        SafeMethod(self, Rocket, kill)(self);
    }

	if ((o->life -= frame_time) < 0L) {
		/* Add cluster bombs */
		new_cluster_bombs(&o->p, &o->wv, o->speed, -BOMB_SPACE*2, 2);
		new_cluster_bombs(&o->p, &o->wv, o->speed, -BOMB_SPACE,   3);
		new_cluster_bombs(&o->p, &o->wv, o->speed,          M(0), 4);
		new_cluster_bombs(&o->p, &o->wv, o->speed, +BOMB_SPACE,   3);
		new_cluster_bombs(&o->p, &o->wv, o->speed, +BOMB_SPACE*2, 2);
		remove_object(self);
	}
}

#define CLUSTER_TIMEOUT SECS(2)
static sObject *new_cluster(void)
{
    sObject *o;
	Rocket *r;

	o = new_object(Cluster_class);
    if (o) {
		r = o->data;
        r->shape = get_shape("BOMB.FSD");
        r->life   = CLUSTER_TIMEOUT;
		r->kp     = 5;
    }
    return o;
}

void launch_cluster_bomb(sObject *from, Pylon *p, int kp)
{
    sObject *bomb;
	Rocket *o;
    Transdata t;

    bomb = new_cluster();
    if (bomb) {
		o = bomb->data;
        o->p  		= p->p;
        o->op 		= p->p;
        o->r.x 		= p->r.x;
        o->r.y 		= p->r.y;
		o->r.z 		= 0;
        o->wv 		= p->wv;
        o->speed 	= p->speed;
        o->firer    = from;
		o->kp 		= kp;

        attach_object(bomb);
    }
}

static void update_torpedo(sObject *self)
{
	Rocket *o;
	sObject *target;
	int32 ground_height;
    sVector v;
    Transdata trans;

	o = self->data;
	if (o->p.y > 0) {
		o->wv.y -= (GRAVITY * frame_time);
    	o->op = o->p;
    	o->p.x += MUL(o->wv.x, frame_time);
    	o->p.y += MUL(o->wv.y, frame_time);
    	o->p.z += MUL(o->wv.z, frame_time);
    	o->r.x = fast_atan(o->wv.y, o->speed);

    	ground_height = get_terrain_height(o->p.x, o->p.z);
    	if (o->p.y < ground_height) {
			if ((ground_height > 0) || (o->speed > FIXSPD(140)) || (o->wv.y < FIXSPD(-60))) {
        		big_ground_bang(&o->op);
        		SafeMethod(self, Rocket, kill)(self);
			} else {
    			o->r.x = 0;
    			setup_trans(&o->r, &trans);
				o->speed = FIXSPD(60);
    			v.x = 0; v.y = 0; v.z = o->speed;
    			rot3d(&v, &o->wv, &trans);
				o->wv.y = 0;
			}
    	}
    } else {
    	o->op = o->p;
    	o->p.x += MUL(o->wv.x, frame_time);
    	o->p.y = 0;
    	o->p.z += MUL(o->wv.z, frame_time);

    	ground_height = get_terrain_height(o->p.x, o->p.z);
		if (ground_height > 0) {
        	big_ground_bang(&o->op);
        	SafeMethod(self, Rocket, kill)(self);
		}
    }
    if ((o->life -= frame_time) < 0) {
		SafeMethod(self, Rocket, kill)(self);
    }
}

static sObject *new_torpedo(void)
{
    sObject *o;
	Rocket *r;

	o = new_object(Torpedo_class);
    if (o) {
		r = o->data;
        r->shape = get_shape("TORPEDO.FSD");
        r->life   = TORPEDO_TIMEOUT;
		r->kp     = 10;
        r->fuel   = 20;
    }
    return o;
}

void launch_torpedo(sObject *from, Pylon *p, int kp)
{
    sObject *torpedo;
	Rocket *o;
    Transdata t;

    torpedo = new_torpedo();
    if (torpedo) {
		o = torpedo->data;
        o->p  		= p->p;
        o->op 		= p->p;
        o->r.x 		= p->r.x;
        o->r.y 		= p->r.y;
		o->r.z 		= 0;
        o->wv 		= p->wv;
        o->speed 	= p->speed;
		o->kp 		= kp;

        o->firer    = from;
        attach_object(torpedo);
		if (missile_symbol->value != viewer)
			missile_symbol->value = torpedo;
    }
}

