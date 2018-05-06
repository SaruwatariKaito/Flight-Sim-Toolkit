/* --------------------------------------------------------------------------
 * WORLD		Copyright (c) Simis Ltd 1993,994,1995
 * ---			All Rights Reserved
 *
 * File:		world.c
 * Ver:			1.00
 * Desc:		World class and world database
 *
 * -------------------------------------------------------------------------- */
#include <math.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "world.h"

#include "ground.h"

#include "class.h"

static char world_error[100];

/* Useful functions provided by world library */
/* ------------------------------------------ */


void werror(char *format, ...)
{
    va_list argptr;

    va_start(argptr, format);
    vsprintf(world_error, format, argptr);
    va_end(argptr);

    debug("World Error : %s", world_error);
}
/* ----------------------------------------------
 * GLOBAL distance
 *
 */
int32 distance(sVector *v)
{
    return (v->z + ((ABS(v->x) + ABS(v->y)) >> 2));
}

int32 get_range(sVector *p1, sVector *p2, sVector *d)
{
    register int32 adx, adz;

    d->x = (p2->x - p1->x);
    d->y = (p2->y - p1->y);
    d->z = (p2->z - p1->z);
    adx = ABS(d->x);
    adz = ABS(d->z);
    if (adx > adz)
        return(adx + (adz >> 2));
    return(adz + (adx >> 2));
}

int32 get_shifted_range(sVector *p1, sVector *p2, sVector *d)
{
    register int32 adx, adz;

    d->x = (p2->x - p1->x) >> SPEED_SHIFT;
    d->y = (p2->y - p1->y) >> SPEED_SHIFT;
    d->z = (p2->z - p1->z) >> SPEED_SHIFT;
    adx = ABS(d->x);
    adz = ABS(d->z);
    if (adx > adz)
        return(adx + (adz >> 2));
    return(adz + (adx >> 2));
}

/* Random data tables
 * ------------------
 */

#define MAX_RAND 67

static int rand_data[MAX_RAND] = {
        0,  43, 20, 45,  7, 34, 32, 40, 24, 17, 25, 55,
		11, 19, 43, 52, 66, 53, 31, 36, 41, 96, 47, 47,
		65, 22, 29, 47, 49, 11,  1, 52,  2, 40, 16, 37,
	    34, 26, 12, 43, 88, 20, 65, 55, 26, 47,  8, 40,
		36, 46, 60, 26, 55, 41,  1, 25,  7, 37, 24, 10,
		53,  4,  5, 13, 60, 36, 32};


static sVector rand_tab[MAX_RAND] = {
        {-12,2,13}, {-12,-8,-3}, {-8,-1,-12}, {0,9,-13},
        {-9,2,8}, {-5,4,-5}, {-7,13,5}, {9,-16,-9},
        {-8,-3,-14}, {3,-3,-15}, {-16,-10,3}, {9,-11,9},
        {15,-4,-5}, {10,5,2}, {15,1,-12}, {-11,15,-12},
        {-7,10,-12}, {-16,-15,-13}, {3,6,7}, {-5,-7,7},
        {-7,0,1}, {14,-3,9}, {-7,-2,15}, {-6,-9,-12},
        {4,9,-2}, {-10,-7,-16}, {-13,-9,-14}, {6,-12,5},
        {-14,7,-11}, {2,-10,2}, {-13,-14,-4}, {15,1,8},
        {8,-14,5}, {-15,-13,-10}, {-15,13,-7}, {-4,3,3},
        {-1,4,-4}, {-2,-12,2}, {-8,3,-14}, {-1,-3,-5},
        {14,-1,12}, {1,-12,-1}, {-13,-3,8}, {-13,-15,4},
        {-6,14,-15}, {13,0,-7}, {-2,7,-14}, {-1,-9,2},
        {-11,-12,14}, {-6,-12,-2}, {-15,6,-2}, {-1,-15,-2},
        {-2,-14,-2}, {11,-16,8}, {-5,12,14}, {-15,-1,-12},
        {6,1,-9}, {7,6,-14}, {-15,-10,3}, {-8,-11,-6},
        {04,-1,12}, {7,-1,-14}, {-6,-6,9}, {-12,-13,-12},
        {-4,-5,6}, {-1,-1,-4}, {-15,-5,13}};

