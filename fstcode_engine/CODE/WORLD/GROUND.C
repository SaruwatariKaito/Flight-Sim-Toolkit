/* --------------------------------------------------------------------------
 * WORLD		Copyright (c) Simis Ltd 1993,994,1995
 * ---			All Rights Reserved
 *
 * File:		ground.c
 * Ver:			1.00
 * Desc:		Draws ground database + objects in database
 *
 * -------------------------------------------------------------------------- */
/* Terrain loading and drawing */
/* Each terrain file is 1 DEG by 1 DEG in latitude longitude */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "ground.h"

#define GREEN 0x11

int draw_sea_on = TRUE;
int draw_ground_on = TRUE;
int ground_grid_on = FALSE;
int ground_grid_size = 1;

typedef struct {
	short *heights;
	uchar *colours;
} Terrain_block;

static Terrain_block terrain_data[2][2] = {
	{{NULL,NULL},{NULL,NULL}},{{NULL,NULL},{NULL,NULL}},
};

static int block_x = 10000, block_y = 10000;	/* Initialised for check_terrain_data */
static int block_i = 1, block_j = 1;

static int32 view_x = 0, view_z = 0;
static int real_terrain_x = 0, real_terrain_y = 0;
static int terrain_x = 0, terrain_y = 0;
static int last_terrain_x = -20000, last_terrain_y = -20000;
static int terrain_changed = TRUE;

static int32 terrain_centre_x = 0;
static int32 terrain_centre_z = 0;

#define TERRAIN_SIZE 100
static int terrain_width = TERRAIN_SIZE;
static int terrain_height = TERRAIN_SIZE;
static int terrain_min_height = 0;
static int terrain_max_height = 15;

#define HEADER_SIZE 128

#define DEFAULT_GRID_GAP 500
#define TSHIFT 2
static int grid_gap = DEFAULT_GRID_GAP;
static int32 big_grid_gap = M(DEFAULT_GRID_GAP);
static int tgrid_gap = Meter(DEFAULT_GRID_GAP) >> TSHIFT;
static int32 sgrid_gap = Meter(DEFAULT_GRID_GAP) << (SSHIFT - TSHIFT);
static int32 sgrid_radius = Meter(DEFAULT_GRID_GAP + (DEFAULT_GRID_GAP/2)) << (SSHIFT - TSHIFT);

static int32 block_width = TERRAIN_SIZE * M(DEFAULT_GRID_GAP);
static int32 block_height = TERRAIN_SIZE * M(DEFAULT_GRID_GAP);

static int looking_down = FALSE;

#define INVIEW 1
#define TRANSFORMED 2
#define DRAW_OBJECTS 4

typedef struct {
	int height;
	short colour, flags;
	sList *objects;
	sVector tpt;
	sVector2 spt;
} Terrain_point;

static void reset_tp(void);
static void update_tp(void);
static void transform_tp(void);
static void draw_transformed(void);

#define TSTEP 1
/*#define TX1 -32
#define TY1 -32
#define TX2 31
#define TY2 31
#define TSIZE 65
#define COL_SHIFT 2*/
#define TX1 -16
#define TY1 -16
#define TX2 15
#define TY2 15
#define TSIZE 33
#define COL_SHIFT 1

int distance_factor = 1;

static Terrain_point *tp_lines[TSIZE];
#define TPT(X,Y) &tp_lines[Y][X]

/* Transform world position into terrain index relative to centre */
#define TX(X) ((X >= 0) ? \
	((X / big_grid_gap) - real_terrain_x) : \
	(((X - (big_grid_gap - 1)) / big_grid_gap) - real_terrain_x))

#define TY(Z) ((Z >= 0) ? \
	((Z / big_grid_gap) - real_terrain_y) : \
	(((Z - (big_grid_gap - 1)) / big_grid_gap) - real_terrain_y))

#define COL_TRI_STEP 0x04
#define MAX_GROUND_COL 0x70
#define SEA_COL 0x20

static void set_grid_gap(int size);
static void check_terrain_data(void);
static void get_point_data(int tx, int ty, Terrain_point *tp);
static int get_square_colour(int tx, int ty, int base_col, int checker, int *dist_shade);
static int get_distance_shade(int tx, int ty);
static void draw_quad_north_east(int x1, int y1, int size);
static void draw_quad_north_west(int x1, int y1, int size);
static void draw_quad_south_east(int x1, int y1, int size);
static void draw_quad_south_west(int x1, int y1, int size);
static void draw_quad_east_north(int x1, int y1, int size);
static void draw_quad_east_south(int x1, int y1, int size);
static void draw_quad_west_north(int x1, int y1, int size);
static void draw_quad_west_south(int x1, int y1, int size);
static void draw_square(int x1, int z1, int step);
static void draw_clipped_square(int x1, int y1, int step);
static void draw_square_grid(int x1, int y1, int step);
static void draw_terrain_objects(sList *objects);

static void (* quad_calls[8])(int, int, int) = {
	draw_quad_north_west,
	draw_quad_west_north,
	draw_quad_west_south,
	draw_quad_south_west,
	draw_quad_south_east,
	draw_quad_east_south,
	draw_quad_east_north,
	draw_quad_north_east,
};

static int clipped_squares = 0;
static char msg[80];


/* -------------------
 * GLOBAL draw_terrain
 * Draw terrain database from viewpoint of view
 * 
 */
void draw_terrain(sObject *view, int dir)
{
	unsigned quad;
	int32 x, z;
    World *m;

    m = view->data;
	clipped_squares = 0;
	looking_down = (view_elevation < DEG(-30)) ? TRUE : FALSE;
	view_x = m->p.x;
	view_z = m->p.z;
	real_terrain_x = view_x / big_grid_gap;
	terrain_x = real_terrain_x % terrain_width;
	if (view_x < 0) {
		real_terrain_x--;
		terrain_x--;
	}
	real_terrain_y = view_z / big_grid_gap;
	terrain_y = real_terrain_y % terrain_height;
	if (view_z < 0) {
		real_terrain_y--;
		terrain_y--;
	}
	if ((real_terrain_x == last_terrain_x) && (real_terrain_y == last_terrain_y)) {
		terrain_changed = FALSE;
	} else {
		terrain_changed = TRUE;
		update_tp();
	}
	reset_tp();
	x = view_x % big_grid_gap;
	if (x < 0)
		x += big_grid_gap;
	x >>= (TSHIFT + SPEED_SHIFT);
	z = view_z % big_grid_gap;
	if (z < 0)
		z += big_grid_gap;
	z >>= (TSHIFT + SPEED_SHIFT);
	push_matrix();
	translate_abs(-x, -m->p.y >> (TSHIFT + SPEED_SHIFT), -z);
	dir = ADDANGLE(m->r.y, dir);
	quad = ((unsigned)(dir & 0xffff) / DEG(45)) & 7;
    (* quad_calls[quad])(TX1, TY1, TSIZE);
	pop_matrix();
	last_terrain_x = real_terrain_x;
	last_terrain_y = real_terrain_y;
}


/* --------------------
 * LOCAL init_tp
 * 
 * Allocate and initialise terrain point array
 */ 
static void init_tp(void)
{
	int i;
	Terrain_point *tp;

	tp = heap_alloc(TSIZE*TSIZE*sizeof(Terrain_point));
	if (tp == NULL)
		stop("No memory for terrain points");
	for (i = 0; i < TSIZE; i++, tp += TSIZE)
		tp_lines[i] = tp;
	tp = tp_lines[0];
	for (i = TSIZE * TSIZE; i--; tp++) {
		tp->flags = 0;
		tp->objects = NULL;
	}
}


/* --------------------
 * LOCAL reset_tp
 * 
 * Reset terrain point array
 */ 
static void reset_tp(void)
{
	int i;
	Terrain_point *tp;

	tp = TPT(0, 0);
	for (i = TSIZE * TSIZE; i--; tp++)
		tp->flags &= ~(INVIEW | TRANSFORMED);
}


/* --------------------
 * LOCAL full_update_tp
 * 
 * Remove all objects from terrain database and
 * re-enter all objects from world database
 */ 
