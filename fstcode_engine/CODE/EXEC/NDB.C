/* --------------------------------------------------------------------------
 * EXEC 		Copyright (c) Simis Ltd 1993,994,1995
 * ---			All Rights Reserved
 *
 * File:		ndb.c
 * Ver:			1.00
 * Desc:		Non Directional Beacon Class
 *
 * -------------------------------------------------------------------------- */
#include <stdlib.h>
#include "ndb.h"
#include "defs.h"

/* Declare Class Pointer */
NDBClass *NDB_class = NULL;
sObject *selected_NDB = NULL;

static sList *NDB_list = NULL;

extern int adf;
extern int adf_bearing;

#define FREQ_MIN 0.00
#define FREQ_MAX 500.00

/* Class Methods */

/* Scan through all NDB's in range for next
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
	debug("DF %f %f %d - %d\n", from, to, up, dist);

	return dist;
}

sObject *scan_NDBs(int up)
{
	float  df, current_freq, nearest_df;
	sObject *o, *nearest;
	NDB    *ndb;
	sList   *l;

	nearest = NULL;
	if (selected_NDB) {
		ndb = selected_NDB->data;
		current_freq = ndb->freq;
	} else {
		current_freq = 0.0;
	}

	Map (l, NDB_list) {
		o   = l->item;
		if (o == selected_NDB)
			continue;
		ndb = o->data;
		df  = freq_distance(current_freq, ndb->freq, up);
		if (nearest == NULL || df < nearest_df) {
			nearest = o;
			nearest_df = df;
		}
	}
	if (nearest) {
		selected_NDB = nearest;
		ndb = selected_NDB->data;
		adf = (int)(ndb->freq * 100);
	}
	return nearest;
}

/* Work out adf bearing */
void update_adf(void)
{
    sVector d, ad;
	World *ac;
	NDB *o;

	if (selected_NDB && Player) {
		ac = Player->data;
		o  = selected_NDB->data;

    	d.x = o->p.x - ac->p.x;
    	d.y = o->p.y - ac->p.y;
    	d.z = o->p.z - ac->p.z;
    	ad.x = ABS(d.x);
    	ad.y = ABS(d.y);
    	ad.z = ABS(d.z);
    	adf_bearing = (ac->r.y-fast_atan(-d.x, d.z)) / DEG_1;
    }
}

/* Object Methods */

static void create_ndb(sObject *o)
{
    BaseClass *c;

    c = o->class;
    o->data = block_alloc(c->sizeof_data);
	if (o->data == NULL)
		stop("No memory for new NDB");

	list_add(o, NDB_list);
}

static void destroy(sObject *self)
{
    BaseClass *c;

    c = self->class;
	list_remove(self, NDB_list);
    block_free(self->data, c->sizeof_data);
}

/* Method used to read in an inital world state */
static void file_read(sObject *self, FILE *fp)
{
	Basic_data data;
    NDB *o;
	float freq;

    o = self->data;

	file_read_basic(&data, fp);

	/* class data - none */
	fscanf(fp, "%f", &freq);

	o->p.x = M(data.x); o->p.y = M(data.y); o->p.z = M(data.z);
	o->flags = data.f;
	o->r.y = DEG(data.ry);
    o->alive = o->shape = get_shape(data.shape);
	o->dead  = get_shape(data.dead);
	o->init_strength = o->strength = data.strength;
	o->freq  = freq;
	set_object_radius(self);
}

void register_ndb(void)
{
    NDB_class = new_class(sizeof(NDBClass), "NDB", sizeof(NDB), Cultural_class);

    NDB_class->create    = create_ndb;
    NDB_class->destroy   = destroy;
	NDB_class->file_read = file_read;

	NDB_list = new_list();
}

