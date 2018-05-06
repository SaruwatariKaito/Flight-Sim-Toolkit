/* --------------------------------------------------------------------------
 * WORLD		Copyright (c) Simis Ltd 1993,994,1995
 * ---			All Rights Reserved
 *
 * File:		smoke.c
 * Ver:			1.00
 * Desc:		Implements smoke object
 *
 * -------------------------------------------------------------------------- */
#include <stdlib.h>
#include "world.h"
#include "smoke.h"

/* Declare Class Pointer */
SmokeClass *Smoke_class = NULL;

int c_smoke1 = 14, c_smoke2 = 13, c_smoke3 = 12;

static void update_smoke(sObject *self)
{
    Smoke *e;

    e = self->data;
	if ((e->life_time += frame_time) >= e->life) {
		remove_object(self);
	}
}

int draw_smoke(sObject *self)
{
    Smoke *e;
    int dist, size;
    sVector d;
    sVector2 s;

    e = self->data;
	d.x = (int)((e->p.x - viewer_p->x) >> SPEED_SHIFT);
    d.y = (int)((e->p.y - viewer_p->y) >> SPEED_SHIFT);
    d.z = (int)((e->p.z - viewer_p->z) >> SPEED_SHIFT);
	dist = magnitude(&d);
    if (dist < Meter(4000)) {
		if (dist > Meter(2500)) {
    		perspect(&d, &s);
    		pixel_prim(s.x, s.y, COL(c_smoke1));
		} else {
			size = Meter(1) + (e->life_time >> 2);
			if (size > Meter(4))
				size = Meter(4);
			sphere3(&d, size, c_smoke2);
		}
	}
	return TRUE;
}

void create_smoke(sVector *p, int life)
{
	sObject *o;
	Smoke  *e;

	o = new_object(Smoke_class);
	if (o) {
		e = o->data;
		e->op = e->p = *p;
		e->timer = 0;
		e->life_time = 0;
		e->life = life;
		attach_object(o);
	}
}

void register_smoke(void)
{
    Smoke_class = new_class(sizeof(SmokeClass), "Smoke", sizeof(Smoke), Mobile_class);

    /* Class methods */
    Smoke_class->update	= update_smoke;
    Smoke_class->draw	= draw_smoke;
}