static int next_rand = 0;
static sVector r_vect = {0,0,0};

sVector *rand_vect(void)
{
    sVector *v;

    v = &rand_tab[next_rand++];
    if (next_rand >= MAX_RAND)
        next_rand = 0;
    r_vect.x = (int32)v->x;
    r_vect.y = (int32)v->y;
    r_vect.z = (int32)v->z;
    return &r_vect;
}

int fst_random(void)
{
	static int index = 0;

	index = (index >= MAX_RAND) ? 0 : index+1;
	return rand_data[index];
}

/* ----------------------------------------------------------------------------
 * Visual Database
 * --------------- */
static sVector no_viewer_posn = {0L, M(100), 0L};
sObject *viewer = NULL;

int c_sky = 0x2f, c_ground = 0x20;
int sun_colour = 0x14;

int shading_scale = 23;
int shading_on = TRUE;
int shaded_ground = FALSE;
int horizon_polygons = TRUE;


void set_viewer(sObject *o)
{
    viewer = o;
	ground_view_changed(o);
}

/* Direction and position of view in world coords */
static int view_direction = 0;
int view_elevation = 0;


sVector2 horizpts1[4];
sVector2 horizpts2[4];
/* -----------------------------------------------
 * GLOBAL draw_horizon
 *
 */ 
void draw_horizon(short int roll, int looking, int sun_col)
{
    int x, z, sa, ca;
    register int h1, h2;
    sVector v;
    sVector2 s1, s2, s3, s4;

    if (view_elevation > DEG(60)) {
        clear_prim(0, view_base, COL(c_sky));
		if (sun_col)
			draw_sun(sun_col);
    } else if (view_elevation < DEG(-60)) {
        clear_prim(0, view_base, COL(c_ground));
    } else {
        sa = SIN(view_direction); ca = COS(view_direction);
        x = -8192; z = 4096;
        v.y = 0;
        v.x = (MUL(x,ca) - MUL(z,sa)) >> SSHIFT;
        v.z = (MUL(x,sa) + MUL(z,ca)) >> SSHIFT;
        perspect(&v, &s1);
        s3 = s1;
        x = -x;
        v.x = (MUL(x,ca) - MUL(z,sa)) >> SSHIFT;
        v.z = (MUL(x,sa) + MUL(z,ca)) >> SSHIFT;
        perspect(&v, &s2);
        s4 = s2;
        clip_xedge(&s3, &s4);
        if (s3.y <= s4.y) {
            h1 = s3.y;
            h2 = s4.y;
        } else {
            h1 = s4.y;
            h2 = s3.y;
        }
        if ((h1 > clip_region.top) && (h2 < clip_region.bottom)) {
            if (ABS(roll) < DEG(90)) {
                clear_prim(0, h2, COL(c_sky));
				if (sun_col)
					draw_sun(sun_col);
                if (s3.y != s4.y) {
                    horizpts1[0].x = 0;
                    horizpts1[0].y = s3.y;
                    horizpts1[1].x = screen_width;
                    horizpts1[1].y = s4.y;
                    if (s3.y < s4.y)
                        horizpts1[2].x = 0;
                    else
                        horizpts1[2].x = screen_width;
                    horizpts1[2].y = h2;
                    polygon(3, horizpts1, c_ground);
                }
                clear_prim(h2, view_base, COL(c_ground));
            } else {
                clear_prim(h1, view_base, COL(c_sky));
				if (sun_col)
					draw_sun(sun_col);
                if (s3.y != s4.y) {
                    horizpts1[0].x = screen_width;
                    horizpts1[0].y = s3.y;
                    horizpts1[1].x = 0;
                    horizpts1[1].y = s4.y;
                    if (s3.y < s4.y)
                        horizpts1[2].x = 0;
                    else
                        horizpts1[2].x = screen_width;
                    horizpts1[2].y = h1;
                    polygon(3, horizpts1, c_ground);
                }
                clear_prim(0, h1, COL(c_ground));
            }
        } else {
            horizpts2[0].x = s2.x;
            horizpts2[0].y = s2.y;
            horizpts2[1].x = s1.x;
            horizpts2[1].y = s1.y;
            horizpts2[2].x = xcentre - (s1.y - ycentre);
            horizpts2[2].y = ycentre + (s1.x - xcentre);
            if ((s1.y > clip_region.bottom) && (s2.y > clip_region.bottom)) {
                if (ABS(roll) < DEG(90)) {
                    clear_prim(0, view_base, COL(c_sky));
					if (sun_col)
						draw_sun(sun_col);
                } else {
                    clear_prim(0, view_base, COL(c_ground));
				}
            } else {
                polygon(3, horizpts2, c_sky);
				if (sun_col)
					draw_sun(sun_col);
            }
            horizpts1[0].x = s1.x;
            horizpts1[0].y = s1.y - 1;
            horizpts1[1].x = s2.x;
            horizpts1[1].y = s2.y - 1;
            horizpts1[2].x = xcentre - (s2.y - ycentre);
            horizpts1[2].y = ycentre + (s2.x - xcentre);
            if ((s1.y < clip_region.top) && (s2.y < clip_region.top)) {
                if (ABS(roll) < DEG(90)) {
                    clear_prim(0, view_base, COL(c_ground));
                } else {
                    clear_prim(0, view_base, COL(c_sky));
					if (sun_col)
						draw_sun(sun_col);
				}
            } else {
                polygon(3, horizpts1, c_ground);
            }
        }
    }
}

