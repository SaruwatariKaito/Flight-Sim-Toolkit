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
#include "anim.h"

/* Declare Class Pointer */
AnimClass *Anim_class = NULL;

/* Method used to read in an inital world state */
static void file_read(sObject *self, FILE *fp)
{
    Anim *o;
	long int x,y,z,f,r;
	int  i, t, n;
	char shape[32];
	char cockpit[32];

    o = self->data;
    fscanf(fp, "%ld %ld %ld %d %d ", &x, &y, &z, &r, &f);
    fscanf(fp, "%s\n", shape);
	o->p.x = M(x); o->p.y = M(y); o->p.z = M(z);
	o->flags = NULL;
	o->r.y = DEG(r);
    o->shape = get_shape(shape);
    fscanf(fp, "%d %d\n", &t, &n);
	if (n > MAX_ANIM)
	    stop("Error loading Animated class data - too many frames\n");
	o->dt = t;
	o->nframes = n;
	for (i = 0; i < n; i++) {
    	fscanf(fp, "%s", shape);
    	o->animation[i] = get_shape(shape);
	}
}

static void update(sObject *self)
{
  Anim *o;

  o = self->data;
  o->next_dt -= frame_time;
  if (o->next_dt <= 0) {
	o->frame = (o->frame+1)%o->nframes;
	o->shape = o->animation[o->frame];
	o->next_dt = o->dt;
  }
}

void register_anim(void)
{
    Anim_class = new_class(sizeof(AnimClass), "Anim", sizeof(Anim), Cultural_class);

	Anim_class->file_read = file_read;
	Anim_class->update    = update;
}