static void full_update_tp(void)
{
	int x, y, tx, ty;
	Terrain_point *tp;
	sList *l;
	sObject *o;
	World *w;

	tx = terrain_x;
	ty = terrain_y;
	for (y = 0; y < TSIZE; y++) {
		tp = TPT(0, y);
		for (x = 0; x < TSIZE; x++, tp++) {
			tp->flags &= ~DRAW_OBJECTS;
			get_point_data(x + TX1 + tx, y + TY1 + ty, tp);
			if (tp->objects) {
				for (l = tp->objects->next; l != tp->objects; l = l->next) {
				  o = l->item;
				  w = o->data;
				  if (w == NULL)
				    stop("Instance data NULL in update_tp");
				  w->tp = NULL;
				}
				list_destroy_all(tp->objects);
				tp->objects = NULL;
			}
		}
	}
	for (l = world_objects->next; l != world_objects; l = l->next) {
		o = l->item;
		w = o->data;
		x = TX(w->p.x);
		y = TY(w->p.z);
		if ((x >= TX1) && (x <= TX2) && (y >= TY1) && (y <= TY2)) {
			tp = TPT(x-TX1, y-TY1);
			if (tp->objects == NULL)
				tp->objects = new_list();
			list_add(o, tp->objects);
			w->tp = tp;
		}
	}
}

static void partial_update_tp1(void)
{
	int x, y, tx, ty;
	Terrain_point *tp;

	tx = terrain_x;
	ty = terrain_y;
	for (y = 0; y < TSIZE; y++) {
		tp = TPT(0, y);
		for (x = 0; x < TSIZE; x++, tp++) {
			tp->flags &= ~DRAW_OBJECTS;
			get_point_data(x + TX1 + tx, y + TY1 + ty, tp);
		}
	}
}

static void partial_update_tp2(void)
{
	int x, y;
	Terrain_point *tp;
	sList *l;
	sObject *o;
	World *w;

	for (l = world_objects->next; l != world_objects; l = l->next) {
		o = l->item;
		w = o->data;
		if (w->tp == NULL) {
			x = TX(w->p.x);
			y = TY(w->p.z);
			if ((x >= TX1) && (x <= TX2) && (y >= TY1) && (y <= TY2)) {
				tp = TPT(x-TX1, y-TY1);
				if (tp->objects == NULL)
					tp->objects = new_list();
				list_add(o, tp->objects);
				w->tp = tp;
			}
		}
	}
}

static void move_objects_right(void)
{
	int x, y;
	Terrain_point *tp1, *tp2;
	sList *l;
	sObject *o;
	World *w;

	for (y = 0; y < TSIZE; y++) {
		tp1 = TPT(TX2-TX1, y);
		if (tp1->objects) {
			for (l = tp1->objects->next; l != tp1->objects; l = l->next) {
				o = l->item;
				w = o->data;
				if (w == NULL)
					stop("Instance data NULL in move_objects_right");
				w->tp = NULL;
			}
			list_destroy_all(tp1->objects);
			tp1->objects = NULL;
		}
	}
	for (y = 0; y < TSIZE; y++) {
		tp1 = TPT(TX2-TX1, y);
		for (x = TX2; x > TX1; x--, tp1--) {
			tp2 = tp1 - 1;
			tp1->objects = tp2->objects;
			if (tp1->objects) {
				for (l = tp1->objects->next; l != tp1->objects; l = l->next) {
					o = l->item;
					w = o->data;
					w->tp = tp1;
				}
			}
		}
	}
	for (y = 0; y < TSIZE; y++) {
		tp1 = TPT(0, y);
		tp1->objects = NULL;
	}
}

static void move_objects_left(void)
{
	int x, y;
	Terrain_point *tp1, *tp2;
	sList *l;
	sObject *o;
	World *w;

	for (y = 0; y < TSIZE; y++) {
		tp1 = TPT(0, y);
		if (tp1->objects) {
			for (l = tp1->objects->next; l != tp1->objects; l = l->next) {
				o = l->item;
				w = o->data;
				if (w == NULL)
					stop("Instance data NULL in update_tp");
				w->tp = NULL;
			}
			list_destroy_all(tp1->objects);
			tp1->objects = NULL;
		}
	}
	for (y = 0; y < TSIZE; y++) {
		tp1 = TPT(0, y);
		for (x = TX1; x < TX2; x++, tp1++) {
			tp2 = tp1 + 1;
			tp1->objects = tp2->objects;
			if (tp1->objects) {
				for (l = tp1->objects->next; l != tp1->objects; l = l->next) {
					o = l->item;
					w = o->data;
					w->tp = tp1;
				}
			}
		}
	}
	for (y = 0; y < TSIZE; y++) {
		tp1 = TPT(TX2-TX1, y);
		tp1->objects = NULL;
	}
}

static void move_objects_up(void)
{
	int x, y;
	Terrain_point *tp1, *tp2;
	sList *l;
	sObject *o;
	World *w;

	tp1 = TPT(0, TY2-TY1);
	for (x = 0; x < TSIZE; x++, tp1++) {
		if (tp1->objects) {
			for (l = tp1->objects->next; l != tp1->objects; l = l->next) {
				o = l->item;
				w = o->data;
				if (w == NULL)
					stop("Instance data NULL in update_tp");
				w->tp = NULL;
			}
			list_destroy_all(tp1->objects);
			tp1->objects = NULL;
		}
	}
	for (y = TY2; y > TY1; y--) {
		for (x = TX1; x <= TX2; x++) {
			tp1 = TPT(x-TX1,y-TY1);
			tp2 = TPT(x-TX1,y-TY1-1);
			tp1->objects = tp2->objects;
			if (tp1->objects) {
				for (l = tp1->objects->next; l != tp1->objects; l = l->next) {
					o = l->item;
					w = o->data;
					w->tp = tp1;
				}
			}
		}
	}
	tp1 = TPT(0, 0);
	for (x = 0; x < TSIZE; x++, tp1++) {
		tp1->objects = NULL;
	}
}

static void move_objects_down(void)
{
	int x, y;
	Terrain_point *tp1, *tp2;
	sList *l;
	sObject *o;
	World *w;

	tp1 = TPT(0, 0);
	for (x = 0; x < TSIZE; x++, tp1++) {
		if (tp1->objects) {
			for (l = tp1->objects->next; l != tp1->objects; l = l->next) {
				o = l->item;
				w = o->data;
				if (w == NULL)
					stop("Instance data NULL in update_tp");
				w->tp = NULL;
			}
			list_destroy_all(tp1->objects);
			tp1->objects = NULL;
		}
	}
	for (y = TY1; y < TY2; y++) {
		for (x = TX1; x <= TX2; x++) {
			tp1 = TPT(x-TX1,y-TY1);
			tp2 = TPT(x-TX1,y-TY1+1);
			tp1->objects = tp2->objects;
			if (tp1->objects) {
				for (l = tp1->objects->next; l != tp1->objects; l = l->next) {
					o = l->item;
					w = o->data;
					w->tp = tp1;
				}
			}
		}
	}
	tp1 = TPT(0, TY2-TY1);
	for (x = 0; x < TSIZE; x++, tp1++) {
		tp1->objects = NULL;
	}
}

static void update_tp(void)
{
	int dx, dy, x, y;
	Terrain_point *tp;

	check_terrain_data();
	dx = real_terrain_x - last_terrain_x;
	dy = real_terrain_y - last_terrain_y;
	if ((ABS(dx) > 1) || (ABS(dy) > 1)) {
		full_update_tp();
	} else {
		partial_update_tp1();
		if (dx == 1) {
			move_objects_left();
		} else if (dx == -1) {
			move_objects_right();
		}
		if (dy == 1) {
			move_objects_down();
		} else if (dy == -1) {
			move_objects_up();
		}
		partial_update_tp2();
	}
	/* Force object drawing on central squares */
	for (y = (TY2 - 1); y <= (TY2 + 3); y++) {
		for (x = (TX2 - 1); x <= (TX2 + 3); x++) {
			tp = TPT(x, y);
			tp->flags |= DRAW_OBJECTS;
		}
	}
}


/* -------------------
 * GLOBAL add_to_tp
 * Add object to terrain square 
 * 
 */
int add_to_tp(sObject *o, int force_draw)
{
	Terrain_point *tp;
	int x, y;
	World *w;

	w = o->data;
	x = TX(w->p.x);
	y = TY(w->p.z);
	if ((x >= TX1) && (x <= TX2) && (y >= TY1) && (y <= TY2)) {
		tp = TPT(x-TX1, y-TY1);
		if (force_draw)
			tp->flags |= DRAW_OBJECTS;
		if (tp->objects == NULL)
			tp->objects = new_list();
		list_add(o, tp->objects);
		w->tp = tp;
		return TRUE;
	}
	return FALSE;
}

static void list_add_sorted(sObject *o, sList *objects);


