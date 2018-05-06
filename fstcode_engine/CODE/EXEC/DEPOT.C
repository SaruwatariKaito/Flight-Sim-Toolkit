/* --------------------------------------------------------------------------
 * EXEC 		Copyright (c) Simis Ltd 1993,994,1995
 * ---			All Rights Reserved
 *
 * File:		depot.c
 * Ver:			2.00
 * Desc:		Depot class implementation
 *
 * Depots generate trucks when Player is within 50K of depot and
 * there are trucks in depot.
 * 
 * -------------------------------------------------------------------------- */
#include <stdlib.h>
#include "depot.h"
#include "defs.h"
#include "truck.h"
#include "sound.h"
#include "explo.h"

/* Declare Class Pointer */
DepotClass *Depot_class = NULL;


/* --------------------
 * METHOD update_depot
 * 
 * 
 */ 
static void update_depot(sObject *self)
{
	Depot *d;
	sList  *l;
	sSymbol *s;
	sVector v;
	int32 range;
	Path   *p;

	d = self->data;

	if (d->shape == d->dead)
		return;

	range = get_range(Posn(self), Posn(Player), &v);
	/* Create trucks */
	if ((d->in_depot > 0) && (range < M(50000))) {
		Map (l, d->paths) {
			s = l->item;
			p = s->value;
			create_truck(d->vehicle_shape, FIXSPD(d->speed), p, d->flags, d->truck_strength, d->shells, d->fire_delay, d->predict, d->aagun);
			d->in_depot -= 1;
		}

		if (d->count-- <= 0) {
			d->timer = SECS(d->interval);
			d->count = d->size-1;
		} else {
  			d->timer = SECS(d->sub_int);
		}
	}

	if (d->flags & TEAM_FLAGS) {
		ground_target(self, range);
	}
}


/* --------------------
 * METHOD reset
 * 
 * Object reset - creates path list structure
 */ 
static void reset(sObject *self)
{
  Depot *d;

  d = self->data;
  d->paths = new_list();
}


/* --------------------
 * METHOD file_read
 * 
 * Reads basic depot data and list of path ids
 */ 
static void file_read(sObject *self, FILE *fp)
{
	Basic_data data;
	Depot *o;
	int n, flags;
	char vehicle[32];
	char model[32];
	char path_name[32];
	sSymbol *path;

	o = self->data;

	file_read_basic(&data, fp);

	o->p.x = M(data.x); o->p.y = M(data.y); o->p.z = M(data.z);
	o->flags = data.f;
	o->r.y = DEG(data.ry);
    o->alive = o->shape = get_shape(data.shape);
	set_object_radius(self);

	o->dead  = get_shape(data.dead);
	o->init_strength = o->strength = data.strength;

	/* Extra */
    fscanf(fp, "%s\n", vehicle);
	o->vehicle_shape = get_shape(vehicle);
    fscanf(fp, "%d %d %d %d %d", &o->in_depot, &o->speed, &o->interval, &o->size, &o->sub_int);

	if (world_file_version >= 4) {
		fscanf(fp, "%d %d %d %d", &flags, &o->truck_strength, &o->shells, &o->fire_delay);
		o->predict = (flags & 0x02) ? TRUE : FALSE;
		o->aagun = (flags & 0x04) ? TRUE : FALSE;
		o->fire_delay *= 100;
		if (o->fire_delay == 0)
			o->fire_delay = 50;
	} else {
		o->predict = FALSE;
		o->aagun = FALSE;
		o->truck_strength = 5;
		o->shells = 0;
		o->fire_delay = 500;
	}

	/* Paths */
    fscanf(fp, "%d", &n);
	while (n--) {
		fscanf(fp, "%s", path_name);
		path = get_symbol(path_symbol_table, path_name);
		if (path && path != nil_symbol)
			list_add_last(path, o->paths);
	}
}

static void kill(sObject *self)
{
    Depot *o;

    o = self->data;

    create_explo(o->p, zero, o->shape, FIXSPD(8));
	explo2_sound();
	remove_target(self);
	if (o->dead) {
		o->shape = o->dead;
		o->in_depot = 0;
	} else {
    	remove_object(self);
	}
}


/* --------------------
 * METHOD resupply
 * 
 * If depot has been killed - rebuild
 * else add one to number of trucks in depot
 */ 
static void resupply(sObject *self)
{
    Depot *o;

    o = self->data;

	if ((o->shape == o->dead) && o->alive) {
		o->shape    = o->alive;
	}
	if (o->strength < o->init_strength) {
		o->strength += 5;
	} else {
		o->in_depot += 1;
	}
}

void register_depot(void)
{
    Depot_class = new_class(sizeof(DepotClass), "Depot", sizeof(Depot), Cultural_class);
	Depot_class->reset 		= reset;
	Depot_class->file_read  = file_read;
    Depot_class->kill 		= kill;

	Depot_class->update = update_depot;

	add_message(Depot_class, "resupply", 0, resupply);
}