static void draw_horizon_lines(sVector2 pts[], int nbands, int col)
{
	int dx, dy, x1, y1, x2, y2, dc;

	dy = pts[1].y - pts[2].y;
	if (dy < 0) dy = -dy;
	dx = pts[1].x - pts[2].x;
	if (dx < 0) dx = -dx;
	if (dx >= dy) {
		clip_xedge(&pts[1], &pts[2]);
		clip_xedge(&pts[0], &pts[3]);
		col <<= 8;
		if (pts[1].y >= pts[0].y) {
			dc = (nbands << 8) / (pts[1].y - pts[0].y);
			for (y1 = pts[0].y, y2 = pts[3].y; y1 <= pts[1].y; y1++, y2++) {
				line(pts[1].x, y1, pts[2].x, y2, col >> 8);
				col += dc;
			}
		} else {
			dc = (nbands << 8) / (pts[0].y - pts[1].y);
			for (y1 = pts[0].y, y2 = pts[3].y; y1 >= pts[1].y; y1--, y2--) {
				line(pts[1].x, y1, pts[2].x, y2, col >> 8);
				col += dc;
			}
		}
	} else {
		clip_yedge(&pts[1], &pts[2]);
		clip_yedge(&pts[0], &pts[3]);
		col <<= 8;
		if (pts[1].x >= pts[0].x) {
			dc = (nbands << 8) / (pts[1].x - pts[0].x);
			for (x1 = pts[0].x, x2 = pts[3].x; x1 <= pts[1].x; x1++, x2++) {
				line(x1, pts[1].y, x2, pts[2].y, col >> 8);
				col += dc;
			}
		} else {
			dc = (nbands << 8) / (pts[0].x - pts[1].x);
			for (x1 = pts[0].x, x2 = pts[3].x; x1 >= pts[1].x; x1--, x2--) {
				line(x1, pts[1].y, x2, pts[2].y, col >> 8);
				col += dc;
			}
		}
	}
}