/* -------------------
 * GLOBAL add_to_tp_sorted
 * 
 * Add object to terrain point list sorted by distance from viewer
 */
int add_to_tp_sorted(sObject *o, int force_draw)
{
	Terrain_point *tp;
	int x, y;
	World *w;

	w = o->data;
	x = TX(w->p.x);
	y = TY(w->p.z);
	if ((x >= TX1) && (x <= TX2) && (y >= TY1) && (y <= TY2)) {
		tp = TPT(x-TX1, y-TY1);
		if (force_draw)
			tp->flags |= DRAW_OBJECTS;
		if (tp->objects == NULL)
			tp->objects = new_list();
		list_add_sorted(o, tp->objects);
		w->tp = tp;
		return TRUE;
	}
	return FALSE;
}


/* -------------------
 * GLOBAL move_in_tp
 * 
 */
int move_in_tp(sObject *o, int force_draw)
{
	Terrain_point *tp;
	int x, y;
	World *w;

	w = o->data;
	x = TX(w->p.x);
	y = TY(w->p.z);
	if ((x >= TX1) && (x <= TX2) && (y >= TY1) && (y <= TY2)) {
		tp = TPT(x-TX1, y-TY1);
		if (force_draw)
			tp->flags |= DRAW_OBJECTS;
		if ((w->tp == NULL) || (tp != w->tp)) {
			remove_from_tp(o);
			if (tp->objects == NULL)
				tp->objects = new_list();
			list_add(o, tp->objects);
			w->tp = tp;
		}
		return TRUE;
	}
	remove_from_tp(o);
	return FALSE;
}


/* -------------------
 * GLOBAL remove_from_tp
 * 
 * Remove object from terrain point database
 */
void remove_from_tp(sObject *o)
{
    World *w;
	Terrain_point *tp;

    w = o->data;
    if (w->tp != NULL) {
		tp = w->tp;
		if (tp && tp->objects)
			list_remove(o, tp->objects);
		w->tp = NULL;
    }
}


/* -------------------
 * GLOBAL get_tp_objects
 * 
 * Returns list of objects in same terrain point as object supplied
 */
sList* get_tp_objects(sObject *o)
{
    World *w;
	Terrain_point *tp;

    w = o->data;
	if (w) {
		tp = w->tp;
		if (tp)
			return tp->objects;
	}
	return NULL;
}

static Terrain_point *transform_point(int x, int y)
{
	int32 zf;
	Terrain_point *tp;
	sVector v;

	tp = TPT(x-TX1, y-TY1);
	if (!(tp->flags & TRANSFORMED)) {
		tp->flags |= TRANSFORMED;
		v.x = x * tgrid_gap;
		v.z = y * tgrid_gap;
		v.y = tp->height << (3 - TSHIFT);
		tp->tpt.x = ((v.x*matrix->a + v.y*matrix->d + v.z*matrix->g) + matrix->j);
    	tp->tpt.y = ((v.x*matrix->b + v.y*matrix->e + v.z*matrix->h) + matrix->k);
    	tp->tpt.z = ((v.x*matrix->c + v.y*matrix->f + v.z*matrix->i) + matrix->l);
        zf = tp->tpt.z >> pshift;
        if (zf > 0) {
           	tp->spt.x = xcentre + DIV(tp->tpt.x, zf);
           	tp->spt.y = ycentre - DIV(tp->tpt.y, zf);
        } else {
            tp->spt.x = xcentre + tp->tpt.x;
            tp->spt.y = ycentre - tp->tpt.y;
        }
	}
	return tp;
}

static int in_view(int x1, int y1, int size)
{
	int32 dz, radius;
	Terrain_point *tp;

	radius = sgrid_gap * size;
/*	radius += radius >> 1;*/
	size >>= 1;
	tp = transform_point(x1+size, y1+size);
	dz = tp->tpt.z;
    if (((dz + radius) > 0) &&
        ((ABS(tp->tpt.x) - radius) < dz) &&
        ((ABS(tp->tpt.y) - radius) < dz)) {
		return TRUE;
	}
	return FALSE;
}

static void scan_quad(int x1, int y1, int size)
{
	Terrain_point *tp;

	if (size <= 1) {
		tp = transform_point(x1, y1);
		tp->flags |= INVIEW;
		transform_point(x1+1, y1);
		transform_point(x1, y1+1);
		transform_point(x1+1, y1+1);
	} else {
		if (in_view(x1, y1, size)) {
			size >>= 1;
			scan_quad(x1 + size, y1 + size, size);
			scan_quad(x1, y1 + size, size);
			scan_quad(x1 + size, y1, size);
			scan_quad(x1, y1, size);
		}
	}
}

static void draw_quad_y(int y1, int y2, int dy)
{
	int x, y, c1, c2, cy1, cy2;
	Terrain_point *tp;

	if (looking_down) {
		c1 = (TSIZE / 2) - 2;
		c2 = (TSIZE / 2) + 2;
		if (dy > 0) {
			cy1 = c1;
			cy2 = c2 + 1;
		} else {
			cy1 = c2;
			cy2 = c1 - 1;
		}
		/* Top region */
		for (y = 0; y < c1; y++) {
			tp = TPT(0, y);
			for (x = 0; x < (TSIZE / 2); x++, tp++) {
				if (tp->flags & INVIEW) {
					draw_square(x + TX1, y + TY1, 1);
					if (tp->objects)
						draw_terrain_objects(tp->objects);
				} else if (tp->flags & DRAW_OBJECTS) {
					if (tp->objects)
						draw_terrain_objects(tp->objects);
				}
			}
			tp = TPT(TSIZE - 1, y);
			for (x = (TSIZE - 1); x >= (TSIZE / 2); x--, tp--) {
				if (tp->flags & INVIEW) {
					draw_square(x + TX1, y + TY1, 1);
					if (tp->objects)
						draw_terrain_objects(tp->objects);
				} else if (tp->flags & DRAW_OBJECTS) {
					if (tp->objects)
						draw_terrain_objects(tp->objects);
				}
			}
		}
		/* Left and right regions */
		for (y = cy1; y != cy2; y += dy) {
			for (x = 0; x < c1; x++) {
				tp = TPT(x, y);
				if (tp->flags & INVIEW) {
					draw_square(x + TX1, y + TY1, 1);
					if (tp->objects)
						draw_terrain_objects(tp->objects);
				} else if (tp->flags & DRAW_OBJECTS) {
					if (tp->objects)
						draw_terrain_objects(tp->objects);
				}
			}
			for (x = (TSIZE - 1); x > c2; x--) {
				tp = TPT(x, y);
				if (tp->flags & INVIEW) {
					draw_square(x + TX1, y + TY1, 1);
					if (tp->objects)
						draw_terrain_objects(tp->objects);
				} else if (tp->flags & DRAW_OBJECTS) {
					if (tp->objects)
						draw_terrain_objects(tp->objects);
				}
			}
		}
		/* Bottom region */
		for (y = (TSIZE - 1); y > c2; y--) {
			tp = TPT(0, y);
			for (x = 0; x < (TSIZE / 2); x++, tp++) {
				if (tp->flags & INVIEW) {
					draw_square(x + TX1, y + TY1, 1);
					if (tp->objects)
						draw_terrain_objects(tp->objects);
				} else if (tp->flags & DRAW_OBJECTS) {
					if (tp->objects)
						draw_terrain_objects(tp->objects);
				}
			}
			tp = TPT(TSIZE - 1, y);
			for (x = (TSIZE - 1); x >= (TSIZE / 2); x--, tp--) {
				if (tp->flags & INVIEW) {
					draw_square(x + TX1, y + TY1, 1);
					if (tp->objects)
						draw_terrain_objects(tp->objects);
				} else if (tp->flags & DRAW_OBJECTS) {
					if (tp->objects)
						draw_terrain_objects(tp->objects);
				}
			}
		}
		/* Centre region */
		for (y = cy1; y != cy2; y += dy) {
			for (x = c1; x < (TSIZE / 2); x++) {
				tp = TPT(x, y);
				if (tp->flags & INVIEW) {
					draw_square(x + TX1, y + TY1, 1);
				}
			}
			for (x = c2; x >= (TSIZE / 2); x--) {
				tp = TPT(x, y);
				if (tp->flags & INVIEW) {
					draw_square(x + TX1, y + TY1, 1);
				}
			}
		}
		for (y = cy1; y != cy2; y += dy) {
			for (x = c1; x <= c2; x++) {
				tp = TPT(x, y);
				if (tp->objects)
					draw_terrain_objects(tp->objects);
			}
		}
	} else {
		for (y = y1; y != y2; y += dy) {
			tp = TPT(0, y);
			for (x = 0; x < (TSIZE / 2); x++, tp++) {
				if (tp->flags & INVIEW) {
					draw_square(x + TX1, y + TY1, 1);
					if (tp->objects)
						draw_terrain_objects(tp->objects);
				} else if (tp->flags & DRAW_OBJECTS) {
					if (tp->objects)
						draw_terrain_objects(tp->objects);
				}
			}
			tp = TPT(TSIZE - 1, y);
			for (x = (TSIZE - 1); x >= (TSIZE / 2); x--, tp--) {
				if (tp->flags & INVIEW) {
					draw_square(x + TX1, y + TY1, 1);
					if (tp->objects)
						draw_terrain_objects(tp->objects);
				} else if (tp->flags & DRAW_OBJECTS) {
					if (tp->objects)
						draw_terrain_objects(tp->objects);
				}
			}
		}
	}
}

