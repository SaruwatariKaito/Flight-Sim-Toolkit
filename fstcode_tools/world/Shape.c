/* FST Shape Viewer */
/* Shape drawing functions */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "lists.h"
#include "defs.h"

typedef unsigned short Pindex;
typedef long int32;

#define MAX_SHAPE_PTS 1024
#define MAX_SHAPE_RADIUS 8000L

#define M(X) ((int32)(X) << 11)
#define MM(X) ((int32)(X) << 1)
#define KM(X) (M(X) * 1000L)
#define Meter(X) ((int32)(X) << 3)

#define MUL(X, Y) ((long)((long)(X) * (long)(Y)))
#define DIV(X, Y) ((long)(X) / (long)(Y))
#define MULDIV(X, Y, Z) (((long)(X) * (long)(Y)) / (long)(Z))

#ifndef ABS
	#define ABS(X) (((X) < 0) ? (-(X)) : (X))
#endif

#ifndef MAX
	#define MAX(X,Y) (((X) > (Y)) ? (X) : (Y))
#endif

#ifndef MIN
	#define MIN(X,Y) (((X) < (Y)) ? (X) : (Y))
#endif

#ifndef SR
	#define SR(V,N) (((V) >= 0) ? ((V) >> (N)) : -(-(V) >> (N)))
#endif

typedef struct {
	int x, y;
} Vector2;

typedef struct {
	int x, y, z;
} Scoord;

typedef struct {
	long x, y, z;
} Vector;

typedef struct {
	int x, y, z;
} Svector;

static Vector zero = {0L, 0L, 0L};

typedef struct {
	int base_colour, colour;
	int npoints;
	Pindex *points;
} Poly;

typedef struct {
	int		npts;
	Scoord	*pts;
	int		npolygons;
	Poly		**polygons;
	Svector	min_size, max_size;
	int		scale, radius;
} Shape;

typedef struct Shape_def {
    char		name[32];
    Shape		*s;
    int32		 visible_dist;
    struct	Shape_def *down;
} Shape_def;

typedef Shape_def** HSHAPE;

static List *loaded_shapes = NULL;
static Shape_def *current_shape = NULL;
static HSHAPE current_handle = NULL;

static Vector2 *spts = NULL;
static Vector *big_pts = NULL;

static int view_x_size = 8, view_y_size = 8;

typedef struct {
	int n;
	List *free;
} Size_def;

static List *free_shape_defs = NULL;
static List *free_shapes = NULL;
static List *free_pts = NULL;
static List *free_polys = NULL;
static List *free_pointers = NULL;
static List *free_pindex = NULL;

static int xcentre = 0, ycentre = 0;
static HDC graph_hdc;