static void draw_horizon_polygons(sVector2 pts[], int nbands, int col, int yfix)
{
	int i, dx, dy, x1, y1, x2, y2;

	dy = pts[1].y - pts[2].y;
	if (dy < 0) dy = -dy;
	dx = pts[1].x - pts[2].x;
	if (dx < 0) dx = -dx;
	if (dx >= dy) {
		clip_xedge(&pts[1], &pts[2]);
		clip_xedge(&pts[0], &pts[3]);
		dy = ((pts[1].y - pts[0].y) << 8) / nbands;
		y1 = pts[0].y << 8;
		y2 = pts[3].y << 8;
		for (i = 0; i < nbands; i++) {
			pts[0].y = (y1 >> 8);
			pts[3].y = (y2 >> 8);
			y1 += dy;
			y2 += dy;
			pts[1].y = (y1 >> 8) - yfix;
			pts[2].y = (y2 >> 8) - yfix;
			polygon(4, pts, col++);
		}
	} else {
		clip_yedge(&pts[1], &pts[2]);
		clip_yedge(&pts[0], &pts[3]);
		dx = ((pts[1].x - pts[0].x) << 8) / nbands;
		x1 = pts[0].x << 8;
		x2 = pts[3].x << 8;
		for (i = 0; i < nbands; i++) {
			pts[0].x = (x1 >> 8);
			pts[3].x = (x2 >> 8);
			x1 += dx;
			x2 += dx;
			pts[1].x = (x1 >> 8);
			pts[2].x = (x2 >> 8);
			polygon(4, pts, col++);
		}
	}
}

void draw_shaded_horizon(short int roll, int32 alt, int looking, int sun_col)
{
    register int i, x, z, sa, ca, gstep, yfix;
    sVector v;
    sVector2 s1, s2, s3, s4;

    if (view_elevation > DEG(60)) {
        clear_prim(0, view_base, COL(c_sky));
		if (sun_col)
			draw_sun(sun_col);
    } else if (view_elevation < DEG(-60)) {
        clear_prim(0, view_base, COL(c_ground));
    } else {
        if (ABS(roll) < DEG(90))
            yfix = 1;
        else
            yfix = -1;
        sa = SIN(view_direction); ca = COS(view_direction);
        /* Sky */
        x = -8192; z = 4096;
        v.y = 0L;
        v.x = (MUL(x,ca) - MUL(z,sa)) >> SSHIFT;
        v.z = (MUL(x,sa) + MUL(z,ca)) >> SSHIFT;
        perspect(&v, &s1);
        x = -x;
        v.x = (MUL(x,ca) - MUL(z,sa)) >> SSHIFT;
        v.z = (MUL(x,sa) + MUL(z,ca)) >> SSHIFT;
        perspect(&v, &s2);
		v.y += 700L;
		x = -x;
		v.x = (MUL(x,ca) - MUL(z,sa)) >> SSHIFT;
		v.z = (MUL(x,sa) + MUL(z,ca)) >> SSHIFT;
		perspect(&v, &s4);
		x = -x;
		v.x = (MUL(x,ca) - MUL(z,sa)) >> SSHIFT;
		v.z = (MUL(x,sa) + MUL(z,ca)) >> SSHIFT;
		perspect(&v, &s3);
		horizpts1[0] = s1;
		horizpts1[1].x = s4.x;
		horizpts1[1].y = s4.y - yfix;
		horizpts1[2].x = s3.x;
		horizpts1[2].y = s3.y - yfix;
		horizpts1[3] = s2;
		if (horizon_polygons) {
			draw_horizon_polygons(horizpts1, 7, c_sky - 7, yfix);
		} else {
			draw_horizon_lines(horizpts1, 7, c_sky - 7);
        }
        horizpts1[0].x = s3.x;
        horizpts1[0].y = s3.y;
        horizpts1[1].x = s4.x;
        horizpts1[1].y = s4.y;
        horizpts1[2].x = xcentre - (s4.y - ycentre);
        horizpts1[2].y = ycentre + (s4.x - xcentre);
        polygon(3, horizpts1, c_sky);
		if (sun_col)
			draw_sun(sun_col);
        /* Ground */
        if (alt > M(1600))
            gstep = 200;
        else
            gstep = (int)(alt >> 14) + 10;
        x = -8192; z = 4096;
        v.y = 0L;
        v.x = (MUL(x,ca) - MUL(z,sa)) >> SSHIFT;
        v.z = (MUL(x,sa) + MUL(z,ca)) >> SSHIFT;
        perspect(&v, &s4);
        x = -x;
        v.x = (MUL(x,ca) - MUL(z,sa)) >> SSHIFT;
        v.z = (MUL(x,sa) + MUL(z,ca)) >> SSHIFT;
        perspect(&v, &s3);
		if (shaded_ground) {
			for (i = 0; i < 7; i++) {
            	s1 = s4;
            	s2 = s3;
            	v.y -= gstep;
            	x = -x;
            	v.x = (MUL(x,ca) - MUL(z,sa)) >> SSHIFT;
            	v.z = (MUL(x,sa) + MUL(z,ca)) >> SSHIFT;
            	perspect(&v, &s4);
            	x = -x;
            	v.x = (MUL(x,ca) - MUL(z,sa)) >> SSHIFT;
            	v.z = (MUL(x,sa) + MUL(z,ca)) >> SSHIFT;
            	perspect(&v, &s3);
            	horizpts1[0].x = s1.x;
            	horizpts1[0].y = s1.y - yfix;
            	horizpts1[1].x = s2.x;
            	horizpts1[1].y = s2.y - yfix;
            	horizpts1[2] = s3;
            	horizpts1[3] = s4;
            	polygon(4, horizpts1, c_ground + 7 - i);
        	}
        }
        horizpts1[0].x = s4.x;
        horizpts1[0].y = s4.y - 1;
        horizpts1[1].x = s3.x;
        horizpts1[1].y = s3.y - 1;
        horizpts1[2].x = xcentre - (s3.y - ycentre);
        horizpts1[2].y = ycentre + (s3.x - xcentre);
        polygon(3, horizpts1, c_ground);
    }
}
/* -----------------------------------------------
 * GLOBAL draw_world
 *
 */ 