static void draw_quad_x(int x1, int x2, int dx)
{
	int x, y, c1, c2, cx1, cx2;
	Terrain_point *tp;

	if (looking_down) {
		c1 = (TSIZE / 2) - 2;
		c2 = (TSIZE / 2) + 2;
		if (dx > 0) {
			cx1 = c1;
			cx2 = c2 + 1;
		} else {
			cx1 = c2;
			cx2 = c1 - 1;
		}
		/* Right region */
		for (x = 0; x < c1; x++) {
			for (y = 0; y < (TSIZE / 2); y++) {
				tp = TPT(x, y);
				if (tp->flags & INVIEW) {
					draw_square(x + TX1, y + TY1, 1);
					if (tp->objects)
						draw_terrain_objects(tp->objects);
				} else if (tp->flags & DRAW_OBJECTS) {
					if (tp->objects)
						draw_terrain_objects(tp->objects);
				}
			}
			for (y = (TSIZE - 1); y >= (TSIZE / 2); y--) {
				tp = TPT(x, y);
				if (tp->flags & INVIEW) {
					draw_square(x + TX1, y + TY1, 1);
					if (tp->objects)
						draw_terrain_objects(tp->objects);
				} else if (tp->flags & DRAW_OBJECTS) {
					if (tp->objects)
						draw_terrain_objects(tp->objects);
				}
			}
		}
		/* Top and bottom regions */
		for (x = cx1; x != cx2; x += dx) {
			for (y = 0; y < c1; y++) {
				tp = TPT(x, y);
				if (tp->flags & INVIEW) {
					draw_square(x + TX1, y + TY1, 1);
					if (tp->objects)
						draw_terrain_objects(tp->objects);
				} else if (tp->flags & DRAW_OBJECTS) {
					if (tp->objects)
						draw_terrain_objects(tp->objects);
				}
			}
			for (y = (TSIZE - 1); y > c2; y--) {
				tp = TPT(x, y);
				if (tp->flags & INVIEW) {
					draw_square(x + TX1, y + TY1, 1);
					if (tp->objects)
						draw_terrain_objects(tp->objects);
				} else if (tp->flags & DRAW_OBJECTS) {
					if (tp->objects)
						draw_terrain_objects(tp->objects);
				}
			}
		}
		/* Left region */
		for (x = (TSIZE - 1); x > c2; x--) {
			for (y = 0; y < (TSIZE / 2); y++) {
				tp = TPT(x, y);
				if (tp->flags & INVIEW) {
					draw_square(x + TX1, y + TY1, 1);
					if (tp->objects)
						draw_terrain_objects(tp->objects);
				} else if (tp->flags & DRAW_OBJECTS) {
					if (tp->objects)
						draw_terrain_objects(tp->objects);
				}
			}
			for (y = (TSIZE - 1); y >= (TSIZE / 2); y--) {
				tp = TPT(x, y);
				if (tp->flags & INVIEW) {
					draw_square(x + TX1, y + TY1, 1);
					if (tp->objects)
						draw_terrain_objects(tp->objects);
				} else if (tp->flags & DRAW_OBJECTS) {
					if (tp->objects)
						draw_terrain_objects(tp->objects);
				}
			}
		}
		/* Centre region */
		for (x = cx1; x != cx2; x += dx) {
			for (y = c1; y < (TSIZE / 2); y++) {
				tp = TPT(x, y);
				if (tp->flags & INVIEW) {
					draw_square(x + TX1, y + TY1, 1);
				}
			}
			for (y = c2; y >= (TSIZE / 2); y--) {
				tp = TPT(x, y);
				if (tp->flags & INVIEW) {
					draw_square(x + TX1, y + TY1, 1);
				}
			}
		}
		for (x = cx1; x != cx2; x += dx) {
			for (y = c1; y <= c2; y++) {
				tp = TPT(x, y);
				if (tp->objects)
					draw_terrain_objects(tp->objects);
			}
		}
	} else {
		for (x = x1; x != x2; x += dx) {
			for (y = 0; y < (TSIZE / 2); y++) {
				tp = TPT(x, y);
				if (tp->flags & INVIEW) {
					draw_square(x + TX1, y + TY1, 1);
					if (tp->objects)
						draw_terrain_objects(tp->objects);
				} else if (tp->flags & DRAW_OBJECTS) {
					if (tp->objects)
						draw_terrain_objects(tp->objects);
				}
			}
			for (y = (TSIZE - 1); y >= (TSIZE / 2); y--) {
				tp = TPT(x, y);
				if (tp->flags & INVIEW) {
					draw_square(x + TX1, y + TY1, 1);
					if (tp->objects)
						draw_terrain_objects(tp->objects);
				} else if (tp->flags & DRAW_OBJECTS) {
					if (tp->objects)
						draw_terrain_objects(tp->objects);
				}
			}
		}
	}
}

static void draw_quad_north_east(int x1, int y1, int size)
{
	scan_quad(x1, y1, size);
	draw_quad_y(TSIZE - 1, 0, -1);
}

static void draw_quad_north_west(int x1, int y1, int size)
{
	scan_quad(x1, y1, size);
	draw_quad_y(TSIZE - 1, 0, -1);
}

static void draw_quad_south_east(int x1, int y1, int size)
{
	scan_quad(x1, y1, size);
	draw_quad_y(0, TSIZE - 1, 1);
}

static void draw_quad_south_west(int x1, int y1, int size)
{
	scan_quad(x1, y1, size);
	draw_quad_y(0, TSIZE - 1, 1);
}

static void draw_quad_east_north(int x1, int y1, int size)
{
	scan_quad(x1, y1, size);
	draw_quad_x(TSIZE - 1, 0, -1);
}

static void draw_quad_east_south(int x1, int y1, int size)
{
	scan_quad(x1, y1, size);
	draw_quad_x(TSIZE - 1, 0, -1);
}

static void draw_quad_west_north(int x1, int y1, int size)
{
	scan_quad(x1, y1, size);
	draw_quad_x(0, TSIZE - 1, 1);
}

static void draw_quad_west_south(int x1, int y1, int size)
{
	scan_quad(x1, y1, size);
	draw_quad_x(0, TSIZE - 1, 1);
}

/*static int clockwise2(sVector2 *s) {return TRUE;}*/
extern int clockwise2(sVector2 *s);


/* --------------------
 * LOCAL draw_square
 * 
 * Draw terrain square, if step is true then
 * use checker board algorithm
 * 
 */ 
