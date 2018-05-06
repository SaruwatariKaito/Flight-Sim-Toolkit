/* --------------------------------------------------------------------------
 * EXEC 		Copyright (c) Simis Ltd 1993,994,1995
 * ---			All Rights Reserved
 *
 * File:		vor.c
 * Ver:			1.00
 * Desc:		VOR class
 *
 * Very high frequency Omidirectional Radial beacon.
 * Referenced by OBI gauge.
 * 
 * -------------------------------------------------------------------------- */
#include <stdlib.h>
#include "vor.h"
#include "defs.h"

/* Declare Class Pointer */
VORClass *VOR_class = NULL;

#define MAX_VOR 2
sObject *selected_VOR[MAX_VOR] = {NULL, NULL};

static sList *VOR_list = NULL;

extern int nav1, nav2;
extern int nav1_bearing;
extern int nav2_bearing;
extern int nav1_distance;
extern int nav2_distance;

#define FREQ_MIN 108.00
#define FREQ_MAX 118.95

/* Class Methods */

/* Scan through all VOR's in range for next
 * beacon */

static float freq_distance(float from, float to, int up)
{
	float dist;

	if (to == from)
		 return 0.0;

	if (up) {
		if (to > from) {
		  dist = to - from;
	  	} else {
		  dist = (FREQ_MAX - from) + (to - FREQ_MIN);
		}
	} else {
		if (to < from) {
		  dist = from - to;
	  	} else {
		  dist = (from - FREQ_MIN) + (FREQ_MAX - to);
		}
	}

	return dist;
}


/* -------------------
 * GLOBAL scan_VORs
 *
 * Search up/down VOR_list for beacon with next
 * frequency.
 */
sObject *scan_VORs(int n, int up)
{
	float  df, current_freq, nearest_df;
	sObject *o, *nearest;
	VOR    *vor;
	sList   *l;

	nearest = NULL;
	if (selected_VOR[n]) {
		vor = selected_VOR[n]->data;
		current_freq = vor->freq;
	} else {
		current_freq = 0.0;
	}

	Map (l, VOR_list) {
		o   = l->item;
		if (o == selected_VOR[n])
			continue;
		vor = o->data;
		df  = freq_distance(current_freq, vor->freq, up);
		if (nearest == NULL || df < nearest_df) {
			nearest = o;
			nearest_df = df;
		}
	}
	if (nearest) {
		selected_VOR[n] = nearest;
		vor = selected_VOR[n]->data;
		switch (n) {
			case 0:
				nav1 = (int)(vor->freq * 100);
				break;
			case 1:
				nav2 = (int)(vor->freq * 100);
				break;
		}
	}
	return selected_VOR[n];
}

static void create_vor(sObject *o)
{
    BaseClass *c;

    c = o->class;
    o->data = block_alloc(c->sizeof_data);
	if (o->data == NULL)
		stop("No memory for new VOR");

	list_add(o, VOR_list);
}

static void destroy(sObject *self)
{
    BaseClass *c;

    c = self->class;
	list_remove(self, VOR_list);
    block_free(self->data, c->sizeof_data);
}

void register_vor(void)
{
    VOR_class = new_class(sizeof(VORClass), "VOR", sizeof(VOR), NDB_class);

    VOR_class->create    = create_vor;
    VOR_class->destroy   = destroy;

	VOR_list = new_list();
}

/* Work out adf bearing */
void update_obi(void)
{
    sVector d, ad;
	World *ac;
	VOR   *o;
	int32 dist, range;

	if (selected_VOR[0] && Player) {
		ac = Player->data;
		o  = selected_VOR[0]->data;

    	d.x = o->p.x - ac->p.x;
    	d.y = o->p.y - ac->p.y;
    	d.z = o->p.z - ac->p.z;
    	ad.x = ABS(d.x);
    	ad.y = ABS(d.y);
    	ad.z = ABS(d.z);
    	nav1_bearing = fast_atan(-d.x, d.z);
    	dist = MAX(ad.x, ad.z);
    	range = MAX(dist, ad.y);
		nav1_distance = range/M(1);
    }

	if (selected_VOR[1] && Player) {
		ac = Player->data;
		o  = selected_VOR[1]->data;

    	d.x = o->p.x - ac->p.x;
    	d.y = o->p.y - ac->p.y;
    	d.z = o->p.z - ac->p.z;
    	ad.x = ABS(d.x);
    	ad.y = ABS(d.y);
    	ad.z = ABS(d.z);
    	nav2_bearing = fast_atan(-d.x, d.z);
    	dist = MAX(ad.x, ad.z);
    	range = MAX(dist, ad.y);
		nav2_distance = range/M(1);
    }
}
