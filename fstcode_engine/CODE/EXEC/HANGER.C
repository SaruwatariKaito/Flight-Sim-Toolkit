/* --------------------------------------------------------------------------
 * EXEC 		Copyright (c) Simis Ltd 1993,994,1995
 * ---			All Rights Reserved
 *
 * File:		hanger.c
 * Ver:			2.00
 * Desc:		Hanger class.
 *
 * Hanger objects setup in world.fst (Windows world.exe tool).
 *
 * Hanger objects create Aircraft objects when Player is within 40Km
 * of hanger.
 *
 * Aircraft initially follow paths linked to Hanger.
 *
 * -------------------------------------------------------------------------- */
#include <stdlib.h>
#include <math.h>
#include "hanger.h"
#include "aircraft.h"
#include "defs.h"
#include "sound.h"
#include "explo.h"

/* Declare Class Pointer */
HangerClass *Hanger_class = NULL;

static int random(int max)
{
	return((rand() * max) / RAND_MAX);
}


/* --------------------
 * METHOD update_hanger
 *
 * Check distance from hanger to Player - if < 40Km and aircraft
 * in hanger then create new aircraft object (aircraft.c)
 *
 */
static void update_hanger(sObject *self)
{
	sObject *ac;
	Hanger *d;
	sList  *l;
	sSymbol *s;
	Path   *p;
	sVector v;
	int32 range;

	d = self->data;

	if (d->shape == d->dead)
		return;

	range = get_range(Posn(self), Posn(Player), &v);
	/* Create ac */
	if ((d->in_hanger > 0) && (range < M(40000))) {
		Map (l, d->paths) {
			s = l->item;
			p = s->value;
			ac = create_ac(d->ac_shape, d->plane_jet, d->plane_min_speed,
				d->plane_max_speed, p, d->behaviour, d->rear_gun,
				d->aircraft_strength, d->aircraft_bombs, d->flags & TEAM_FLAGS);
			d->in_hanger -= 1;
			if (ac && (self == Player)) {
				set_viewer(ac);
			}
		}
	}
	if (self == Player)
		d->timer = SECS(2);
	else
		d->timer = SECS(100) + random(SECS(50));
	if (d->flags & TEAM_FLAGS) {
		ground_target(self, range);
	}
}

static void reset(sObject *self)
{
  Hanger *d;

  d = self->data;
  d->paths = new_list();
}

#define NUM_POINTS 19
static sVector2 points[NUM_POINTS];


/* --------------------
 * LOCAL read_model
 *
 * Read model file and calculate the instance data values required
 * for a hanger to generate aircraft
 */
static void read_model(sObject *hanger, char *filename)
{
    FILE *fp;
	int version, i;
	int thrust, weight, fuel_weight;
	int biplane, jet, dummy;
	float drag_factor, stall_angle, wing_eff;
	float f_dummy, c_power;
	int units, grid, centre;
	sVector2 v;
	char s[60];
	Hanger *o;

	o = hanger->data;
	fp = fopen(filename, "r");
	if (fp != NULL) {
		fscanf(fp, "%s %d\n", s, &version);
		if (version != 1) {
			stop("Model file version is not 1");
		}
		fscanf(fp, "%s %d\n", s, &weight);
		fscanf(fp, "%s %f\n", s, &drag_factor);
		fscanf(fp, "%s %f\n", s, &f_dummy);
		fscanf(fp, "%s %f\n", s, &f_dummy);
		fscanf(fp, "%s %f\n", s, &f_dummy);
		fscanf(fp, "%s %f\n", s, &f_dummy);
		fscanf(fp, "%s %f\n", s, &stall_angle);
		fscanf(fp, "%s %f\n", s, &wing_eff);
		fscanf(fp, "%s %f\n", s, &c_power);
		fscanf(fp, "%s %f\n", s, &f_dummy);
		fscanf(fp, "%s %f\n", s, &f_dummy);

		fscanf(fp, "%s %d\n", s, &dummy);
		fscanf(fp, "%s %d\n", s, &dummy);
		fscanf(fp, "%s %d\n", s, &dummy);
		fscanf(fp, "%s %d\n", s, &biplane);
		fscanf(fp, "%s %d\n", s, &dummy);
		fscanf(fp, "%s %d\n", s, &dummy);

		fscanf(fp, "%s %d\n", s, &thrust);
		fscanf(fp, "%s %d\n", s, &fuel_weight);
		fscanf(fp, "%s %d\n", s, &dummy);
		fscanf(fp, "%s %d\n", s, &jet);

		fscanf(fp, "%s %d\n", s, &units);
		fscanf(fp, "%s %d\n", s, &grid);
		fscanf(fp, "%s %d\n", s, &centre);

		for (i = 0; i < NUM_POINTS; i++) {
			fscanf(fp, "%d %d\n", &v.x, &v.y);
			points[i].x = (v.x * 10) / units;
			points[i].y = (v.y * 10) / units;
		}
		fclose(fp);

		o->plane_turn_rate = 10;
		o->plane_max_speed = calc_top_speed(points, thrust, drag_factor, weight);
		o->plane_max_speed -= o->plane_max_speed / 6;
		o->plane_min_speed = calc_stall_speed(points, wing_eff, stall_angle, weight);
		o->plane_min_speed += o->plane_min_speed / 2;
		o->plane_jet = jet;
	} else {
		o->plane_turn_rate = 10;
		o->plane_max_speed = FIXSPD(300);
		o->plane_min_speed = FIXSPD(100);
		o->plane_jet = TRUE;
	}
}