static void draw_square(int x1, int y1, int step)
{
	int col, i, j, x, y, clip_flag, zero_count, dist_shade;
	int h[4];
	Terrain_point *tp;
    sVector2 spts[4], bpts[4];

/*	if (!draw_ground_on)
		return;
 */
	x = x1 - TX1;
	y = y1 - TY1;
	tp = TPT(x, y);
	if (tp->tpt.z > sgrid_radius) {
		col = tp->colour;
		zero_count = clip_flag = 0;
		if ((h[0] = tp->height) == 0)
			zero_count++;
        if (tp->spt.x < clip_region.left) clip_flag |= 1;
		else if (tp->spt.x > clip_region.right) clip_flag |= 2;
		if (tp->spt.y < clip_region.top) clip_flag |= 4;
		else if (tp->spt.y > clip_region.bottom) clip_flag |= 8;
		spts[0] = tp->spt;
		tp = TPT(x, y+step);
		if ((h[1] = tp->height) == 0)
			zero_count++;
        if (tp->spt.x < clip_region.left) clip_flag |= 1;
		else if (tp->spt.x > clip_region.right) clip_flag |= 2;
		if (tp->spt.y < clip_region.top) clip_flag |= 4;
		else if (tp->spt.y > clip_region.bottom) clip_flag |= 8;
		spts[1] = tp->spt;
		tp = TPT(x+step, y+step);
		if ((h[2] = tp->height) == 0)
			zero_count++;
        if (tp->spt.x < clip_region.left) clip_flag |= 1;
		else if (tp->spt.x > clip_region.right) clip_flag |= 2;
		if (tp->spt.y < clip_region.top) clip_flag |= 4;
		else if (tp->spt.y > clip_region.bottom) clip_flag |= 8;
		spts[2] = tp->spt;
		tp = TPT(x+step, y);
		if ((h[0] == h[1]) && (h[0] == h[2]) && (h[0] == tp->height)) {
        	if (tp->spt.x < clip_region.left) clip_flag |= 1;
			else if (tp->spt.x > clip_region.right) clip_flag |= 2;
			if (tp->spt.y < clip_region.top) clip_flag |= 4;
			else if (tp->spt.y > clip_region.bottom) clip_flag |= 8;
			spts[3] = tp->spt;
			col = get_square_colour(x1, y1, col, TRUE, &dist_shade);
			if (h[0] == 0) {
				if (draw_sea_on) {
					col = SEA_COL + dist_shade;
					if (clip_flag == 0) {
						poly_prim(4, spts, COL(col));
					} else {
						polygon(4, spts, col);
					}
				}
			} else {
				if (clip_flag == 0) {
					poly_prim(4, spts, COL(col));
				} else {
					polygon(4, spts, col);
				}
			}
		} else {
			if (tp->height == 0)
				zero_count++;
			if (zero_count == 3) {
        		if (tp->spt.x < clip_region.left) clip_flag |= 1;
				else if (tp->spt.x > clip_region.right) clip_flag |= 2;
				if (tp->spt.y < clip_region.top) clip_flag |= 4;
				else if (tp->spt.y > clip_region.bottom) clip_flag |= 8;
				spts[3] = tp->spt;
				h[3] = tp->height;
				for (i = 0; (h[i] == 0) && (i < 4); i++)
					;
				col = get_square_colour(x1, y1, col, TRUE, &dist_shade);
				if (--i < 0)
					i = 3;
				for (j = 0; j < 3; j++) {
					bpts[j] = spts[i];
					if (++i > 3)
						i = 0;
				}
				if (clip_flag == 0) {
            		if (clockwise2(bpts))
						poly_prim(3, bpts, COL(col));
				} else {
            		if (clockwise2(bpts))
						polygon(3, bpts, col);
				}
				if (draw_sea_on) {
					if (--i < 0)
						i = 3;
					for (j = 0; j < 3; j++) {
						bpts[j] = spts[i];
						if (++i > 3)
							i = 0;
					}
					col = SEA_COL + dist_shade;
					if (clip_flag == 0) {
						poly_prim(3, bpts, COL(col));
					} else {
						polygon(3, bpts, col);
					}
				}
			} else {
				col = get_square_colour(x1, y1, col, FALSE, &dist_shade);
				if (clip_flag == 0) {
            		if (clockwise2(spts))
						poly_prim(3, spts, COL(col));
				} else {
            		if (clockwise2(spts))
						polygon(3, spts, col);
				}
				spts[1] = spts[2];
        		if (tp->spt.x < clip_region.left) clip_flag |= 1;
				else if (tp->spt.x > clip_region.right) clip_flag |= 2;
				if (tp->spt.y < clip_region.top) clip_flag |= 4;
				else if (tp->spt.y > clip_region.bottom) clip_flag |= 8;
				spts[2] = tp->spt;
				if (col < (MAX_GROUND_COL - COL_TRI_STEP))
					col += COL_TRI_STEP;
				if (clip_flag == 0) {
            		if (clockwise2(spts))
						poly_prim(3, spts, COL(col));
				} else {
            		if (clockwise2(spts))
						polygon(3, spts, col);
				}
			}
		}
	} else {
		clipped_squares++;
		draw_clipped_square(x1, y1, step);
	}
	if (ground_grid_on) {
		if ((ABS(x1) < 8) && (ABS(y1) < 8)) {
			draw_square_grid(x1, y1, step);
		}
	}
}

#define NEAR_CLIP (8L << (SSHIFT + zoom_shift))
static int z_clip(sVector *p1, sVector *p2)
{
    register int dz, z1;

    if (p2->z >= p1->z) {
        if (p1->z >= NEAR_CLIP)
            return 1;
        if (p2->z < NEAR_CLIP)
            return 0;
        if (p2->z == NEAR_CLIP) {
            p1->x = p2->x;
            p1->y = p2->y;
        	p1->z = NEAR_CLIP;
        	return 2;
        }
		dz = (int)((p2->z - p1->z) >> SSHIFT);
        z1 = (int)((NEAR_CLIP - p1->z) >> SSHIFT);
        if (dz == 0) {
            p1->x += MUL(((p2->x - p1->x) >> SSHIFT), z1) << SSHIFT;
            p1->y += MUL(((p2->y - p1->y) >> SSHIFT), z1) << SSHIFT;
        }
        else {
            p1->x += (int32)MULDIV(((p2->x - p1->x) >> SSHIFT), z1, dz) << SSHIFT;
            p1->y += (int32)MULDIV(((p2->y - p1->y) >> SSHIFT), z1, dz) << SSHIFT;
        }
    	p1->z = NEAR_CLIP;
    	return 2;
    }
	return z_clip(p2, p1);
}

static void draw_clipped_poly(int npts, Terrain_point **tps, int col, int clockwise_test)
{
	int i, n, c;
	int32 zf;
	Terrain_point *tp;
    sVector p1, p2;
    sVector2 spts[8];

	tp = tps[npts - 1];
	for (i = 0, n = 0; i < npts; i++) {
    	p1 = tp->tpt;
		tp = tps[i];
    	p2 = tp->tpt;
    	c = z_clip(&p1, &p2);
    	if (c > 0) {
			zf = p1.z >> pshift;
			spts[n].x = xcentre + (int)(p1.x / zf);
			spts[n++].y = ycentre - (int)(p1.y / zf);
			if (c > 1) {
	       		zf = p2.z >> pshift;
	       		spts[n].x = xcentre + (int)(p2.x / zf);
	       		spts[n++].y = ycentre - (int)(p2.y / zf);
			}
		}
	}
	if (n > 2) {
		if (clockwise_test) {
        	if (clockwise2(spts))
				polygon(n, spts, col);
		} else {
			polygon(n, spts, col);
		}
	}
}

static void draw_clipped_square(int x1, int y1, int step)
{
	int col, i, j, x, y, zero_count, dist_shade;
	int h[4];
	Terrain_point *tp[4], *btp[4];

	x = x1 - TX1;
	y = y1 - TY1;
	zero_count = 0;
	tp[0] = TPT(x, y);
	if ((h[0] = tp[0]->height) == 0)
		zero_count++;
	tp[1] = TPT(x, y+step);
	if ((h[1] = tp[1]->height) == 0)
		zero_count++;
	tp[2] = TPT(x+step, y+step);
	if ((h[2] = tp[2]->height) == 0)
		zero_count++;
	tp[3] = TPT(x+step, y);
	if ((h[0] == h[1]) && (h[0] == h[2]) && (h[0] == tp[3]->height)) {
		col = get_square_colour(x1, y1, tp[0]->colour, TRUE, &dist_shade);
		if (h[0] == 0) {
			if (draw_sea_on) {
				col = SEA_COL + dist_shade;
				draw_clipped_poly(4, tp, col, FALSE);
			}
		} else {
			draw_clipped_poly(4, tp, col, FALSE);
		}
	} else {
		if (tp[3]->height == 0)
			zero_count++;
		if (zero_count == 3) {
			h[3] = tp[3]->height;
			for (i = 0; (h[i] == 0) && (i < 4); i++)
				;
			col = get_square_colour(x1, y1, tp[0]->colour, TRUE, &dist_shade);
			if (--i < 0)
				i = 3;
			for (j = 0; j < 3; j++) {
				btp[j] = tp[i];
				if (++i > 3)
					i = 0;
			}
			draw_clipped_poly(3, btp, col, TRUE);
			if (draw_sea_on) {
				if (--i < 0)
					i = 3;
				for (j = 0; j < 3; j++) {
					btp[j] = tp[i];
					if (++i > 3)
						i = 0;
				}
				col = SEA_COL + dist_shade;
				draw_clipped_poly(3, btp, col, FALSE);
			}
		} else {
			col = get_square_colour(x1, y1, tp[0]->colour, FALSE, &dist_shade);
			draw_clipped_poly(3, tp, col, TRUE);
			tp[1] = tp[2];
			tp[2] = tp[3];
			if (col < (MAX_GROUND_COL - COL_TRI_STEP))
				col += COL_TRI_STEP;
			draw_clipped_poly(3, tp, col, TRUE);
		}
	}
}

