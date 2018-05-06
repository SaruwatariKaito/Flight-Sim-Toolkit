/* --------------------------------------------------------------------------
 * EXEC 		Copyright (c) Simis Ltd 1993,994,1995
 * ---			All Rights Reserved
 *
 * File:		military.c
 * Ver:			2.00
 * Desc:		Military aircraft class
 *
 * Inherits from plane.c
 * Adds pylon based weapons system.
 *
 * -------------------------------------------------------------------------- */

#include <stdlib.h>
#include "military.h"
#include "defs.h"
#include "sound.h"

#include "bullets.h"
#include "weapons.h"
#include "flare.h"

static int last_padlock_view_on = FALSE;
static int old_look_direction = 0;
static int old_look_elevation = 0;

extern void read_model_params(char *file, Plane *o);

/* Declare Class Pointer */
MilitaryClass *Military_class = NULL;

/* Bodge for gun recoil */
static int weapon_firing = FALSE;

extern void update_plane(sObject *self);


/* --------------------
 * METHOD update_military
 *
 * Implements weapons system carrying logic
 * Calls superclass update_plane for aero model
 *
 */
void update_military(sObject *self)
{
    Military *o;
	sObject *target;
	World *tgt;
	int bearing, elev;
	int32 range, offset;
	sVector d, tgt_p;
	sVector r;
	Transdata trans;

	o = self->data;
	if (o->fast_shape) {
		if (o->speed > FIXSPD(500)) {
			o->shape = o->fast_shape;
		} else {
			if (o->gear_down_shape) {
				if (o->gear_down)
					o->shape = o->gear_down_shape;
				else
					o->shape = o->default_shape;
			} else {
				o->shape = o->default_shape;
			}
		}
	}

	/* Force output */
	weapon_firing = FALSE;
	update_plane(self);

	if (target_track_on || padlock_view_on || (o->rear_gun && (o->flags & TEAM_FLAGS))) {
		if (total_time > o->target_check_time) {
			o->target_check_time = total_time + SECS(5);
			if (target_track_on || padlock_view_on)
				o->nearest_target = get_nearest_target(self, TRUE, M(2000), FALSE);
			else
				o->nearest_target = get_nearest_target(self, TRUE, M(700), FALSE);
		}
	}

	if (padlock_view_on) {
		if (!last_padlock_view_on) {
			old_look_direction = look_direction;
			old_look_elevation = look_elevation;
		}
		if (o->nearest_target) {
			tgt = o->nearest_target->data;
			range = get_range(&o->p, &tgt->p, &d);
			if (range > M(2000)) {
				o->nearest_target = NULL;
			} else {
            	d.x >>= SPEED_SHIFT;
            	d.y >>= SPEED_SHIFT;
            	d.z >>= SPEED_SHIFT;
            	range >>= SPEED_SHIFT;
				setup_trans(&o->r, &trans);
				invrot3d(&d, &tgt_p, &trans);
            	bearing = fast_atan(-tgt_p.x, tgt_p.z);
				elev = fast_atan(tgt_p.y, range);
				set_view_orientation(TRUE, bearing, elev);
			}
		}
		if (o->nearest_target == NULL) {
			set_view_orientation(TRUE, old_look_direction, old_look_elevation);
			padlock_view_on = FALSE;
		}
	} else if (last_padlock_view_on) {
		set_view_orientation(TRUE, old_look_direction, old_look_elevation);
	}
	last_padlock_view_on = padlock_view_on;

	if ((o->rear_gun) && (o->flags & TEAM_FLAGS)) {
		if (o->nearest_target) {
			tgt = o->nearest_target->data;
			range = get_range(&o->p, &tgt->p, &d);
			if (range > M(500)) {
				if (target_track_on || padlock_view_on) {
					if (range > M(2000))
						o->nearest_target = NULL;
				} else {
					o->nearest_target = NULL;
				}
			} else {
				if ((o->rear_gun_time < total_time) &&
					(tgt->flags & TEAM_FLAGS) &&
					((o->flags & TEAM_FLAGS) != (tgt->flags & TEAM_FLAGS))) {
            		d.x >>= SPEED_SHIFT;
            		d.y >>= SPEED_SHIFT;
            		d.z >>= SPEED_SHIFT;
            		range >>= SPEED_SHIFT;
					setup_trans(&o->r, &trans);
					invrot3d(&d, &tgt_p, &trans);
            		bearing = fast_atan(-tgt_p.x, tgt_p.z);
					elev = fast_atan(tgt_p.y, range);
            		if ((ABS(bearing) > DEG(135)) &&
			 			(elev < DEG(30)) && (elev > DEG(-10))) {
						o->rear_gun_time = total_time + 25L;
						r.x = ADDANGLE(fast_atan(d.y, range), range / 30);
						r.y = fast_atan(-d.x, d.z);
						r.z = 0;
						offset = M(5);
						fire_bullet(self, &r, offset, FIXSPD(1200), 200, FALSE);
            		}
				}
			}
		}
	}
}


static void military_detaching(sObject *self, sObject *old)
{
	Military *o;

    o = self->data;
	if (o->runway == old) {
		o->runway = NULL;
	}
	if (o->nearest_target == old) {
		o->nearest_target = NULL;
	}
}

