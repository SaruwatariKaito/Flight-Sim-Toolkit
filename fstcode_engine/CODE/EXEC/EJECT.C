/* --------------------------------------------------------------------------
 * EXEC 		Copyright (c) Simis Ltd 1993,994,1995
 * ---			All Rights Reserved
 *
 * File:		eject.c
 * Ver:			1.00
 * Desc:		Ejected pilot class
 *
 * -------------------------------------------------------------------------- */
#include <stdlib.h>
#include "eject.h"
#include "defs.h"
#include "ground.h"

extern void set_view_direction(int abs, int direction);
static sSymbol *become_viewer;

/* Declare Class Pointer */
EseatClass *Eseat_class = NULL;

#define EJECTACCEL 2

/* --------------------
 * METHOD update
 * 
 * Ejected object falls under gravity until Player aircraft
 * dies (or ejected object hits ground)
 * 
 */ 
static void update(sObject *self)
{
    Eseat *o;

    o = self->data;
    if (o->p.y < (get_terrain_height(o->p.x, o->p.z) + M(3))) {
		if (player_die) {
			end_fst();
		} else {
			if (self == viewer)
				control_set_viewer(Player);
			ejector_symbol->value = NULL;
			remove_object(self);
		}
     } else {
		if (o->wv.y > FIXSPD(-30))
        	o->wv.y -= EJECTACCEL * frame_time;
        o->op = o->p;
        o->p.x += MUL(o->wv.x, frame_time);
        o->p.y += MUL(o->wv.y, frame_time);
        o->p.z += MUL(o->wv.z, frame_time);
		o->r.x += MUL(o->dr.x, frame_time);
		o->r.y += MUL(o->dr.y, frame_time);
		o->r.z += MUL(o->dr.z, frame_time);
    }
}

void register_eject(void)
{
    Eseat_class = new_class(sizeof(EseatClass), "Eseat", sizeof(Eseat), Mobile_class);
	Eseat_class->update = update;
	Eseat_class->check_collide = NULL;
    SetupMessage(become_viewer);
}


/* -------------------
 * GLOBAL start_eject
 * 
 * Create eject object and make it viewer
 * 
 */
sObject *start_eject(sVector *p)
{
	sObject *o;
	Eseat  *e;

	o = new_object(Eseat_class);
	if (o) {
		e = o->data;
		e->shape = get_shape("EJECT.FSD");
		e->p = *p;
		e->wv.y = FIXSPD(100);
		e->dr.y = 4;
		e->strength = 0;
		ejector_symbol->value = o;
		attach_object(o);
    	change_cockpit(NULL);
  		set_viewer(o);
	}
	return o;
}
