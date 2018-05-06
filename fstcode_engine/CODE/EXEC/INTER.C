/* --------------------------------------------------------------------------
 * EXEC 		Copyright (c) Simis Ltd 1993,994,1995
 * ---			All Rights Reserved
 *
 * File:		inter.c
 * Ver:			2.00
 * Desc:		Interaction between objects in game
 *
 * Implements three lists:
 *
 * radar_targets - objects which can be seen by aircraft radar
 *
 * air_targets - objects which can be targeted by AA weapons
 *
 * ground_targets - objects which can be targeted by AG weapons
 *
 * All objects are responsible for adding/removing themselves from the
 * inter lists.
 *
 * -------------------------------------------------------------------------- */
/* FST Interaction control
 * ----------------------- */

#include <stdio.h>

#include "world.h"
#include "ground.h"
#include "text.h"
#include "defs.h"
#include "sound.h"

#include "rocket.h"

int weapon_type		= WT_GUN;
int weapon_n		= 0;
int weapon_lock		= FALSE;
int weapon_track	= W_NOTRACK;

/* Currently selected target */
sObject *selected = NULL;

static int32 selected_time 		= 0L;
static int32 target_check_time 	= 0L;

int selected_range = 0;

static sList *radar_targets 	= NULL;
static sList *air_targets 	= NULL;
static sList *ground_targets = NULL;

#define RADAR_MARK GREEN
#define RADAR_BACK BLACK
static int radar_scale = 1;


/* --------------------
 * LOCAL draw_targets
 *
 * Draw radar dots
 */
static void draw_targets(int cx, int cy, int heading, sList *targets, int col)
{
	int32 dx, dz;
	int i, dist, px, pz, spr, cpr;
	sList *l;
	sObject *o;
	World *w, *p;
	sVector2 rp;

    spr = SIN(-heading);
    cpr = COS(-heading);
    col = COL(col);
	p = Player->data;
	for (l = targets->next; l != targets; l = l->next) {
		o = l->item;
		w = o->data;
		if (w) {
			dx = (w->p.x - p->p.x) >> SPEED_SHIFT;
        	dz = (w->p.z - p->p.z) >> SPEED_SHIFT;
			if ((ABS(dx) < Meter(20000)) && (ABS(dz) < Meter(20000))) {
        		px = (int)(dx >> 6);
        		pz = (int)(dz >> 6);
        		rp.x = (int)((MUL(px, cpr) - MUL(pz, spr)) >> SSHIFT);
        		rp.y = (int)((MUL(px, spr) + MUL(pz, cpr)) >> SSHIFT);
        		rp.x >>= (5 + radar_scale);
        		rp.y >>= (5 + radar_scale);
				rp.x += cx;
				rp.y = cy - rp.y;
        		pixel_prim(rp.x, rp.y, col);
			}
		}
    }
}


/* -------------------
 * GLOBAL draw_radar_display
 *
 * Draw radar into rectangle - called from gauges.c
 */
void draw_radar_display(int cx, int cy, int width, int height)
{
	int dx, dy, heading, col;
	World *p;
    Screen_clip s;

	s = clip_region;
	clipping(cx - width, cx + width, cy - height, cy + height);
	p = Player->data;
	col = COL(RADAR_MARK);
	dx = width;
	dy = height;
	heading = (short)p->r.y;

	if (!radar_fail) {
		hline_prim(cx - 2, cx + 2, cy, col);
		hline_prim(cx - 1, cx + 1, cy + 2, col);
		vline_prim(cy - 1, cy + 2, cx, col);
		draw_targets(cx, cy, heading, ground_targets, GREEN);
		draw_targets(cx, cy, heading, air_targets, WHITE);
	} else {
		draw_string(cx - 10, cy - 3, "FAIL", RADAR_MARK);
	}
	clipping(s.left, s.right, s.top, s.bottom);
}

/* --------------
 * Weapons code
 * -------------- */
sVector ccip = {0L, 0L,  0L};
static int weapon_track_modes[] = {0, 0, W_HEAT, W_GROUND, 0, 0};