int draw_world(sObject *view, int azimuth, int elevation, int roll,
    void (*ground)(sObject *o, int dir))
{
    World *db;
    static int last_azimuth = -1;
    static sObject *last_viewer = NULL;
    register sList *l, *nl;
    sVector v;

	if (view == NULL)
		return -1;

    db = view->data;
	push_matrix();
	rotx(elevation);
    roty(azimuth);
	if (roll != 0)
		rotz(roll);
    rotz(db->r.z);
    rotx(db->r.x);
    roty(db->r.y);

	v.x = v.y = 0;
    v.z = Meter(100);
    invrotate(&v, &v);
    view_direction = vector_azimuth(&v);
    view_elevation = vector_elevation(&v);

	if (draw_sea_on)
		c_ground = 0x27;
	else
		c_ground = 0x20;
	if (shading_on)
		draw_shaded_horizon((short)(db->r.z), db->p.y, azimuth, sun_colour);
	else
		draw_horizon((short)(db->r.z), azimuth, sun_colour);
	if (ground)
       	(*ground)(view, azimuth);

	if (viewer) {
		SafeMethod(viewer, World, draw_2d)(viewer);
	}

	pop_matrix();

	last_azimuth = azimuth;

    return view_direction;
}


/* ----------------------------------------------------------------------------
 * Update DB objects
 * ----------------- */
sList *world_objects = NULL;
static sList *next_update_node   = NULL;
/* -----------------------------------------------
 * GLOBAL system_update
 *
 */ 
void system_update(sObject *o)
{
	World *w;

	w = o->data;
	/* Dubious optimisation */
/*	if (w->tp) {
		if (MethodValue(o,World, update) != NULL) {
        	if ((w->timer -= frame_time) <= 0) {
        		w->timer = 0;
            	SafeMethod(o, World, update)(o);
        	}
		}
	}*/
	if (MethodValue(o,World, update) != NULL) {
        if ((w->timer -= frame_time) <= 0) {
        	w->timer = 0;
            SafeMethod(o, World, update)(o);
        }
	}
}

/* -----------------------------------------------
 * GLOBAL moving_system_update
 *
 */
void moving_system_update(sObject *o)
{
	World *w;

	w = o->data;

	if (MethodValue(o,World, update) != NULL) {
        if ((w->timer -= frame_time) <= 0) {
        	w->timer = 0;
            SafeMethod(o, World, update)(o);
        }
	}

	move_in_tp(o, TRUE);

	if (MethodValue(o,World, check_collide) != NULL) {
        SafeMethod(o, World, check_collide)(o);
	}
}


