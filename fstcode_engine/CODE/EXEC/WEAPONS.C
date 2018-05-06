/* --------------------------------------------------------------------------
 * EXEC 		Copyright (c) Simis Ltd 1993,994,1995
 * ---			All Rights Reserved
 *
 * File:		weapons.c
 * Ver:			2.00
 * Desc:		Weapons carrying pylon class
 *
 * -------------------------------------------------------------------------- */
#include <stdlib.h>
#include "sound.h"
#include "bangs.h"
#include "explo.h"
#include "bullets.h"
#include "rocket.h"
#include "world.h"
#include "mobile.h"
#include "ground.h"
#include "missile.h"

#include "weapons.h"

#include "defs.h"

#define NUM_PYLONS 7
/* Pylon Numbers
 * -------------
 *        7
 *     4     3
 *   5         2
 * 6		     1
 *
 */

#define MAX_SIDE 2
static int   current_side = 0;
static int   weapon_pylons[MAX_SIDE][MAX_WEAPON_TYPE] = {{7, 4, 5, 6, 5, 5}, {7, 3, 2, 1, 2, 2}};
static int32 pylon_positions[NUM_PYLONS]     = {M(4),M(3),M(2),M(-2),M(-3),M(-4),M(0)};
static int   pylon_angles[NUM_PYLONS]        = {-60,-85,-95,-105,-75,-60,0};

/* Declare Class Pointer */
WeaponsClass *Weapons_class = NULL;

#define MAX_WAVE 5
static int wave_size[MAX_WAVE] = {1, 2, 3, 2, 1};

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
	p->r    = o->r;
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


static void update(sObject *self)
{
	int32 ground_height;
	Pylon   pylon;
	Weapons *o;
	World *carrier;

	o = self->data;

	/* Calculate ccip's
	 * ---------------- */
	switch (o->weapon_selected) {
		case WT_ROCKET:
			carrier = o->carrier->data;
			ground_height = get_terrain_height(carrier->p.x, carrier->p.z);
			calc_rocket_ccip(o->carrier, &ccip, ground_height);
			break;
		case WT_BOMB:
		case WT_CLSTR:
			carrier = o->carrier->data;
			ground_height = get_terrain_height(carrier->p.x, carrier->p.z);
			calc_bomb_ccip(o->carrier, &ccip, ground_height);
			break;
	}

	o->fire_timer -= frame_time;
}

static int cycle(sObject *self)
{
	Weapons *o;
	int      current;

	o = self->data;

	current = o->weapon_selected;
	o->weapon_selected = (o->weapon_selected+1) % MAX_WEAPON_TYPE;
	while (o->n[o->weapon_selected] <= 0 && o->weapon_selected != current)
		o->weapon_selected = (o->weapon_selected+1) % MAX_WEAPON_TYPE;

	if (o->weapon_selected <= 0) {
		o->weapon_selected = WT_GUN;
	}

	/* Set globals */
	set_weapon_selected(o->weapon_selected);
	weapon_n = o->n[o->weapon_selected];

	switch_sound();
	return o->weapon_selected;
}