static void draw_grid_line(sVector p1, sVector p2, int col)
{
	int x1, y1, x2, y2, c;
	int32 zf;

	c = z_clip(&p1, &p2);
	if (c > 0) {
		zf = p1.z >> pshift;
		x1 = xcentre + (int)(p1.x / zf);
		y1 = ycentre - (int)(p1.y / zf);
		zf = p2.z >> pshift;
		x2 = xcentre + (int)(p2.x / zf);
		y2 = ycentre - (int)(p2.y / zf);
		line(x1, y1, x2, y2, col);
	}
}

static void draw_grid_lines_x(sVector *p1, sVector *p2, sVector *p3, sVector *p4, int n)
{
	sVector c1, c2;

	c1.x = (p1->x + p2->x) >> 1;
	c1.y = (p1->y + p2->y) >> 1;
	c1.z = (p1->z + p2->z) >> 1;
	c2.x = (p3->x + p4->x) >> 1;
	c2.y = (p3->y + p4->y) >> 1;
	c2.z = (p3->z + p4->z) >> 1;
	draw_grid_line(c1, c2, GREEN);
	if (n > 1) {
		draw_grid_lines_x(p1, &c1, &c2, p4, n - 1);
		draw_grid_lines_x(&c1, p2, p3, &c2, n - 1);
	}
}

static void draw_grid_lines_z(sVector *p1, sVector *p2, sVector *p3, sVector *p4, int n)
{
	sVector c1, c2;

	c1.x = (p2->x + p3->x) >> 1;
	c1.y = (p2->y + p3->y) >> 1;
	c1.z = (p2->z + p3->z) >> 1;
	c2.x = (p4->x + p1->x) >> 1;
	c2.y = (p4->y + p1->y) >> 1;
	c2.z = (p4->z + p1->z) >> 1;
	draw_grid_line(c1, c2, GREEN);
	if (n > 1) {
		draw_grid_lines_z(p1, p2, &c1, &c2, n - 1);
		draw_grid_lines_z(&c2, &c1, p3, p4, n - 1);
	}
}

static void draw_square_grid(int x1, int y1, int step)
{
	int x, y;
	Terrain_point *tp[4];
	sVector p1, p2;

	x = x1 - TX1;
	y = y1 - TY1;
	tp[0] = TPT(x, y);
	tp[1] = TPT(x, y+step);
	tp[2] = TPT(x+step, y+step);
	tp[3] = TPT(x+step, y);
	draw_grid_line(tp[0]->tpt, tp[1]->tpt, GREEN);
	draw_grid_line(tp[1]->tpt, tp[2]->tpt, GREEN);
	draw_grid_line(tp[2]->tpt, tp[3]->tpt, GREEN);
	draw_grid_line(tp[3]->tpt, tp[0]->tpt, GREEN);
	if (ground_grid_size > 1) {
		draw_grid_lines_x(&tp[0]->tpt, &tp[1]->tpt, &tp[2]->tpt, &tp[3]->tpt, ground_grid_size - 1);
		draw_grid_lines_z(&tp[0]->tpt, &tp[1]->tpt, &tp[2]->tpt, &tp[3]->tpt, ground_grid_size - 1);
	}
}

static int get_square_colour(int tx, int ty, int base_col, int checker, int *dist_shade)
{
	int x, y, dist;

	if (tx >= 0)
		x = tx;
	else
		x = -tx;
	if (ty >= 0)
		y = ty;
	else
		y = -ty;

	if (x >= y)
		dist = (x + y - (y >> 1));
	else
		dist = (x + y - (x >> 1));
	if (distance_factor > 0)
		dist = (dist >> distance_factor) - 2;
	else
		dist = (dist << -distance_factor) - 2;

	if (dist < 0)
		dist = 0;
	else if (dist > 7)
		dist = 7;

	*dist_shade = dist;
	if (base_col == SEA_COL) {
		return (base_col + dist);
	}
	if (base_col > 15)
		return (base_col - 16);
	if (checker) {
		tx += terrain_x;
		ty += terrain_y;
		if (ty & 1) {
			if (tx & 1)
				base_col += 1;
		} else {
			if (!(tx & 1))
				base_col += 1;
		}
	}
	if (!shading_on)
		return (0x30 + (base_col << 2));
	if (dist < 4)
		return (0x30 + (base_col << 2) + dist);
	return ((0x70 - 4) + ((base_col & 0xe) << 1) + dist);
}

static void zero_terrain_heights(short *data)
{
	int size;
	int *p;

	size = terrain_width * terrain_height;
	if (sizeof(int) == 4) {
		p = (int *)data;
		size >>= 1;
		while (size--)
			*p++ = 0;
	} else {
		while (size--)
			*data++ = 0;
	}
}

static int load_terrain_file(char *file_name, short *heights)
{
	FILE *fp;
	int width, size, gap;
	char header[HEADER_SIZE+2], *p;

	size = terrain_height * terrain_width;
	fp = fopen(file_name, "rb");
	if (fp != NULL) {
		fread(header, 1, HEADER_SIZE, fp);
		header[HEADER_SIZE] = '\0';
		p = strstr(header, "SIZE=");
		width = atoi(p + 5);
		if ((width != terrain_width) || (width != terrain_height))
			stop("Wrong terrain file size");
		p = strstr(header, "GAP=");
		if (p != NULL) {
			gap = atoi(p + 4);
			set_grid_gap(gap);
		}
		fread(heights, sizeof(short), size, fp);
		fclose(fp);
		return TRUE;
	}
	zero_terrain_heights(heights);
	return FALSE;
}

static void process_terrain(short *heights)
{
	int x, y, index, height;

	terrain_min_height = 30000;
	terrain_max_height = 0;
	for (y = 0; y < terrain_height; y++) {
		index = y * terrain_width;
		for (x = 0; x < terrain_width; x++, index++) {
			height = heights[index];
			if (height < terrain_min_height)
				terrain_min_height = height;
			if (height > terrain_max_height)
				terrain_max_height = height;
		}
	}
}

static int get_terrain_colour(int height, int min_height, int height_range)
{
	int col;

	col = ((height - terrain_min_height) * 16) / height_range;
	if (col > 15)
		col = 15;
	else if ((col == 0) && (height != 0))
		col = 1;
	return col;
}

static void setup_terrain_colours(short *heights, uchar *colours)
{
	int x, y, index, height, height_range;

	height_range = terrain_max_height - terrain_min_height;
	if (height_range < 15)
		height_range = 15;
	for (y = 0; y < (terrain_height - 1); y++) {
		index = y * terrain_width;
		for (x = 0; x < (terrain_width - 1); x++, index++) {
			height = heights[index];
			height += heights[index + 1];
			height += heights[index + terrain_width];
			height += heights[index + terrain_width + 1];
			height >>= 2;
			colours[index] = get_terrain_colour(height, terrain_min_height, height_range);
		}
		/* Last point in line */
		height = heights[index];
		colours[index] = get_terrain_colour(height, terrain_min_height, height_range);
		/* Last line */
		index = (terrain_height - 1) * terrain_width;
		for (x = 0; x < terrain_width; x++, index++) {
			height = heights[index];
			colours[index] = get_terrain_colour(height, terrain_min_height, height_range);
		}
	}
}

static void swap_terrain_blocks(Terrain_block *tb1, Terrain_block *tb2)
{
	Terrain_block tb;

	tb = *tb1;
	*tb1 = *tb2;
	*tb2 = tb;
}

static void get_file_name(char *name, int bx, int by)
{
	char xs[8], ys[8];

	if (bx >= 0)
		sprintf(xs, "0%02d", bx);
	else
		sprintf(xs, "_%02d", -bx);
	if (by >= 0)
		sprintf(ys, "0%02d", by);
	else
		sprintf(ys, "_%02d", -by);
	sprintf(name, "T%s%s.FTD", xs, ys);
}