static void hanger_file_read(sObject *self, FILE *fp)
{
	Basic_data data;
	Hanger *o;
    char ac_shape[16];
    char ac_model[16];
    char ac_cockpit[16];
	int n, flags;
	char path_name[32];
	sSymbol *path;

	o = self->data;

	file_read_basic(&data, fp);

	o->p.x = M(data.x); o->p.y = M(data.y); o->p.z = M(data.z);
	o->flags = data.f;
	o->r.y = DEG(data.ry);
    o->alive = o->shape = get_shape(data.shape);
	set_object_radius(self);

	/* Extra */
    fscanf(fp, "%s\n", ac_shape);
    fscanf(fp, "%s\n", ac_model);
    fscanf(fp, "%s\n", ac_cockpit);
	o->ac_shape   = get_shape(ac_shape);
	o->ac_cockpit = read_gauges(ac_cockpit);
	if (world_file_version >= 3) {
		fscanf(fp, "%d %d", &o->in_hanger, &o->behaviour);
		fscanf(fp, "%d %d %d", &flags, &o->aircraft_strength, &o->aircraft_bombs);
		o->rear_gun = (flags & 0x01) ? TRUE : FALSE;
	} else {
		fscanf(fp, "%d %d", &o->in_hanger, &o->behaviour);
		o->rear_gun = FALSE;
		o->aircraft_strength = 10;
		o->aircraft_bombs = 0;
	}

	o->dead  = get_shape(data.dead);
	o->init_strength = o->strength = data.strength;

	/* Paths */
    fscanf(fp, "%d", &n);
	while (n--) {
		fscanf(fp, "%s", path_name);
		path = get_symbol(path_symbol_table, path_name);
		if (path && path != nil_symbol)
			list_add_last(path, o->paths);
	}

	read_model(self, ac_model);

	o->timer = SECS(5) + random(SECS(10));
}

void hanger_kill(sObject *self)
{
    Hanger *o;

    o = self->data;

    create_explo(o->p, zero, o->shape, FIXSPD(8));
	explo2_sound();
	remove_target(self);
	if (o->dead) {
		o->shape = o->dead;
		o->in_hanger = 0;
	} else {
    	remove_object(self);
	}
}

void hanger_resupply(sObject *self)
{
    Hanger *o;

    o = self->data;

	if ((o->shape == o->dead) && o->alive) {
		o->shape = o->alive;
	}
	if (o->strength < o->init_strength) {
		o->strength += 5;
	} else {
		o->in_hanger += 1;
	}
}

void register_hanger(void)
{
    Hanger_class = new_class(sizeof(HangerClass), "Hanger", sizeof(Hanger), Cultural_class);

	Hanger_class->reset 	= reset;
	Hanger_class->file_read = hanger_file_read;
	Hanger_class->update 	= update_hanger;
    Hanger_class->kill 		= hanger_kill;

	add_message(Hanger_class, "resupply", 0, hanger_resupply);
}