extern int box_size;

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void *get_free_item(int size, List *free_items)
{
	List *l;
	Size_def *s;

	for (l = free_items->next; l != free_items; l = l->next) {
		s = l->item;
		if (s->n == size) {
			return(list_remove_first(s->free));
		}
	}
	return NULL;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void put_free_item(int size, void *free, List *free_items)
{
	List *l;
	Size_def *s;

	for (l = free_items->next; l != free_items; l = l->next) {
		s = l->item;
		if (s->n == size) {
			list_add(free, s->free);
			return;
		}
	}
	s = calloc(1, sizeof(Size_def));
	if (s == NULL) {
		stop("No memory for new Size_def");
	}
	s->n = size;
	s->free = new_list();
	list_add(s, free_items);
	list_add(free, s->free);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static Shape_def *alloc_shape_defs(int nshape_defs)
{
	Shape_def *shape_defs;

	shape_defs = get_free_item(nshape_defs, free_shape_defs);
	if (shape_defs == NULL) {
		shape_defs = calloc(nshape_defs, sizeof(Shape_def));
		if (shape_defs == NULL) {
			stop("No memory for shape_defs");
		}
	}
	return shape_defs;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void discard_shape_defs(int nshape_defs, Shape_def *shape_defs)
{
	put_free_item(nshape_defs, shape_defs, free_shape_defs);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static Shape *alloc_shape(int nshapes)
{
	Shape *shapes;

	shapes = get_free_item(nshapes, free_shapes);
	if (shapes == NULL) {
		shapes = calloc(nshapes, sizeof(Shape));
		if (shapes == NULL) {
			stop("No memory for shapes");
		}
	}
	return shapes;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void discard_shapes(int nshapes, Shape *shapes)
{
	put_free_item(nshapes, shapes, free_shapes);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static Scoord *alloc_pts(int npts)
{
	Scoord *pts;

	pts = get_free_item(npts, free_pts);
	if (pts == NULL) {
		pts = calloc(npts, sizeof(Scoord));
		if (pts == NULL) {
			stop("No memory for shape points");
		}
	}
	return pts;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void discard_pts(int npts, Scoord *pts)
{
	put_free_item(npts, pts, free_pts);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static Poly *alloc_polys(int npolys)
{
	Poly *polys;

	polys = get_free_item(npolys, free_polys);
	if (polys == NULL) {
		polys = calloc(npolys, sizeof(Poly));
		if (polys == NULL) {
			stop("No memory for shape polys");
		}
	}
	return polys;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void discard_polys(int npolys, Poly *polys)
{
	put_free_item(npolys, polys, free_polys);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void *alloc_pointers(int npointers)
{
	char *pointers;

	pointers = get_free_item(npointers, free_pointers);
	if (pointers == NULL) {
		pointers = calloc(npointers, sizeof(char *));
		if (pointers == NULL) {
			stop("No memory for shape pointers");
		}
	}
	return pointers;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void discard_pointers(int npointers, void *pointers)
{
	put_free_item(npointers, pointers, free_pointers);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static Pindex *alloc_pindex(int npindex)
{
	Pindex *pindex;

	pindex = get_free_item(npindex, free_pindex);
	if (pindex == NULL) {
		pindex = calloc(npindex, sizeof(Pindex));
		if (pindex == NULL) {
			stop("No memory for shape pindex");
		}
	}
	return pindex;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void discard_pindex(int npindex, Pindex *pindex)
{
	put_free_item(npindex, pindex, free_pindex);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static int find_scale(int32 val, int32 limit)
{
	int n;

	for (n = 0; val > limit; n++) {
		val >>= 1;
	}
	return n;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void load_points(FILE *fp, Shape *s)
{
	int i, npts, scale;
	int32 x, y, z, xmax, ymax, zmax, radius;
	Vector min, max;
	Scoord *pts;

	npts = s->npts;
	if (npts > MAX_SHAPE_PTS)
		warning("Too many points in shape");

	s->pts = pts = alloc_pts(npts);
	xmax = ymax = zmax = 0L;
	min = zero;
	max = zero;
	for (i = 0; i < npts; i++) {
		fscanf(fp,"%ld %ld %ld", &x, &y, &z);
		if (i < MAX_SHAPE_PTS) {
			big_pts[i].x = x;
			big_pts[i].y = y;
			big_pts[i].z = z;
		}
		if (x > max.x)
			max.x = x;
		else if (x < min.x)
			min.x = x;
		if (y > max.y)
			max.y = y;
		else if (y < min.y)
			min.y = y;
		if (z > max.z)
			max.z = z;
		else if (z < min.z)
			min.z = z;
	}
	xmax = MAX(max.x, -min.x);
	ymax = MAX(max.y, -min.y);
	zmax = MAX(max.z, -min.z);
	radius = MAX(xmax, MAX(ymax, zmax));

	scale = find_scale(radius, MAX_SHAPE_RADIUS);
	if (scale > 0) {
		for (i = 0; i < npts; i++, pts++) {
			if (i < MAX_SHAPE_PTS) {
				pts->x = (int)SR(big_pts[i].x, scale);
		pts->y = (int)SR(big_pts[i].y, scale);
				pts->z = (int)SR(big_pts[i].z, scale);
			}
		}
		s->min_size.x = (int)(min.x >> scale);
		s->min_size.y = (int)(min.y >> scale);
		s->min_size.z = (int)(min.z >> scale);
		s->max_size.x = (int)(max.x >> scale);
		s->max_size.y = (int)(max.y >> scale);
		s->max_size.z = (int)(max.z >> scale);
		s->radius = (int)(radius >> scale);
	} else {
		for (i = 0; i < npts; i++, pts++) {
			if (i < MAX_SHAPE_PTS) {
				pts->x = (int)big_pts[i].x;
		pts->y = (int)big_pts[i].y;
		pts->z = (int)big_pts[i].z;
			}
		}
		s->min_size.x = (int)min.x;
		s->min_size.y = (int)min.y;
		s->min_size.z = (int)min.z;
		s->max_size.x = (int)max.x;
		s->max_size.y = (int)max.y;
		s->max_size.z = (int)max.z;
		s->radius = (int)radius;
	}
	s->scale = scale;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void load_polys(FILE *fp, Shape *s)
{
	int i, npolys, np, col, pt;
	Poly *poly, **pp;
	Pindex *pts;

	npolys = s->npolygons;
	poly = alloc_polys(npolys);
	s->polygons = pp = alloc_pointers(npolys);
	for (i = 0; i < npolys; i++) {
		fscanf(fp, "%x %d", &col, &np);
		if (np == 0)
			stop("Polygon with no points");
		poly->npoints = np;
		poly->points = pts = alloc_pindex(np);
		while (np--) {
			fscanf(fp, "%d", &pt);
			*pts++ = pt;
		}
    col &= 0xff;
		poly->base_colour = col;
		poly->colour = col;
		*pp++ = poly++;
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static Shape *load_shape_data(FILE *fp)
{
	Shape *s;

	s = alloc_shape(1);
	if (s) {
		fscanf(fp, "%d", &s->npts);
		load_points(fp, s);
		fscanf(fp, "%d", &s->npolygons);
		load_polys(fp, s);
	}
	return s;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static Shape_def *load_shape_def(FILE *fp, int nlevels)
{
	Shape_def *sd;

	if (nlevels == 0) {
		return NULL;
     }
	sd = alloc_shape_defs(1);
	if (sd) {
		// visible_dist in meters
		fscanf(fp, "%ld", &sd->visible_dist);
		sd->visible_dist = M(sd->visible_dist);
		sd->s = load_shape_data(fp);
		sd->down = load_shape_def(fp, nlevels-1);
	}
	return sd;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static Shape_def *load_shape_file(char *name)
{
	FILE *fp;
	int version;
	char filetype[32];
	Shape_def *sd;
	int  n;

	sd = NULL;
	fp = fopen(proj_filename(name), "r");
	if (fp) {
		fscanf(fp, "%d %s", &version, filetype);
          if (strcmp(filetype, "FSTShape") == 0) {
			if ((version == 1) || (version == 2)) {
				fscanf(fp, "%d", &n);
				sd = load_shape_def(fp, n);
			}
               else {
				fclose(fp);
				stop("Not version 1 or 2");
			}
		}
          else {
			fclose(fp);
			stop("Not Shape File");
		}
		fclose(fp);
	}
	return sd;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Loads specified shape
static HSHAPE load_shape(char *name)
{
	List *l;
	Shape_def *sd;
	HSHAPE hshape;

	for (l = loaded_shapes->next; l != loaded_shapes; l = l->next) {
		hshape = l->item;
		sd = *hshape;
		if (strcmp(name, sd->name) == 0) {
			return hshape;
		}
	}
	sd = load_shape_file(name);
	if (sd == NULL) {
		hshape = load_shape("box.fsd");
	}

	if (sd) {
		hshape = malloc(sizeof(Shape_def*));
		*hshape = sd;
		list_add_last(hshape, loaded_shapes);
	}
	return hshape;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void destroy_shape_data(Shape *s)
{
	int i;
	Poly *poly, **pp;

	if (s) {
	discard_pts(s->npts, s->pts);
		pp = s->polygons;
		for (i = 0; i < s->npolygons; i++) {
			poly = *pp++;
			discard_pindex(poly->npoints, poly->points);
			discard_polys(1, poly);
    }
		discard_pointers(s->npolygons, s->polygons);
		discard_shapes(1, s);
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void destroy_shape_def(Shape_def *sd)
{
	if (sd) {
		destroy_shape_data(sd->s);
		destroy_shape_def(sd->down);
		discard_shape_defs(1, sd);
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void draw_polygon(int npts, Vector2 *pts, int col)
{
	int i;
	POINT points[9];

	col = col;
	for (i = 0; i < npts; i++) {
		points[i].x = pts[i].x;
		points[i].y = pts[i].y;
	}
	points[npts].x = pts[0].x;
	points[npts].y = pts[0].y;
	Polyline(graph_hdc, points, npts + 1);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void draw_line(int x1, int y1, int x2, int y2, int col)
{
	col = col;

	MoveTo(graph_hdc, x1, y1);
	LineTo(graph_hdc, x2, y2);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void draw_polygons(int npolys, Poly **polys, Vector2 cpts[])
{
	int i, p;
	int pt, pt2, npts;
	Pindex *points;
	//int sx, sy;
	Poly *poly;
	Vector2 spts[8];

	for (p = 0; p < npolys; p++) {
		poly = polys[p];
		points = poly->points;
		npts = poly->npoints;
		if (npts == 2) {
			pt = points[0];
			pt2 = points[1];
			draw_line(cpts[pt].x, cpts[pt].y, cpts[pt2].x, cpts[pt2].y, poly->colour);
		}
		else if (npts > 2) {
			for (i = 0; i < npts; i++) {
				pt = points[i];
				spts[i].x = cpts[pt].x;
				spts[i].y = cpts[pt].y;
			}
			draw_polygon(npts, spts, poly->colour);
		}
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void draw_shape_2d(Shape *shape, int ry)
{
	int i, npts, x, y, sa, ca;
	Scoord *pts;
	Vector2 *sp;

	ry = 360 - ry;

	sa = (int)(1024.0 * sin((float)ry * (3.14159265358979 / 180.0)));	//182.0416666667
	ca = (int)(1024.0 * cos((float)ry * (3.14159265358979 / 180.0)));
	npts = shape->npts;
	if (npts > MAX_SHAPE_PTS) {
		warning("Too many points in selected shape");
		return;
	}
	for (i = npts, pts = shape->pts, sp = spts; i; i--, pts++, sp++) {
		x = (int)SR((MUL(pts->x,ca) - MUL(pts->z,sa)), 10);
		y = (int)SR((MUL(pts->x,sa) + MUL(pts->z,ca)), 10);

		x = MULDIV(x, view_x_size, M(500) >> shape->scale);
		y = MULDIV(y, view_y_size, M(500) >> shape->scale);

		sp->x = xcentre + x;
		sp->y = ycentre - y;
	}
	draw_polygons(shape->npolygons, shape->polygons, spts);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// size is pixels in 500 m
void draw_shape(HDC hdc, HSHAPE hshape, int wx, int wy, int ry, int x_size, int y_size)
{
	int radius;
	Shape_def *sd;
	Shape *s;

	if (hshape) {
		sd = *hshape;
		if (sd) {
			graph_hdc = hdc;
			xcentre = wx;
			ycentre = wy;
			s = sd->s;
			view_x_size = x_size;
			view_y_size = y_size;

			radius = MULDIV(s->radius, view_x_size, M(500) >> s->scale);

			if (radius > 2) {
				draw_shape_2d(s, ry);
				return;
			}
		}
	}
	MoveTo(hdc, wx - box_size, wy - box_size);
	LineTo(hdc, wx + box_size, wy - box_size);
	LineTo(hdc, wx + box_size, wy + box_size);
	LineTo(hdc, wx - box_size, wy + box_size);
	LineTo(hdc, wx - box_size, wy - box_size);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void draw_current_shape(HDC hdc, int wx, int wy, int ry, int x_size, int y_size)
{
	int radius;
	Shape *s;

	if (current_shape) {
		graph_hdc = hdc;
		xcentre = wx;
		ycentre = wy;
		s = current_shape->s;
		view_x_size = x_size;
		view_y_size = y_size;
		radius = MULDIV(s->radius, view_x_size, M(500) >> s->scale);
		if (radius > 1) {
			draw_shape_2d(s, ry);
		}
		//MoveTo(hdc, wx - box_size, wy - box_size);
		//LineTo(hdc, wx + box_size, wy - box_size);
		//LineTo(hdc, wx + box_size, wy + box_size);
		//LineTo(hdc, wx - box_size, wy + box_size);
		//LineTo(hdc, wx - box_size, wy - box_size);
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// height of centre of shape in M
long int current_shape_ycentre(void)
{
	long int yc;

	yc = 0L;
	if (current_shape && current_shape->s) {
		yc = (long int)current_shape->s->min_size.y << current_shape->s->scale;
	}

	return -yc/M(1);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void *load_current_shape(char *file_name, char *shape_name)
{
	HSHAPE hshape;

	if (current_shape) {
		if (strcmp(shape_name, current_shape->name) != 0) {
			//destroy_shape_def(current_shape);
			hshape = load_shape(file_name);
			//new bob
			if (hshape == NULL) {
				hshape = load_shape("box.fsd");
			}
			current_handle = hshape;
			current_shape = *hshape;
			if (current_shape) {
				strcpy(current_shape->name, shape_name);
			}
		}
	}
	else {
		hshape = load_shape(file_name);
		//new bob
		if (hshape == NULL) {
			hshape = load_shape("box.fsd");
		}
		current_handle = hshape;
		current_shape = *hshape;
		if (current_shape) {
			strcpy(current_shape->name, shape_name);
		}
	}
	if (current_shape) {
		return (void*)current_handle;
	}
	return NULL;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void setup_shape(void)
{
	spts = calloc(MAX_SHAPE_PTS, sizeof(Vector2));
	if (spts == NULL) {
		stop("No memory for shape points");
	}
	big_pts = calloc(MAX_SHAPE_PTS, sizeof(Vector));
	if (big_pts == NULL) {
		stop("No memory for shape points");
	}
	free_shape_defs	= new_list();
	free_shapes		= new_list();
	free_pts			= new_list();
	free_polys		= new_list();
	free_pointers		= new_list();
	free_pindex		= new_list();
	loaded_shapes		= new_list();
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void tidyup_shape(void)
{
	free(big_pts);
	free(spts);
}