/* -----------------------------------------------
 * GLOBAL system_update_world
 *
 */
void system_update_world(void)
{
    register sList *l;
    register sObject *o;
    World    *db;

    for (l = world_objects->next; l != world_objects; l = next_update_node) {
        next_update_node = l->next;
        o = (sObject*)l->item;
		db = o->data;
		if (db == NULL) {
			 stop("NULL instance data in system_update (%d %d)", o->id, class_id(o->class));
		}
        if (MethodValue(o,World, system_update) != NULL) {
            SafeMethod(o,World, system_update)(o);
        }
    }

    next_update_node = NULL;
}

/* All objects on update lists must be subclasses of World */
void update_world(void)
{
	system_update_world();
}

/* -----------------------------------------------
 * GLOBAL recolour_world
 *
 */
void recolour_world(void)
{
    sList *l;
    sObject *obj;
    World *o;
	sShape_def *sd;

    for (l = world_objects->next; l != world_objects; l = l->next) {
        obj = (sObject*)l->item;
		o = obj->data;
    	sd = o->shape;
		while (sd) {
        	recolour_shape(sd->s, &o->r);
			sd = sd->down;
    	}
    }
}

/* Default method for World class Attach method */
static int world_attach(sObject *o)
{
    WorldClass *c;
    World *w;

    c = o->class;
    w = o->data;

    if (w->flags & ATTACHED)
        return FALSE;

	add_to_tp(o, FALSE);

	if (c->system_update != NULL) {
        list_add_last(o, world_objects);
	    w->flags |= ATTACHED;

    	return TRUE;
	}
	return FALSE;
}

/* -----------------------------------------------
 * GLOBAL broadcast_detaching
 *
 */
void broadcast_detaching(sObject *old)
{
	sList *l;
	sObject *o;

	for (l = world_objects->next; l != world_objects; l = l->next) {
		o = l->item;
    	SafeMethod(o, World, detaching)(o, old);
	}
}

/* Default method for World class Detach method */

/* -----------------------------------------------
 * GLOBAL world_detach
 *
 */
int world_detach(sObject *o)
{
    WorldClass *c;
    World *w;

    c = o->class;
    w = o->data;

    if ((w->flags & ATTACHED) == 0)
        return FALSE;

	broadcast_detaching(o);
	/* Remove object from tp */
	if (w->tp != NULL) {
	  remove_from_tp(o);
	}

    /* If object is pointed to by next update node */
    if (next_update_node && (o == next_update_node->item)) {
        next_update_node = next_update_node->next;
    }

    if (c->system_update != NULL) {
        list_remove(o, world_objects);
	}
	w->flags &= ~DETACHING;
    w->flags &= ~ATTACHED;

    return TRUE;
}

static sList *dying_objects = NULL;
static sList *detaching_objects = NULL;

/* -----------------------------------------------
 * GLOBAL attach_object
 *
 */
int attach_object(sObject *o)
{
    SafeMethod(o, World, attach)(o);

	return TRUE;
}

/* -----------------------------------------------
 * GLOBAL remove_objects
 *
 */
void remove_objects(void)
{
  sObject *o;

  while ((o = list_remove_first(detaching_objects)) != NULL) {
	debug("remove %d\n", o->id);
    SafeMethod(o, World, detach)(o);
  }
  while ((o = list_remove_first(dying_objects)) != NULL) {
	debug("remove %d\n", o->id);
    SafeMethod(o, World, detach)(o);
	destroy_object(o);
  }
}

/* Adds object to set of objects to be removed/detached at end of cycle */

/* -----------------------------------------------
 * GLOBAL remove_object
 *
 */
void remove_object(sObject *o)
{
	World *w;

	w = o->data;
	if ((w->flags & DETACHING) == 0) {
		w->flags |= DETACHING;
		list_set_add(o, dying_objects);
	}
}

/* -----------------------------------------------
 * GLOBAL detach_object
 *
 */