static void load_terrain_block(int bx, int by, Terrain_block *tb)
{
	int i, j, old_max_height, old_min_height;
	Terrain_block *tb1;
	char file_name[256];

	get_file_name(file_name, bx, by);
	load_terrain_file(file_name, tb->heights);
	old_min_height = terrain_min_height;
	old_max_height = terrain_max_height;
	process_terrain(tb->heights);
	setup_terrain_colours(tb->heights, tb->colours);
	if ((terrain_min_height < old_min_height) || (terrain_max_height > old_max_height)) {
		for (i = 0; i < 2; i++) {
			for (j = 0; j < 2; j++) {
				tb1 = &terrain_data[i][j];
				if (tb1 != tb) {
					setup_terrain_colours(tb->heights, tb->colours);
				}
			}
		}
	}
}

static void check_terrain_data(void)
{
	int vx, vz, bx, by;

	if (view_x >= 0)
		terrain_centre_x = (view_x + (block_width >> 1)) / block_width;
	else
		terrain_centre_x = (view_x - (block_width >> 1)) / block_width;
	terrain_centre_x *= block_width;
	if (view_z >= 0)
		terrain_centre_z = (view_z + (block_height >> 1)) / block_height;
	else
		terrain_centre_z = (view_z - (block_height >> 1)) / block_height;
	terrain_centre_z *= block_height;

	vx = view_x - (block_width >> 1);
	bx = vx / block_width;
	if (vx < 0)
		bx--;
	if (view_x >= 0) {
		if ((view_x % block_width) < (block_width >> 1))
			block_i = 1;
		else
			block_i = 0;
	} else {
		if ((-view_x % block_width) < (block_width >> 1))
			block_i = 0;
		else
			block_i = 1;
	}
	vz = view_z - (block_height >> 1);
	by = vz / block_height;
	if (vz < 0)
		by--;
	if (view_z >= 0) {
		if ((view_z % block_height) < (block_height >> 1))
			block_j = 1;
		else
			block_j = 0;
	} else {
		if ((-view_z % block_height) < (block_height >> 1))
			block_j = 0;
		else
			block_j = 1;
	}
	if ((bx != block_x) || (by != block_y)) {
		if (bx != block_x) {
			swap_terrain_blocks(&terrain_data[0][0], &terrain_data[1][0]);
			swap_terrain_blocks(&terrain_data[0][1], &terrain_data[1][1]);
		}
		if (by != block_y) {
			swap_terrain_blocks(&terrain_data[0][0], &terrain_data[0][1]);
			swap_terrain_blocks(&terrain_data[1][0], &terrain_data[1][1]);
		}
		if ((ABS(bx - block_x) > 1) || (ABS(by - block_y) > 1)) {
			load_terrain_block(bx, by, &terrain_data[0][0]);
			load_terrain_block(bx + 1, by, &terrain_data[1][0]);
			load_terrain_block(bx, by + 1, &terrain_data[0][1]);
			load_terrain_block(bx + 1, by + 1, &terrain_data[1][1]);
		} else {
			if (bx > block_x) {
				if (by == block_y) {
					load_terrain_block(bx + 1, by, &terrain_data[1][0]);
					load_terrain_block(bx + 1, by + 1, &terrain_data[1][1]);
				} else if (by > block_y) {
					load_terrain_block(bx, by + 1, &terrain_data[0][1]);
				} else if (by < block_y) {
					load_terrain_block(bx, by, &terrain_data[0][0]);
				}
			} else if (bx < block_x) {
				if (by == block_y) {
					load_terrain_block(bx, by, &terrain_data[0][0]);
					load_terrain_block(bx, by + 1, &terrain_data[0][1]);
				} else if (by > block_y) {
					load_terrain_block(bx + 1, by + 1, &terrain_data[1][1]);
				} else if (by < block_y) {
					load_terrain_block(bx + 1, by, &terrain_data[1][0]);
				}
			} else if (by > block_y) {
				load_terrain_block(bx, by + 1, &terrain_data[0][1]);
				load_terrain_block(bx + 1, by + 1, &terrain_data[1][1]);
			} else if (by < block_y) {
				load_terrain_block(bx, by, &terrain_data[0][0]);
				load_terrain_block(bx + 1, by, &terrain_data[1][0]);
			}
		}
		block_x = bx;
		block_y = by;
	}
}

static void get_point_data(int tx, int ty, Terrain_point *tp)
{
	int index, bx, by;
	short *heights;
	uchar *colours;

	bx = block_i;
	by = block_j;
	if (view_x < 0)
		tx += terrain_width;
	if (tx < 0) {
		tx += terrain_width;
		bx = 0;
	} else if (tx >= terrain_width) {
		tx -= terrain_width;
		bx = 1;
	}
	if (view_z < 0)
		ty += terrain_height;
	if (ty < 0) {
		ty += terrain_height;
		by = 0;
	} else if (ty >= terrain_height) {
		ty -= terrain_height;
		by = 1;
	}
	index = (ty * terrain_width) + tx;
	tp->height = terrain_data[bx][by].heights[index];
	tp->colour = terrain_data[bx][by].colours[index];
}

static int check_line_heights(int x1, int y1, int x2, int y2, int h1, int h2)
{
    int dx, dy, x_step, y_step, gc, i, dh;
	Terrain_point *tp;

	dx = x2 - x1;
    dy = y2 - y1;
	dh = 0;
	if (dy < 0) {
        dy = -dy;
        y_step = -1;
    } else {
        y_step = 1;
    }
    if (dx < 0) {
        dx = -dx;
        x_step = -1;
    } else {
        x_step = 1;
    }
	if (dx > dy) {
		if (dx > 0)
			dh = (h2 - h1) / dx;
        gc = dx >> 1;
        for (i = 0; i <= dx; i++) {
			tp = TPT(x1, y1);
			if (INV_M(h1) < tp->height)
				return FALSE;
			h1 += dh;
			x1 += x_step;
            gc -= dy;
            if (gc < 0) {
                y1 += y_step;
                gc += dx;
            }
        }
    } else {
		if (dy > 0)
			dh = (h2 - h1) / dy;
        gc = dy >> 1;
        for (i = 0; i <= dy; i++) {
			tp = TPT(x1, y1);
			if (INV_M(h1) < tp->height)
				return FALSE;
			h1 += dh;
			y1 += y_step;
            gc -= dx;
            if (gc < 0) {
				x1 += x_step;
                gc += dy;
            }
        }
    }
	return TRUE;
}


/* -------------------
 * GLOBAL check_intervisibility
 * Returns TRUE if line between points is clear of terrain
 * 
 */
int check_intervisibility(sVector *p1, sVector *p2)
{
	int tx1, ty1, tx2, ty2;

	tx1 = TX(p1->x);
	ty1 = TY(p1->z);
	tx2 = TX(p2->x);
	ty2 = TY(p2->z);
	if ((tx1 >= TX1) && (tx1 <= TX2) && (ty1 >= TY1) && (ty1 <= TY2) &&
		(tx2 >= TX1) && (tx2 <= TX2) && (ty2 >= TY1) && (ty2 <= TY2)) {
		tx1 -= TX1;
		ty1 -= TY1;
		tx2 -= TX1;
		ty2 -= TY1;
		return (check_line_heights(tx1, ty1, tx2, ty2, p1->y, p2->y));
	}
	return TRUE;
}


/* -------------------
 * GLOBAL get_terrain_height
 * 
 * Returns height of point x,z in terrain database
 */
int32 get_terrain_height(int32 x, int32 z)
{
	int tx, ty, i, h[4], zero_count;
	Terrain_point *tp;

	tx = TX(x);
	ty = TY(z);
	if ((tx >= TX1) && (tx < TX2) && (ty >= TY1) && (ty < TY2)) {
		zero_count = 0;
		tx -= TX1;
		ty -= TY1;
		tp = TPT(tx, ty);
		if ((h[0] = tp->height) == 0)
			zero_count++;
		tp = TPT(tx+1, ty);
		if ((h[1] = tp->height) == 0)
			zero_count++;
		tp = TPT(tx, ty+1);
		if ((h[2] = tp->height) == 0)
			zero_count++;
		tp = TPT(tx+1, ty+1);
		if ((h[3] = tp->height) == 0)
			zero_count++;
		x = INV_M(x % big_grid_gap);
		z = INV_M(z % big_grid_gap);
		if (x < 0)
			x += grid_gap;
		if (z < 0)
			z += grid_gap;
		if (zero_count == 3) {
			/* Square is half sea */
			for (i = 0; (h[i] == 0) && (i < 4); i++)
				;
			if ((i == 0) || (i == 3)) {
				z = grid_gap - z - 1;
				if (x >= z) {
					h[2] += MULDIV((h[3] - h[2]), x, grid_gap);
					h[2] += MULDIV((h[1] - h[3]), z, grid_gap);
				} else {
					h[2] += MULDIV((h[0] - h[2]), z, grid_gap);
					h[2] += MULDIV((h[1] - h[0]), x, grid_gap);
				}
				return(M(h[2]));
			}
		}
		/* Square is 2 triangles with line from h[0] to h[3] */
		if (x >= z) {
			h[0] += MULDIV((h[1] - h[0]), x, grid_gap);
			h[0] += MULDIV((h[3] - h[1]), z, grid_gap);
		} else {
			h[0] += MULDIV((h[2] - h[0]), z, grid_gap);
			h[0] += MULDIV((h[3] - h[2]), x, grid_gap);
		}
		return(M(h[0]));
	}
	return M(1);
}

