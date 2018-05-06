/* --------------------------------------------------------------------------
 * EXEC 		Copyright (c) Simis Ltd 1993,994,1995
 * ---			All Rights Reserved
 *
 * File:		cultural.c
 * Ver:			2.00
 * Desc:		Cultual class
 * 
 * Cultural class is simplest class implmented in EXEC - cultural objects
 * are loaded from world.fst (defined using world editor).
 *
 * -------------------------------------------------------------------------- */
#include <stdlib.h>
#include "cultural.h"
#include "defs.h"
#include "sound.h"
#include "explo.h"

#include "ground.h"

/* Declare Class Pointer */
CulturalClass *Cultural_class = NULL;


/* --------------------
 * METHOD cultural_update
 * 
 * If object is on a team it considers itself
 * as a possible ground target (inter.c)
 */ 
static void cultural_update(sObject *self)
{
    Cultural *o;
	sVector    d;
	int32 range;

    o = self->data;
	o->timer = SECS(4);

	if (o->shape == o->dead)
		return;

	if (o->flags & TEAM_FLAGS) {
		range = get_range(Posn(self), Posn(Player), &d);
		ground_target(self, range);
	}
}


/* -------------------
 * GLOBAL become_viewer
 * 
 * Make this object the viewer
 */
void become_viewer(sObject *o)
{
  set_viewer(o);
}

/* Method used to read in an inital world state */
static void file_read(sObject *self, FILE *fp)
{
	Basic_data data;
    Cultural *o;

    o = self->data;

	file_read_basic(&data, fp);

	/* class data - none */

	o->p.x = M(data.x); o->p.y = M(data.y); o->p.z = M(data.z);
	o->flags = DRAW_FIRST | data.f;
	o->r.y = DEG(data.ry);
    o->alive = o->shape = get_shape(data.shape);
	set_object_radius(self);
	o->dead  = get_shape(data.dead);
	o->init_strength = o->strength = data.strength;
}

static int cultural_hit(sObject *self, sObject *collide, int kp)
{
    Cultural *o;

    o = self->data;
    if (o->strength > 0) {
		o->strength -= kp;
		if (o->strength <= 0) {
			target_scoring(self, collide);
			SafeMethod(self, World, kill)(self);
			return TRUE;
		}
	}
	return FALSE;
}

static void cultural_kill(sObject *self)
{
    Cultural *o;

    o = self->data;
	if (o->shape == o->dead)
		return;

	if (!o->dead) {
		remove_supported(self);
	}
	create_explo(o->p, zero, o->shape, FIXSPD(8));
	explo2_sound();
	remove_target(self);
	if (o->dead) {
		o->shape = o->dead;
	} else {
    	remove_object(self);
	}
}

static void resupply(sObject *self)
{
    Cultural *o;

    o = self->data;

	if ((o->shape == o->dead) && o->alive) {
		o->shape    = o->alive;
	}
	if (o->strength < o->init_strength) {
		o->strength += 5;
	}
}

void register_cultural(void)
{
    Cultural_class = new_class(sizeof(CulturalClass), "Cultural", sizeof(Cultural), World_class);

	Cultural_class->file_read = file_read;
    Cultural_class->update = cultural_update;
    Cultural_class->hit  = cultural_hit;
    Cultural_class->kill = cultural_kill;

	add_message(Cultural_class, "become_viewer", 0, become_viewer);
	add_message(Cultural_class, "resupply", 0, resupply);
}

sObject *new_cultural(long int x, long int y, long int z, int dry, sShape_def *sd)
{
    sObject *o;
    Cultural *c;

    o = new_object(Cultural_class);
    if (o) {
        c = o->data;
        c->p.x = x;
        c->p.y = y;
        c->p.z = z;
        c->dr.y = dry;
        c->shape = sd;
    }
    return o;
}