void detach_object(sObject *o)
{
	World *w;

	w = o->data;
	w->flags |= DETACHING;
	list_set_add(o, detaching_objects);
}


/* ------------------------------------------------------------------------- */


/* -----------------------------------------------
 * GLOBAL check_collisions
 * Check for collisions between object obj and rest of world
 * self must have a collide method
 *
 */
sObject *check_collisions(sObject *self)
{
	sObject *obj;
	World  *o, *tgt;
	sList   *collide_list, *l;
	int     kp;

	if (MethodValue(self, World, collide) == NULL)
		return NULL;

	o = self->data;
	/* Find list of object which self could collide with */
	collide_list = get_tp_objects(self);
	if (collide_list) {
		Map (l, collide_list) {
			obj = l->item;
			if (obj == self)
				continue;
			tgt = obj->data;
			if (!(tgt->flags & DETACHING) && tgt->shape) {
				kp = CallMethod(self, World, collide)(self, obj);
				if (kp > 0) {
					/* Tell the object that it has been hit */
					if (CallMethod(obj, World, hit)(obj, self, kp) > 0)
						return obj;
				}
			}
		}
	}
    return NULL;
}

/* -----------------------------------------------
 * GLOBAL set_object_radius
 *
 */
void set_object_radius(sObject *self)
{
	int scale, max_x, max_z;
	sShape_def *sd;
	World *o;

	o = self->data;
	sd = o->shape;
	if (sd) {
		while (sd->down) {
			sd = sd->down;
		}
		scale = sd->s->scale;
		o->radius = (int32)sd->s->radius << scale;
		max_x = MAX(-sd->s->min_size.x, sd->s->max_size.x);
		max_z = MAX(-sd->s->min_size.z, sd->s->max_size.z);
		o->flat_radius = (int32)MAX(max_x, max_z) << scale;
	} else {
		o->radius = 0;
		o->flat_radius = 0;
	}
}

/* Scoring */

int scoring_on = TRUE;
static sList *scored_shapes = NULL;
static sList *killed_shapes = NULL;
static sObject *last_hitter = NULL;

/* -----------------------------------------------
 * GLOBAL player_scored_hit
 *
 */
void player_scored_hit(sObject *hitter)
{
	last_hitter = hitter;
}

/* -----------------------------------------------
 * GLOBAL target_scoring
 *
 */
void target_scoring(sObject *killed, sObject *killer)
{
	World *o;
	sShape_def *sd;

	if (scoring_on) {
		o = killed->data;
		sd = o->shape;
		list_add_last(sd->name, killed_shapes);
		if (killer == last_hitter) {
			last_hitter = NULL;
			list_add_last(sd->name, scored_shapes);
		}
	}
}

/* -----------------------------------------------
 * GLOBAL write_score
 *
 */
void write_score(void)
{
	FILE *fp;
	char *name;

	fp = fopen("SCORE.DAT", "w");
	if (fp) {
		fprintf(fp, "Killed by player\n");
		if (scored_shapes) {
			while ((name = list_remove_first(scored_shapes)) != NULL) {
				fprintf(fp, "%s\n", name);
			}
		}
		fprintf(fp, "All killed objects\n");
		if (killed_shapes) {
			while ((name = list_remove_first(killed_shapes)) != NULL) {
				fprintf(fp, "%s\n", name);
			}
		}
		fclose(fp);
	}
}


/* ------------------------------------------------------------------------- */

WorldClass *World_class = NULL;

static void destroy(sObject *self)
{
    BaseClass *c;

    c = self->class;
    block_free(self->data, c->sizeof_data);
}

static void file_save(sObject *self, FILE *fp)
{
    World *o;
    sShape_def *sd;

    o = self->data;
    fprintf(fp, "%d\n", o->flags);
    SaveVector(fp, o->p);

    SaveVector(fp, o->r);
    fprintf(fp, "%ld\n", o->radius);
    if (o->shape) {
        sd = o->shape;
        fprintf(fp, "%s\n", sd->name);
    } else {
        fprintf(fp, "NULL\n");
    }
}

