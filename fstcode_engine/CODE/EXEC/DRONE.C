/* --------------------------------------------------------------------------
 * EXEC 		Copyright (c) Simis Ltd 1993,994,1995
 * ---			All Rights Reserved
 *
 * File:		.c
 * Ver:			1.00
 * Desc:		
 *
 * -------------------------------------------------------------------------- */
#include <stdlib.h>
#include <string.h>
#include "drone.h"
#include "defs.h"

/* Declare Class Pointer */
DroneClass *Drone_class = NULL;

static void update_drone(sObject *self)
{
    Mobile *e;
    sObject *obj;
    sVector v, dr;
    Transdata trans;

    e = self->data;
    e->r.y = SUBANGLE(e->r.y, ((((int32)SIN(e->r.z) << 10) >> SSHIFT) * frame_time) >> 6);
    setup_trans(&e->r, &trans);

    v.x = 0; v.y = 0; v.z = e->speed;
    rot3d(&v, &e->wv, &trans);
    e->p.x += MUL(e->wv.x, frame_time);
    e->p.y += MUL(e->wv.y, frame_time);
    e->p.z += MUL(e->wv.z, frame_time);
}

/* Method used to read in an inital world state */
static void file_read(sObject *self, FILE *fp)
{
    Drone *o;
	long int x,y,z,f,r;
	char shape[32];
	char cockpit[32];

    o = self->data;
    fscanf(fp, "%ld %ld %ld %d %d ", &x, &y, &z, &r, &f);
    fscanf(fp, "%s\n", shape);
    fscanf(fp, "%s\n", cockpit);
	o->p.x = M(x); o->p.y = M(y); o->p.z = M(z);
	o->flags = f;
	o->r.y = DEG(r);
    o->shape = get_shape(shape);
	o->r.z = DEG(20);
	o->speed = FIXSPD(200);
	if (strcmp(cockpit, "nil") != 0)
		o->cpt_sym = read_gauges(cockpit);
}

void drone_viewer(sObject *o)
{
  set_viewer(o);
}

static int draw_cockpit(sObject *o)
{
	Drone *d;

	d = o->data;
	draw_gauges(d->cpt_sym);
	return TRUE;
}

void register_drone(void)
{
    Drone_class = new_class(sizeof(DroneClass), "Drone", sizeof(Drone), Mobile_class);
	Drone_class->update = update_drone;
	Drone_class->file_read = file_read;
	Drone_class->draw_2d   = draw_cockpit;

	add_message(Drone_class, "become_viewer", 0, drone_viewer);
}

sObject *new_drone(int32 x, int32 y, int32 z, sShape_def *sd)
{
    sObject *o;
    Mobile *m;

    o = new_object(Drone_class);
    m = o->data;
    m->p.x = x;
    m->p.y = y;
    m->p.z = z;
	m->r.z = DEG(20);
	m->speed = FIXSPD(200);
    m->shape = sd;

	return o;
}