/* Method used to read in an inital world state */
static void file_read(sObject *self, FILE *fp)
{
	Basic_data data;
    Military *o;
	char shape[32];
	char cockpit[32];
	char model[32];
	char gear_down_shape[32];
	char fast_shape[32];
	int  i, wn[MAX_WEAPON_TYPE], wkp[MAX_WEAPON_TYPE];
	sVector min, max;

    o = self->data;
	file_read_basic(&data, fp);

	/* read class data */
    fscanf(fp, "%s\n", cockpit);
    fscanf(fp, "%s\n", model);
    fscanf(fp, "%s\n", gear_down_shape);
    fscanf(fp, "%s\n", fast_shape);
	if (world_file_version >= 3) {
		fscanf(fp, "%d", &o->rear_gun);
	} else {
		o->rear_gun = FALSE;
	}

	if (world_file_version >= 2) {
		for (i = 0; i < MAX_WEAPON_TYPE; i++)
			fscanf(fp, "%d %d", &wn[i], &wkp[i]);
	} else {
		for (i = 0; i < 6; i++)
			fscanf(fp, "%d %d", &wn[i], &wkp[i]);
		for (i = 6; i < MAX_WEAPON_TYPE; i++) {
			wn[i] = wkp[i] = 0;
		}
	}

	o->p.x = M(data.x); o->p.y = M(data.y); o->p.z = M(data.z);
	o->flags = data.f;
	o->r.y = DEG(data.ry);
    o->default_shape = o->shape = get_shape(data.shape);
	o->speed = FIXSPD(0);
	o->cockpit = read_gauges(cockpit);
    o->gear_down_shape = get_shape(gear_down_shape);
    o->fast_shape = get_shape(fast_shape);
	if (o->gear_down_shape)
		o->shape = o->gear_down_shape;
	set_object_radius(self);
	find_size(o->shape, &min, &max);
	o->eye_height = -min.y;
	if (o->eye_height < MM(800))
		o->eye_height = MM(800);

	o->strength = data.strength;

	read_model_params(model, self->data);
	o->start_class = self->class;

	o->weapon_system = new_weapons_system(self);
	if (o->weapon_system) {
		for (i = 0; i < MAX_WEAPON_TYPE; i++) {
			SafeMethod(o->weapon_system, Weapons, rearm)(o->weapon_system, i, wn[i], wkp[i]);
			o->rearm_data[i] = wn[i];
		}
		weapon_n = wn[WT_GUN];
	}
	o->rear_gun_time = 0L;
	o->target_check_time = 0L;
	o->nearest_target = NULL;
}

static int fire1_control(sObject *self)
{
	Military *o;
	sObject   *ws;

	o = self->data;
	ws = o->weapon_system;
	if (!o->landed) {
		SafeMethod(ws, Weapons, fire)(ws);
		/* Force output */
		weapon_firing = TRUE;
	}
	return TRUE;
}

static int weapon_cycle(sObject *self)
{
	Military *o;
	sObject   *ws;

	o = self->data;
	ws = o->weapon_system;
	SafeMethod(ws, Weapons, cycle)(ws);

	return TRUE;
}

static int fire_flare(sObject *self)
{
	Military *o;

	o = self->data;
	if (!o->landed) {
		drop_flare(&o->op, &o->wv);
		flare_sound();
	}
	return TRUE;
}

static int fire_chaff(sObject *self)
{
	Military *o;

	o = self->data;
	if (!o->landed) {
		chaff_sound();
	}
	return TRUE;
}

static int jetison_stores(sObject *self)
{
	Military *o;

	o = self->data;
	if (!o->landed) {
		clunk_sound();
	}
	return TRUE;
}

static int show_tracked_aircraft(sObject *from, sObject *target)
{
    int sx, sy, col;
	World *tgt;
	World *o;
    int32 range;
    sVector p;
    sVector2 s;

	o = from->data;
	tgt = target->data;
	if (tgt == NULL || !(tgt->flags & TARGET_TAG))
		return FALSE;

    p.x = (tgt->p.x - o->p.x) >> SPEED_SHIFT;
    p.y = (tgt->p.y - o->p.y) >> SPEED_SHIFT;
    p.z = (tgt->p.z - o->p.z) >> SPEED_SHIFT;
    range = perspect(&p, &s);
    sx = s.x - xcentre;
    sy = s.y - ycentre;
    if (range > 0) {
		if ((ABS(sx) > 50) || (ABS(sy) > 50)) {
			line(xcentre, ycentre, s.x, s.y, GREEN);
		}
		return TRUE;
	}
	return FALSE;
}

extern void draw_floating_hud(sSymbol *cs);
static int draw_cockpit(sObject *self)
{
	Military *o;

	o = self->data;
	if (look_direction == 0 && look_elevation == 0) {
		draw_gauges(o->cockpit);
		if (o->nearest_target && target_track_on)
			show_tracked_aircraft(self, o->nearest_target);
	} else {
    	draw_floating_hud(o->cockpit);
	}
	return TRUE;
}

static int military_rearm(sObject *self)
{
	Military *m;
	sObject *o;
	int i;

	m = self->data;
	o = m->weapon_system;
	if (o) {
		for (i = 0; i < MAX_WEAPON_TYPE; i++) {
			SafeMethod(m->weapon_system, Weapons, rearm)(m->weapon_system, i, m->rearm_data[i], 0);
		}
	}
	return TRUE;
}

void register_military(void)
{
    Military_class = new_class(sizeof(MilitaryClass), "Military", sizeof(Military), Plane_class);

	Military_class->update = update_military;
	Military_class->file_read = file_read;
	Military_class->draw_2d   = draw_cockpit;
	Military_class->detaching = military_detaching;
	add_message(Military_class, "primary_fire",   0, fire1_control);
	add_message(Military_class, "weapon_cycle",   0, weapon_cycle);
	add_message(Military_class, "flare",          0, fire_flare);
	add_message(Military_class, "chaff",          0, fire_chaff);
	add_message(Military_class, "jetison_stores", 0, jetison_stores);
	add_message(Military_class, "rearm", 0, military_rearm);
}