static void file_load(sObject *self, FILE *fp)
{
    World *o;
	sVector min, max;
    char name[32];

    o = self->data;
    fscanf(fp, "%d\n", &o->flags);
	o->flags &= ~DETACHING;
    o->flags &= ~ATTACHED;
    LoadVector(fp, o->p);
    debug("World load\n");
    LoadVector(fp, o->r);
    fscanf(fp, "%ld\n", &o->radius);
    fscanf(fp, "%s\n", name);
    if (strcmp(name, "NULL") == 0) {
        o->shape = NULL;
    } else {
        o->shape = get_shape(name);
    }
	set_object_radius(self);
	attach_object(self);
    debug("World load.\n");
}

static int draw(sObject *self)
{
    World *o;

    o = self->data;
    if (o->shape) {
        return(draw_shape_def((sShape_def *)o->shape, &o->p, &o->r, FALSE));
    }
	return FALSE;
}

static void world_kill(sObject *o)
{
    World *db;

    db = o->data;

	small_bang(Posn(o));
    /* create_explo(db->p, szero, db->shape, FIXSPD(8)); */
    remove_object(o);
}

static void world_detaching(sObject *self, sObject *old)
{
	return;
}

/* Method used to read in an inital world state */
static void world_read(sObject *self, FILE *fp)
{
    World *o;
	long int x,y,z,f,r;
	char shape[32];

    o = self->data;
    fscanf(fp, "%ld %ld %ld %d %d ", &x, &y, &z, &r, &f);
    fscanf(fp, "%s\n", shape);
	o->p.x = M(x); o->p.y = M(y); o->p.z = M(z);
	o->flags = f;
	o->r.y = DEG(r);
    o->shape = get_shape(shape);
}

static int world_hit(sObject *self, sObject *collide, int hit)
{
  if (self != collide && MethodExists(self, World, kill))
	 return hit;
  return 0;
}

sObject *new_world_obj(long int x, long int y, long int z, sShape_def *sd)
{
    sObject *o;
    World *db;

    o = new_object(World_class);
    if (o) {
        db = o->data;
        db->p.x = x;
        db->p.y = y;
        db->p.z = z;
        db->shape = sd;
    }
    return o;
}

/* -----------------------------------------------
 * GLOBAL register_World
 *
 */
void register_World(void)
{
    World_class = new_class(sizeof(WorldClass), "World", sizeof(World), Base_class);

    /* Overide Obj Class methods */
    Overide(World_class, destroy,   destroy);
    Overide(World_class, file_load, file_load);
    Overide(World_class, file_save, file_save);

    /* Basic World Class Methods */
    World_class->attach = world_attach;
    World_class->detach = world_detach;

    World_class->system_update= system_update;
    World_class->update       = NULL;

    World_class->draw           = draw;
    World_class->draw_2d        = NULL;

    World_class->hit            = world_hit;
    World_class->collide        = NULL;
    World_class->kill           = world_kill;
    World_class->detaching 		= world_detaching;
    World_class->file_read      = world_read;
}

/* -------------------------------------------------------------------------*/

/* -----------------------------------------------
 * GLOBAL init_world
 *
 */
void init_world(void)
{
    init_mem();
    init_lists();
    init_symbols();

/*	init_ground();*/

    init_objects();
}

/* -----------------------------------------------
 * GLOBAL setup_world
 *
 */
void setup_world(void)
{
    setup_symbols();
    setup_shape();

/*	setup_ground();*/

    /* Creates Basic class Obj */
    setup_objects();

    register_World();
    register_mobile();
    register_bangs();
    register_bullets();
    register_explo();
    register_smoke();

    init_all_classes();

    world_objects = new_list();
    dying_objects = new_list();
    detaching_objects = new_list();
	scored_shapes = new_list();
	killed_shapes = new_list();
}

/* -----------------------------------------------
 * GLOBAL reset_world
 *
 */ 
void reset_world(void)
{
}

/* -----------------------------------------------
 * GLOBAL tidyup_world
 *
 */ 
void tidyup_world(void)
{
    tidyup_symbols();
}