void inter_cycle_weapon(sObject *self)
{
	clear_selection();

	/* Cycle to next avail weapon */
	weapon_type = (weapon_type + 1) % 6;
	weapon_track = weapon_track_modes[weapon_type];
}

void set_weapon_selected(int type)
{
	clear_selection();

	weapon_type =  type;
	weapon_track = weapon_track_modes[weapon_type];
}

/* select_target()
 * ---------------
 * Find target on other team nearest to o
 * set as currently selected target
 */
void select_target(sObject *targeter, int type)
{
	World *o, *tgt;
    int bearing, elev, min, o_team;
    sList *l, *targets;
    sObject *target;
    sVector r;
    sVector *tgt_p, d;
    Transdata trans;

	/* Select target list */
    if (type == W_HEAT)
        targets = air_targets;
    else if (type == W_GROUND)
        targets = ground_targets;
    else
        targets = NULL;

    if (targets != NULL) {
		o = targeter->data;
		o_team = o->flags & TEAM_FLAGS;
        setup_trans(&o->r, &trans);
        min = DEG(170);
        for (l = targets->next; l != targets; l = l->next) {
            target = (sObject *)l->item;
			tgt = target->data;
			if (tgt && ((tgt->flags & TEAM_FLAGS) != o_team)) {
				tgt_p = Posn(target);
            	d.x = (tgt_p->x - o->p.x) >> SPEED_SHIFT;
            	d.y = (tgt_p->y - o->p.y) >> SPEED_SHIFT;
            	d.z = (tgt_p->z - o->p.z) >> SPEED_SHIFT;
            	invrot3d(&d, &d, &trans);
            	bearing = fast_atan(-d.x, d.z);
            	bearing = ABS(bearing);
            	elev = fast_atan(d.y, d.z);
            	elev = ABS(elev);
            	if ((bearing < DEG(30)) && (elev < DEG(30)) && ((bearing + elev) < min)) {
               		min = bearing + elev;
               		selected = target;
       				selected_time = simulation_time;
               		selected_range = (int)(magnitude(&d) >> 13);
               		weapon_lock = TRUE;
   					if (type == W_HEAT)
						swlock_sound();
            	}
			}
        }
		debug("SELECT TGT %d %x\n", list_length(l), selected);
    }
}

sObject *get_nearest_target(sObject *targeter, int air_flag, int32 max_range, int include_player)
{
	World *o, *tgt;
    int o_team;
	int32 range, min_range;
    sList *l, *targets;
    sObject *target, *nearest;
    sVector d;

    if (air_flag)
        targets = air_targets;
    else
        targets = ground_targets;

	o = targeter->data;
	o_team = o->flags & TEAM_FLAGS;
	min_range = max_range;
	nearest = NULL;
	if (include_player && Player) {
		tgt = Player->data;
		if ((tgt->flags & TEAM_FLAGS) &&
				((tgt->flags & TEAM_FLAGS) != o_team)) {
			range = get_range(&o->p, &tgt->p, &d);
			if (range < min_range) {
				min_range = range;
               	nearest = Player;
            }
		}
	}
	for (l = targets->next; l != targets; l = l->next) {
        target = (sObject *)l->item;
		if (target != targeter) {
			tgt = target->data;
			if (tgt && (tgt->flags & TEAM_FLAGS) &&
					((tgt->flags & TEAM_FLAGS) != o_team)) {
				range = get_range(&o->p, &tgt->p, &d);
				if (range < min_range) {
					min_range = range;
               		nearest = target;
            	}
			}
		}
    }
	return nearest;
}


/* -------------------
 * GLOBAL air_target
 *
 * Called by possible air target objects from their update methods
 */
void air_target(sObject *o, int32 range)
{
	World *w;

	w = o->data;
	if (range < M(8000)) {
        if ((w->flags & TARGET_TAG) == 0) {
            list_add(o, air_targets);
            w->flags |= TARGET_TAG;
        }
    } else {
        if ((w->flags & TARGET_TAG) != 0)
            list_remove(o, air_targets);
            w->flags &= ~TARGET_TAG;
    }
}


/* -------------------
 * GLOBAL ground_target
 *
 * Called by possible ground target objects from their update methods
 */