static int fire(sObject *self)
{
	static long int pylon_offset = M(4);
	static sVector offset = {M(0), M(0), M(0)};
    sVector r, dr;
    Transdata t;
	Weapons *o;
	sObject *carrier;
	Mobile *c;
	Pylon  pylon;

	o = self->data;
	if (o->fire_timer > 0)
	    return FALSE;

	carrier = o->carrier;
	c = carrier->data;
	switch (o->weapon_selected) {
		case WT_GUN:
			if (o->n[WT_GUN] > 0) {
    			fire_bullet(carrier, &c->r, M(5), c->speed+FIXSPD(1200), SECS(7), TRUE);
				o->fire_timer = 5L;
				cannon_noise();
				o->n[WT_GUN] -= 1;
				weapon_n = o->n[WT_GUN];
			}
			break;
		case WT_ROCKET:
			if (o->n[WT_ROCKET] > 0) {
				fire_noise(40);
				calc_pylon(carrier, weapon_pylons[0][WT_ROCKET], &pylon);
				fire_rocket(carrier, &pylon, o->kp[WT_ROCKET]);
				calc_pylon(carrier, weapon_pylons[1][WT_ROCKET], &pylon);
				fire_rocket(carrier, &pylon, o->kp[WT_ROCKET]);
				o->n[WT_ROCKET] -= 2;
				o->fire_timer = 100L;
			}
			break;
		case WT_AA:
			if ((o->n[WT_AA] > 0) && selected) {
   				calc_pylon(carrier, weapon_pylons[current_side][WT_AA], &pylon);
				fire_noise(20);
				add_aa(carrier, &pylon, selected, o->kp[WT_AA]);
				o->n[WT_AA] -= 1;
				o->fire_timer = 100L;
				current_side = !current_side;
			}
			break;
		case WT_AG:
			if ((o->n[WT_AG] > 0) && selected) {
   				calc_pylon(carrier, weapon_pylons[current_side][WT_AG], &pylon);
				fire_noise(20);
				add_ag(carrier, &pylon, selected, o->kp[WT_AG]);
				o->n[WT_AG] -= 1;
				o->fire_timer = 100L;
				current_side = !current_side;
			}
			break;
		case WT_BOMB:
			if (o->n[WT_BOMB] > 0) {
   				calc_pylon(carrier, weapon_pylons[current_side][WT_BOMB], &pylon);
				clunk_sound();
				launch_bomb(carrier, &pylon, o->kp[WT_BOMB]);
				o->n[WT_BOMB] -= 1;
				o->fire_timer = 100L;
				current_side = !current_side;
			}
			break;
		case WT_CLUSTER:
			if (o->n[WT_CLUSTER] > 0) {
   				calc_pylon(carrier, weapon_pylons[current_side][WT_CLUSTER], &pylon);
				launch_cluster_bomb(carrier, &pylon, o->kp[WT_CLUSTER]);
				o->n[WT_CLUSTER] -= 1;
				o->fire_timer = 100L;
				clunk_sound();
				current_side = !current_side;
			}
			break;
		case WT_TORPEDO:
			if (o->n[WT_TORPEDO] > 0) {
   				calc_pylon(carrier, weapon_pylons[current_side][WT_TORPEDO], &pylon);
				clunk_sound();
				launch_torpedo(carrier, &pylon, o->kp[WT_TORPEDO]);
				o->n[WT_TORPEDO] -= 1;
				o->fire_timer = 100L;
				current_side = !current_side;
			}
			break;
	}

	/* Set globals */
	weapon_n = o->n[o->weapon_selected];

	return TRUE;
}

void rearm(sObject *self, int type, int n, int kp)
{
	Weapons *o;

	o = self->data;
	if (type >= 0 && type < MAX_WEAPON_TYPE) {
		o->n[type] = n;
		if (type == o->weapon_selected)
			weapon_n = o->n[type];
		if (kp > 0)
			o->kp[type] = kp;
	}
}

int draw(sObject *o)
{
  /* The weapons system is not drawn
   */
    return FALSE;
}

int hit(sObject *o, sObject *c, int kp)
{
  /* The weapons system is invisible to collisions
   */
    return 0;
}

void register_weapons(void)
{
    Weapons_class = new_class(sizeof(WeaponsClass), "Weapons", sizeof(Weapons), World_class);
	Weapons_class->system_update = moving_system_update;
	Weapons_class->update = update;
	Weapons_class->rearm = rearm;
	Weapons_class->fire  = fire;
	Weapons_class->draw  = draw;
	Weapons_class->cycle = cycle;
	Weapons_class->check_collide = NULL; /* Weapon system cannot collide with anything */
	Weapons_class->hit   = hit; /* Weapon system cannot collide with anything */
}

sObject *new_weapons_system(sObject *carrier)
{
  sObject  *o;
  Weapons *w;
  int i;

  o = new_object(Weapons_class);
  if (o) {
	  w = o->data;
	  w->shape = NULL;
	  w->carrier 	= carrier;
	  w->weapon_selected = WT_GUN;
	  for (i = 0; i < MAX_WEAPON_TYPE; i++) {
	  	w->n[i]  	= 0;
		w->kp[i]	= 1;
	  }

	  attach_object(o);
	  return o;
  }
  return NULL;
}