/* size argument is in meters eg: 500 */
static void set_grid_gap(int size)
{
	grid_gap = size;
	big_grid_gap = M(size);
	tgrid_gap = Meter(size) >> TSHIFT;
	sgrid_gap = Meter(size) << (SSHIFT - TSHIFT);
	sgrid_radius = Meter(size + (size/2)) << (SSHIFT - TSHIFT);
	block_width = TERRAIN_SIZE * M(size);
	block_height = TERRAIN_SIZE * M(size);
}

static void setup_terrain(void)
{
	int size, i, j;

	set_grid_gap(DEFAULT_GRID_GAP);
	terrain_height = TERRAIN_SIZE;
	terrain_width = TERRAIN_SIZE;
	size = terrain_height * terrain_width;
	for (i = 0; i < 2; i++) {
		for (j = 0; j < 2; j++) {
			terrain_data[i][j].heights = (short *)heap_alloc(size * sizeof(short));
			if (terrain_data[i][j].heights == NULL)
				stop("No memory for terrain heights");
			zero_terrain_heights(terrain_data[i][j].heights);
			terrain_data[i][j].colours = (uchar *)heap_alloc(size * sizeof(uchar));
			if (terrain_data[i][j].colours == NULL)
				stop("No memory for terrain colours");
		}
	}
}

static void init_terrain(void)
{
	init_tp();
}

static void list_add_sorted(sObject *new, sList *objects)
{
	int32 d1, d2, x, z;
	sList *l;
	sObject *o;
	World *w;

	w = new->data;
	x = (w->p.x - view_x);
	z = (w->p.z - view_z);
	d1 = ABS(x) + ABS(z) + w->flat_radius;
	l = objects->next;
	while (l != objects) {
		o = l->item;
		w = o->data;
		x = (w->p.x - view_x);
		z = (w->p.z - view_z);
		d2 = ABS(x) + ABS(z) + w->flat_radius;
		if (d1 < d2) {
			list_insert(new, l);
			break;
		}
		l = l->next;
	}
	if (l == objects) {
		list_add_last(new, objects);
	}
}

static void sort_terrain_objects(sList *objects)
{
	int32 d1, d2, x, z;
	sList *l1, *l2;
	sObject *o;
	World *w;

	l1 = objects->next;
	if (l1 != objects) {
		o = l1->item;
		w = o->data;
		x = (w->p.x - view_x);
		z = (w->p.z - view_z);
		d1 = ABS(x) + ABS(z) + w->flat_radius;
		l2 = l1->next;
		while (l2 != objects) {
			o = l2->item;
			w = o->data;
			x = (w->p.x - view_x);
			z = (w->p.z - view_z);
			d2 = ABS(x) + ABS(z) + w->flat_radius;
			if (d1 > d2) {
				l1->next = l2->next;
				l2->next->prev = l1;
				l2->prev = l1->prev;
				l1->prev->next = l2;
				l1->prev = l2;
				l2->next = l1;
				l2 = l1->next;
			} else {
				l1 = l2;
				l2 = l1->next;
				d1 = d2;
			}
		}
	}
}

static void draw_terrain_objects(sList *objects)
{
	sList   *l;
	sObject *o;
	Base   *data;

	sort_terrain_objects(objects);
	for (l = objects->prev; l != objects; l = l->prev) {
		o = l->item;
        if (o != viewer)
            SafeMethod(o, World, draw)(o);
	}
}

static void draw_terrain_map(int sx, int sy, uchar *cols)
{
	int x, y, index, col;
	short *p;
	uchar pixels[200];

	if (screen_width > 320) {
		p = (short *)pixels;
		for (y = 0; y < terrain_height; y++, sy--) {
			index = y * terrain_width;
			for (x = 0; x < terrain_width; x++, index++) {
				col = cols[index];
				if (col == 0)
					col = SEA_COL;
				else
					col = 0x30 + (col << 2);
				p[x] = col | (col << 8);
			}
			bitmap_prim(sx * 2, sy * 2, 200, pixels);
			bitmap_prim(sx * 2, sy * 2 + 1, 200, pixels);
		}
	} else {
		for (y = 0; y < terrain_height; y++, sy--) {
			index = y * terrain_width;
			for (x = 0; x < terrain_width; x++, index++) {
				col = cols[index];
				if (col == 0)
					col = SEA_COL;
				else
					col = 0x30 + (col << 2);
				pixels[x] = col;
			}
			bitmap_prim(sx, sy, 100, pixels);
		}
	}
}

void draw_map_object(sObject *o, int colour)
{
	int sx, sy;
    World *w;
	Screen_clip s;

	s = clip_region;
    w = o->data;
	sx = 160 + ((w->p.x - terrain_centre_x) / big_grid_gap);
	sy = 100 - ((w->p.z - terrain_centre_z) / big_grid_gap);
	if (screen_width > 320) {
		clipping(120, screen_width - 121, 0, 399);
		block_prim(sx * 2, sy * 2, COL(colour));
	} else {
		clipping(60, screen_width - 61, 0, 199);
		pixel_prim(sx, sy, COL(colour));
	}
	clipping(s.left, s.right, s.top, s.bottom);
}

void draw_ground_map(sObject *view)
{
	Screen_clip s;

	s = clip_region;
	clipping(0, screen_width - 1, 0, screen_height - 1);
	draw_terrain_map(60, terrain_height - 1, terrain_data[0][1].colours);
	draw_terrain_map(60 + terrain_width, terrain_height - 1, terrain_data[1][1].colours);
	draw_terrain_map(60, 2 * terrain_height - 1, terrain_data[0][0].colours);
	draw_terrain_map(60 + terrain_width, 2 * terrain_height - 1, terrain_data[1][0].colours);
	clipping(s.left, s.right, s.top, s.bottom);
}

void ground_view_changed(sObject *view)
{
	int tx, ty, i, n, iters;
	Terrain_point *tp;
    World *m;

    m = view->data;
	view_x = m->p.x;
	view_z = m->p.z;
	real_terrain_x = view_x / big_grid_gap;
	real_terrain_y = view_z / big_grid_gap;
	terrain_x = (view_x / big_grid_gap) % terrain_width;
	terrain_y = (view_z / big_grid_gap) % terrain_height;
	if ((real_terrain_x != last_terrain_x) || (real_terrain_y != last_terrain_y)) {
		update_tp();
	}
	last_terrain_x = real_terrain_x;
	last_terrain_y = real_terrain_y;
	for (ty = -TY1 - 1; ty < -TY1 + 1; ty++) {
		for (tx = -TX1 - 1; tx < -TX1 + 1; tx++) {
			tp = TPT(tx, ty);
			if (tp->objects) {
				iters = list_length(tp->objects) * 2;
				for (n = 0; n < iters; n++)
					sort_terrain_objects(tp->objects);
			}
		}
	}
}

void draw_ground(sObject *view, int dir)
{
	draw_terrain(view, dir);
}

void startup_ground(sObject *view)
{
    World *m;

    m = view->data;
	view_x = m->p.x;
	view_z = m->p.z;
	real_terrain_x = view_x / big_grid_gap;
	real_terrain_y = view_z / big_grid_gap;
	terrain_x = (view_x / big_grid_gap) % terrain_width;
	terrain_y = (view_z / big_grid_gap) % terrain_height;
	terrain_changed = TRUE;
	update_tp();
	reset_tp();
}

void setup_ground(void)
{
	setup_terrain();
/*	shaded_ground = FALSE;
 */
}

void init_ground(void)
{
	init_terrain();
}

