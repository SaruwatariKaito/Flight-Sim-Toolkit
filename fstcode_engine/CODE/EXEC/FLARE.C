/* --------------------------------------------------------------------------
 * EXEC 		Copyright (c) Simis Ltd 1993,994,1995
 * ---			All Rights Reserved
 *
 * File:		flare.c
 * Ver:			1.00
 * Desc:		Implements Flare class
 * 
 * Flares fall under gravity with simple drag model
 * Flare class contains list of active flares
 *
 * -------------------------------------------------------------------------- */
#include <stdlib.h>
#include "flare.h"
#include "defs.h"

/* Declare Class Pointer */
FlareClass *Flare_class = NULL;

#define FLARE_DRAG_COEF 1000
static void update_flare(sObject *self)
{
    Flare *o;

    o = self->data;

	o->life -= frame_time;
	if (o->life < 0) {
		list_remove(self, Flare_class->active_flares);
		remove_object(self);
	}

    o->wv.y -= GRAVITY * frame_time;
	o->wv.x = o->wv.x - ((o->wv.x * frame_time) / FLARE_DRAG_COEF);
	o->wv.z = o->wv.z - ((o->wv.z * frame_time) / FLARE_DRAG_COEF);
    o->op = o->p;
    o->p.x += MUL(o->wv.x, frame_time);
    o->p.y += MUL(o->wv.y, frame_time);
    o->p.z += MUL(o->wv.z, frame_time);
}

int draw_flare(sObject *self)
{
    Flare *e;
    register int dist;
    sVector v;
    sVector2 s;

    e = self->data;

    v.x = (int)((e->p.x - viewer_p->x) >> SPEED_SHIFT);
    v.y = (int)((e->p.y - viewer_p->y) >> SPEED_SHIFT);
    v.z = (int)((e->p.z - viewer_p->z) >> SPEED_SHIFT);
    dist = fast_perspect(&v, &s);
    pixel_prim(s.x, s.y, COL(RED));

    return TRUE;
}

void register_flare(void)
{
    Flare_class = new_class(sizeof(FlareClass), "Flare", sizeof(Flare), Mobile_class);

	Flare_class->update = update_flare;
	Flare_class->draw   = draw_flare;
	Flare_class->active_flares = new_list();
}

sObject *drop_flare(sVector *posn, sVector *wv)
{
	sObject *flare;
	Flare  *o;

	flare = new_object(Flare_class);
	if (flare) {
		o = flare->data;
		o->shape = NULL;
		o->p  = *posn;
		o->wv = *wv;
		o->life = SECS(5L);
		list_add(flare, Flare_class->active_flares);
		attach_object(flare);
	}
	return flare;
}
