/* --------------------------------------------------------------------------
 * EXEC 		Copyright (c) Simis Ltd 1993,994,1995
 * ---			All Rights Reserved
 *
 * File:		ils.c
 * Ver:			1.00
 * Desc:		Instrument Landing System (ILS) class
 *
 * -------------------------------------------------------------------------- */
#include <stdlib.h>
#include "ils.h"
#include "defs.h"

extern int ils;
extern int ils_radial;
extern int ils_bearing;
extern int ils_elev;

/* Declare Class Pointer */
ILSClass *ILS_class = NULL;

static sList *ILS_list = NULL;

static sObject *closest_ILS = NULL;


/* --------------------
 * METHOD update_ils
 * 
 * Checks for distance to Player - if closest ILS
 * then sets itself up as active ILS object
 */ 
static void update_ils(sObject *self)
{
    sVector ad, d;
	ILS *o, *current;
	World *ac;
	int32 dist, range;

	o = self->data;
    o->timer = SECS(5);

	if ((self == closest_ILS) && Player) {
		ac = Player->data;

    	d.x = o->p.x - ac->p.x;
    	d.y = o->p.y - ac->p.y;
    	d.z = o->p.z - ac->p.z;
    	ad.x = ABS(d.x);
    	ad.y = ABS(d.y);
    	ad.z = ABS(d.z);

    	dist = MAX(ad.x, ad.z);
    	range = MAX(dist, ad.y);

    	ils_radial  = o->r.y;
    	ils_bearing = fast_atan(-d.x, d.z);
        ils_elev    = -fast_atan(d.y, dist);
		ils         = (int)(o->freq * 100);
    	o->timer = 0;
	}
}

static void create_ils(sObject *o)
{
    BaseClass *c;

    c = o->class;
    o->data = block_alloc(c->sizeof_data);
	if (o->data == NULL)
		stop("No memory for new ILS");
	list_add(o, ILS_list);
}

static void destroy(sObject *self)
{
    BaseClass *c;

	list_remove(self, ILS_list);
    c = self->class;
    block_free(self->data, c->sizeof_data);
}

void register_ils(void)
{
    ILS_class = new_class(sizeof(ILSClass), "ILS", sizeof(ILS), NDB_class);

	ILS_class->create  = create_ils;
	ILS_class->destroy = destroy;
	ILS_class->update  = update_ils;

	ILS_list = new_list();
}


/* -------------------
 * GLOBAL select_nearest_ils
 * 
 * selectes closest ILS to player
 */
void select_nearest_ils(void)
{
    sVector d;
	int32 range, nearest_range;
	sList *l;
	sObject *o, *nearest;
	ILS    *ils;

	nearest_range = 0L;
	nearest = NULL;
	Map (l, ILS_list) {
		o = l->item;
		ils = o->data;
		range = get_range(Posn(Player), &ils->p, &d);
		if (nearest == NULL || range < nearest_range) {
			nearest_range = range;
			nearest = o;
		}
	}
	closest_ILS = nearest;
}
