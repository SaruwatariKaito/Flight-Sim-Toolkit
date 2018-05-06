/* --------------------------------------------------------------------------
 * EXEC 		Copyright (c) Simis Ltd 1993,994,1995
 * ---			All Rights Reserved
 *
 * File:		sam.c
 * Ver:			1.00
 * Desc:		SAM class
 *
 * Surface to Air Missile firer. 
 * SAM Missile defined in missile.c
 * 
 * -------------------------------------------------------------------------- */
#include <stdlib.h>
#include "sam.h"
#include "defs.h"
#include "sound.h"
#include "explo.h"

extern void add_sam(sObject *site, sObject *tgt, int kp);

/* Declare Class Pointer */
SAMClass *SAM_class = NULL;

static void update_sam(sObject *self)
{
	SAM *o;
	sObject *target;
	World *tgt;
    int32 range, detect_range;
    sVector d;

	o = self->data;
	if (o->shape == o->dead) {
        o->timer = SECS(20);
		return;
	}

	if (Player) {
        range = get_range(&o->p, Posn(Player), &d);
        /* radar_target(o, range); */
        o->timer = (range > M(10000)) ? SECS(10L) : SECS(4L);

		target = get_nearest_target(self, TRUE, range, FALSE);
		if (target == NULL) {
			target = Player;
		}
		tgt = target->data;
        if (tgt->p.y < o->floor) {
            detect_range = 0L;
        } else {
            detect_range = o->detect;
        }
        if ((range < detect_range) &&
				(o->flags & TEAM_FLAGS) && (tgt->flags & TEAM_FLAGS) &&
				((o->flags & TEAM_FLAGS) != (tgt->flags & TEAM_FLAGS))) {
            add_sam(self, target, o->kp);
            o->timer = SECS(25L);
        }
		ground_target(self, range);
    }
}


/* Method used to read in an inital world state */
static void file_read(sObject *self, FILE *fp)
{
	Basic_data data;
    SAM *o;
	int detect, floor, dummy;

    o = self->data;

	file_read_basic(&data, fp);

	o->p.x = M(data.x); o->p.y = M(data.y); o->p.z = M(data.z);
	o->flags = data.f;
	o->r.y = DEG(data.ry);
    o->alive = o->shape = get_shape(data.shape);
	o->dead  = get_shape(data.dead);
	o->init_strength = o->strength = data.strength;
	set_object_radius(self);

	/* setup rest of iv's */
	if (world_file_version >= 3)
		fscanf(fp, "%d %ld %d %ld", &dummy, &detect, &o->kp, &floor);
	else
		fscanf(fp, "%ld %d %ld", &detect, &o->kp, &floor);
	o->detect = (long int)M(detect * 1000L);
	o->floor  = (long int)M(floor);

	o->timer = 0L;
}

static void kill(sObject *self)
{
    SAM *o;

    o = self->data;
	create_explo(o->p, zero, o->shape, FIXSPD(8));

	remove_target(self);
	if (o->dead) {
		o->shape = o->dead;
	} else {
    	remove_object(self);
	}
}

static void resupply(sObject *self)
{
    SAM *o;

    o = self->data;
	if (o->alive) {
		o->shape    = o->alive;
		o->strength = o->init_strength;
	}
}

void register_sam(void)
{
    SAM_class = new_class(sizeof(SAMClass), "SAM", sizeof(SAM), Cultural_class);

	SAM_class->file_read = file_read;
	SAM_class->update    = update_sam;
	SAM_class->kill	   	 = kill;

	add_message(SAM_class, "resupply", 0, resupply);
}