void ground_target(sObject *o, int32 range)
{
	World *w;

	w = o->data;
    if (range < M(8000)) {
		if ((w->flags & TARGET_TAG) == 0) {
            list_add(o, ground_targets);
            w->flags |= TARGET_TAG;
        }
    } else {
        if ((w->flags & TARGET_TAG) != 0)
            list_remove(o, ground_targets);
            w->flags &= ~TARGET_TAG;
    }
}


/* -------------------
 * GLOBAL check_targets
 *
 * Called from main loop to deselect old targets
 */
void check_targets(void)
{
	/* Deselect target after 1 min */
	if ((simulation_time - selected_time) > SECS(60L)) {
		debug("CLEAR TGT\n");
		if (weapon_track != W_NOTRACK)
        	selected = NULL;
        weapon_lock = FALSE;
        selected_range = 0;
    }

	/* Automatic tgt selection */
	if (simulation_time >= target_check_time) {
		target_check_time = simulation_time + SECS(1L);
		if ((weapon_track != W_NOTRACK) && !weapon_lock) {
			debug("RESELECT TGT\n");
			select_target(Player, weapon_track);
		}
    }

}

void clear_selection(void)
{
    selected 		= NULL;
    weapon_lock 	= FALSE;
    selected_range 	= 0;
}


/* -------------------
 * GLOBAL remove_target
 *
 * Remove target object from all lists
 */
void remove_target(sObject *o)
{
	World *w;

    if (o == selected) {
        selected 	= NULL;
        weapon_lock = FALSE;
        selected_range = 0;
    }
    list_remove(o, air_targets);
    list_remove(o, ground_targets);
    list_remove(o, radar_targets);
	w = o->data;
    w->flags &= ~TARGET_TAG;
}


/* -------------------
 * GLOBAL over_shape
 *
 * Check if position p is over shape at sp/sr
 */
int over_shape(sVector *p, sShape *s, sVector *sp, sVector *sr)
{
	int rot, sa, ca, dx, dz, rdx, rdz;

	dx = (p->x - sp->x) >> s->scale;
	dz = (p->z - sp->z) >> s->scale;
	if ((dx < s->radius) && (dz < s->radius)) {
		rot = sr->y;
    	if (rot) {
        	sa = SIN(-rot); ca = COS(-rot);
        	rdx = (int)((MUL(dx,ca) - MUL(dz,sa)) >> SSHIFT);
        	rdz = (int)((MUL(dx,sa) + MUL(dz,ca)) >> SSHIFT);
        	if ((rdx > s->min_size.x) && (rdx < s->max_size.x) &&
					(rdz > s->min_size.z) && (rdz < s->max_size.z))
            	return TRUE;
        	else
            	return FALSE;
    	} else {
        	if ((dx > s->min_size.x) && (dx < s->max_size.x) &&
					(dz > s->min_size.z) && (dz < s->max_size.z))
            	return TRUE;
        	else
            	return FALSE;
    	}
	}
	return FALSE;
}

/* Kills off any objects supported by self */
void remove_supported(sObject *self)
{
    World *o, *w;
	sList *objects, *l;
	sObject *test;
	sShape_def *sd;
	sShape *s;

	o = self->data;
	sd = o->shape;
	while (sd->down)
		sd = sd->down;
	s = sd->s;
	objects = get_tp_objects(self);
	if (objects) {
		for (l = objects->next; l != objects; l = l->next) {
			test = l->item;
			if ((test != self) && (test != Player)) {
				w = test->data;
				if (!(w->flags & DETACHING) &&
					over_shape(&w->p, s, &o->p, &o->r)) {
					if (w->p.y < (o->p.y + (s->max_size.y << (s->scale + 1)))) {
						SafeMethod(test, World, kill)(test);
					}
				}
			}
		}
	}
}

void reset_inter(void)
{
    selected 		= NULL;
    weapon_lock 	= FALSE;
    selected_range 	= 0;
    list_destroy(ground_targets);
    list_destroy(air_targets);
    list_destroy(radar_targets);
}

void setup_inter(void)
{
    air_targets 	= new_list();
    ground_targets 	= new_list();
    radar_targets 	= new_list();
}

