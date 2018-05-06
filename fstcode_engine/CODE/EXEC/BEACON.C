/* --------------------------------------------------------------------------
 * EXEC 		Copyright (c) Simis Ltd 1993,994,1995
 * ---			All Rights Reserved
 *
 * File:		beacon.c
 * Ver:			2.00
 * Desc:		Implments NDB and VOR navigation beacons
 *
 * -------------------------------------------------------------------------- */
#include <stdlib.h>
#include "beacon.h"

/* Declare Class Pointer */
BeaconClass *Beacon_class = NULL;
BeaconClass *NDB_class    = NULL;
BeaconClass *VOR_class    = NULL;

static void file_read(sObject *self, FILE *fp)
{
    Beacon *o;
	long int x,y,z,f,r;
	char shape[32];

    o = self->data;
    fscanf(fp, "%ld %ld %ld %d %d ", &x, &y, &z, &r, &f);
    fscanf(fp, "%s\n", shape);

    fscanf(fp, "%ld", &o->freq);
	o->p.x = M(x); o->p.y = M(y); o->p.z = M(z);
	o->flags = NULL;
	o->r.y = DEG(r);
    o->shape = get_shape(shape);
	set_object_radius(self);
}

void register_beacon(void)
{
    Beacon_class = new_class(sizeof(BeaconClass), "Beacon", sizeof(Beacon), World_class);
	Beacon_class->file_read = file_read;

	NDB_class = copy_class(Beacon_class, "NDB");
	VOR_class = copy_class(Beacon_class, "VOR");
}

