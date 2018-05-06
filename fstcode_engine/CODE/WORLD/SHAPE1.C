/* --------------------------------------------------------------------------
 * WORLD		Copyright (c) Simis Ltd 1993,994,1995
 * -----		All Rights Reserved
 *
 * File:		Shape.c
 * Ver:			1.00
 * Desc:		3D Shape loading and drawing module
 *
 * Loads shape geometry from shape.fsd
 * shape texture information from shape.fti
 * and shape texture data from shape.pcx
 *
 * -------------------------------------------------------------------------- */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

#include "io.h"
#include "pic.h"
#include "mem.h"
#include "lists.h"
#include "shape.h"

#include "world.h"


int sorting_algorithm = 1;

int binary_shape_file = FALSE;
static int saved_binary_shape_file = FALSE;

int presort_on = TRUE;
int fast_presort_on = TRUE;

#define MAX_SHAPE_RADIUS 32000L
/*#define SCALE_LIMIT 75000L*/
#define SCALE_LIMIT 32000L
#define FULL_CLIP_SIZE 4500

int quad_textures_on = FALSE;
sBitmap *texture_pic = NULL;
int texture_palette_loaded = FALSE;
sColour texture_palette[256];
static void set_poly_texture1(sShape *s, sPolygon *poly);
static void set_poly_texture2(sShape *s, sPolygon *poly);
void begin_texture_load(void);
void end_texture_load(void);

int sort_count = 0;
int copied_poly_count = 0, invisible_poly_count = 0;

static int shape_version = 1;

static void presort_shape(sShape_def *sd);

static Vertex tpts[MAX_SHAPE_PTS];
static sVector rot_sun = {0, 1 << SSHIFT, 0};

/* Returns shape radius in world coords */
int32 find_size(sShape_def *sd, sVector *min_size, sVector *max_size)
{
	int scale;

	if (sd) {
		while (sd->down) {
			sd = sd->down;
		}
		scale = sd->s->scale;
		min_size->x = sd->s->min_size.x << scale;
		min_size->y = sd->s->min_size.y << scale;
		min_size->z = sd->s->min_size.z << scale;
		max_size->x = sd->s->max_size.x << scale;
		max_size->y = sd->s->max_size.y << scale;
		max_size->z = sd->s->max_size.z << scale;
		return ((int32)sd->s->radius << scale);
	} else {
		*min_size = zero;
		*max_size = zero;
		return 0L;
	}
}

/* Returns shape radius in world coords */
int32 find_size_all(sShape_def *sd, sVector *min_size, sVector *max_size)
{
	int32 radius;
	int scale;
	sVector min, max;
	sShape *s;

	min = zero;
	max = zero;
	radius = 0L;
	scale = 0;
	while (sd) {
		s = sd->s;
		if (s->min_size.x < min.x)
			min.x = s->min_size.x;
		if (s->min_size.y < min.y)
			min.y = s->min_size.y;
		if (s->min_size.z < min.z)
			min.z = s->min_size.z;
		if (s->max_size.x > max.x)
			max.x = s->max_size.x;
		if (s->max_size.y > max.y)
			max.y = s->max_size.y;
		if (s->max_size.z > max.z)
			max.z = s->max_size.z;
		scale = s->scale;
		if (((int32)s->radius << scale) > radius)
			radius = (int32)s->radius << scale;
		sd = sd->down;
	}
	min_size->x = min.x << scale;
	min_size->y = min.y << scale;
	min_size->z = min.z << scale;
	max_size->x = max.x << scale;
	max_size->y = max.y << scale;
	max_size->z = max.z << scale;
	return radius;
}

static void rotate_shape(sShape *s)
{
    int i;
    Vertex *pts, *tp;

    for (i = s->npts, pts = s->pts, tp = tpts; i; i--, pts++, tp++) {
        vertex_rotate(pts, tp);
    }
}

static int poly_col(sPolygon *poly, int col)
{
	int c, a;

	if ((col < 0x90) || (col > 0xff))
		return col;
	if (poly->npoints > 2) {
		a = (int)((MUL(poly->norm.x, rot_sun.x) + MUL(poly->norm.y, rot_sun.y) + MUL(poly->norm.z, rot_sun.z)) >> SSHIFT);
		c = (a + 100) / ((1 << SSHIFT) / 4);
		if (c < -3)
			c = -3;
		else if (c > 4)
			c = 4;
		c = (col - 4) + c;
	} else {
		c = col;
	}
	return c;
}

void recolour_shape(sShape *s, sVector *r)
{
    int i, npolys;
    sPolygon *poly, **polys;
	Transdata tt;

    setup_trans(r, &tt);
	rot3d(&sun_position, &rot_sun, &tt);

	polys = s->polygons;
	npolys = s->npolygons;
	for (i = 0; i < npolys; i++) {
		poly = polys[i];
		poly->colour = poly_col(poly, poly->base_colour);
	}
}

static void poly_dist(sPolygon *poly, int *minp, int *maxp)
{
	int i, min, max;
	int npts;
	Pindex *points;
	Vertex *np;

	points = poly->points;
	npts = poly->npoints;
	min = 30000;
	max = -30000;
	for (i = 0; i < npts; i++) {
		np = &tpts[points[i]];
		if (np->z < min)
			min = np->z;
		if (np->z > max)
			max = np->z;
	}
	*minp = min;
	*maxp = max;
}

static void poly_x(Vertex *pts, sPolygon *poly, int *minp, int *maxp)
{
    int i, min, max;
    int npts;
	Pindex *points;
    Vertex *np;

    points = poly->points;
    npts = poly->npoints;
    min = 30000;
    max = -30000;
    for (i = 0; i < npts; i++) {
        np = &pts[points[i]];
        if (np->x < min)
            min = np->x;
        if (np->x > max)
            max = np->x;
    }
    *minp = min;
    *maxp = max;
}

static void poly_y(Vertex *pts, sPolygon *poly, int *minp, int *maxp)
{
    int i, min, max;
    int npts;
	Pindex *points;
    Vertex *np;

    points = poly->points;
    npts = poly->npoints;
    min = 30000;
    max = -30000;
    for (i = 0; i < npts; i++) {
        np = &pts[points[i]];
        if (np->y < min)
            min = np->y;
        if (np->y > max)
            max = np->y;
    }
    *minp = min;
    *maxp = max;
}

static void poly_z(Vertex *pts, sPolygon *poly, int *minp, int *maxp)
{
    int i, min, max;
    int npts;
	Pindex *points;
    Vertex *np;

    points = poly->points;
    npts = poly->npoints;
    min = 30000;
    max = -30000;
    for (i = 0; i < npts; i++) {
        np = &pts[points[i]];
        if (np->z < min)
            min = np->z;
        if (np->z > max)
            max = np->z;
    }
    *minp = min;
    *maxp = max;
}

/* Shape file data structure:
 *
 * Version_number File_type
 * Number_of_shapes
 * {Shape}
 * Edit_data.
 *
 *     shape = Visible_dist
 *             npoints {point}
 *             npolygons {polygons}
 *             Threshold
 *
 *         point = X Y Z
 *         polygon = COLOUR NPOINTIDS {POINTID}
 */

static char shape_name[32];

static void shape_stop(char *reason)
{
	binary_shape_file = saved_binary_shape_file;
    stop("Shape %s - %s\n", shape_name, reason);
}

/* --------------------------
 * Named access to shape defs
 *
 */
static sList *shapes_list = NULL;

static void reset_shapes_list(void)
{
    if (shapes_list != NULL)
        list_destroy_all(shapes_list);

    shapes_list = new_list();
}

static void add_named_shape(sShape_def *sd, char *name)
{
    strcpy(sd->name, name);
    list_add(sd, shapes_list);
}

sShape_def *get_shape(char *name)
{
    sList *l;
    sShape_def *sd, *s;

    sd = NULL;
    /* If shape is already loaded return pointer to it */
    Map (l, shapes_list) {
        s = l->item;
        if (strcmp(name, s->name) == 0) {
            sd = s;
            break;
        }
    }
    if (sd == NULL) {
        sd = load_shape_file(name);
    }

	if (sd == NULL && (strcmp(name, "nil") != 0)) {
		werror("Shape %s not in project directory", name);
        sd = load_shape_file("BOX.FSD");
		if (sd == NULL)
			stop("BOX.FSD not in FST project");
	}

    return sd;
}

/* --------------------------------------------------------------------------
 * Shape
 * ----- */

static int read_binary_short(FILE *fp)
{
	int val;

	val = fgetc(fp);
	val |= fgetc(fp) << 8;
	return val;
}

static long read_binary_long(FILE *fp)
{
	long val;

	val = (long)fgetc(fp);
	val |= (long)fgetc(fp) << 8;
	val |= (long)fgetc(fp) << 16;
	val |= (long)fgetc(fp) << 24;
	return val;
}

static int read_shape_short(FILE *fp)
{
	int val;

	if (binary_shape_file)
		return read_binary_short(fp);
	fscanf(fp, "%d", &val);
	return val;
}

static long read_shape_long(FILE *fp)
{
	long val;

	if (binary_shape_file)
		return read_binary_long(fp);
	fscanf(fp, "%ld", &val);
	return val;
}

/* Calculate polygon colour based upon Sun position - lighting model */
static int polygon_colour(sPolygon *poly, int col)
{
	int c, a;

	if ((col < 0x90) || (col > 0xff))
		return col;
	if (poly->npoints > 2) {
		a = (int)((MUL(poly->norm.x, sun_position.x) + MUL(poly->norm.y, sun_position.y) + MUL(poly->norm.z, sun_position.z)) >> SSHIFT);
		c = (a + 100) / ((1 << SSHIFT) / 4);
		if (c < -3)
			c = -3;
		else if (c > 4)
			c = 4;
		c = (col - 4) + c;
	} else {
		c = col;
	}
	return c;
}

static int find_scale(int32 val, int32 limit)
{
	int n;

	for (n = 0; val > limit; n++) {
		val >>= 1;
	}
	return n;
}

static void rescale_shape(sShape *s, int new_scale)
{
    int i, shift;
    Vertex *pts;

    shift = s->scale - new_scale;
    if (shift < 0) {
        shift = -shift;
        for (i = s->npts, pts = s->pts; i; i--, pts++) {
            pts->x = SR(pts->x, shift);
            pts->y = SR(pts->y, shift);
            pts->z = SR(pts->z, shift);
        }
        s->min_size.x >>= shift;
        s->min_size.y >>= shift;
        s->min_size.z >>= shift;
        s->max_size.x >>= shift;
        s->max_size.y >>= shift;
        s->max_size.z >>= shift;
        s->radius >>= shift;
    } else if (shift > 0) {
        for (i = s->npts, pts = s->pts; i; i--, pts++) {
            pts->x <<= shift;
            pts->y <<= shift;
            pts->z <<= shift;
        }
        s->min_size.x <<= shift;
        s->min_size.y <<= shift;
        s->min_size.z <<= shift;
        s->max_size.x <<= shift;
        s->max_size.y <<= shift;
        s->max_size.z <<= shift;
        s->radius <<= shift;
    }
    s->scale = new_scale;
}

static void load_points(FILE *fp, sShape *s)
{
    int i, npts, scale;
    int32 x, y, z, xmax, ymax, zmax, radius;
    sVector min, max;
    Vertex *pts;
    sVector big_pts[MAX_SHAPE_PTS];

	npts = s->npts;
    if (npts > MAX_SHAPE_PTS)
        shape_stop("Too many points");
    s->pts = pts = heap_alloc(npts * sizeof(Vertex));
	if (s->pts == NULL)
		shape_stop("Failed to allocate memory for Points array");

    xmax = ymax = zmax = 0L;
    min = zero;
    max = zero;
    for (i = 0; i < npts; i++) {
		x = read_shape_long(fp) / 2;
		y = read_shape_long(fp) / 2;
		z = read_shape_long(fp) / 2;
        big_pts[i].x = x;
        big_pts[i].y = y;
        big_pts[i].z = z;
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
        	pts->x = (int)SR(big_pts[i].x, scale);
        	pts->y = (int)SR(big_pts[i].y, scale);
        	pts->z = (int)SR(big_pts[i].z, scale);
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
        	pts->x = (int)big_pts[i].x;
        	pts->y = (int)big_pts[i].y;
        	pts->z = (int)big_pts[i].z;
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

static void load_polys(FILE *fp, sShape *s)
{
    int i, npolys, np, col, pt1, pt2, pt3;
    sPolygon *poly, **pp;
    Pindex *pts;

	npolys = s->npolygons;
    poly = heap_alloc(npolys * sizeof(sPolygon));
	if (poly == NULL)
		shape_stop("Failed to allocate memory for sPolygon array");

    s->polygons = pp = heap_alloc(npolys * sizeof(sPolygon *));
	if (s->polygons == NULL)
		shape_stop("Failed to allocate memory for sPolygon pointers");
	for (i = 0; i < npolys; i++) {
		if (binary_shape_file) {
			col = read_shape_short(fp);
			np = read_shape_short(fp);
		} else {
	    	fscanf(fp, "%x %d", &col, &np);
		}
		if (np == 0)
			shape_stop("Polygon with no points");
        poly->npoints = np;
        poly->points = pts = heap_alloc(np * sizeof(Pindex));
		if (poly->points == NULL)
			shape_stop("Failed to allocate memory for Polygon points");
        while (np--) {
			pt1 = read_shape_short(fp);
            *pts++ = pt1;
        }
		if (poly->npoints > 2) {
        	pt1 = poly->points[0];
        	pt2 = poly->points[1];
        	pt3 = poly->points[2];
        	poly_normal(&s->pts[pt2], &s->pts[pt1], &s->pts[pt3], &poly->norm);
		} else {
			poly->norm = zero;
		}
		if (col < 0x100) {
			if (shape_version == 1) {
				poly->base_colour = col;
				poly->colour = polygon_colour(poly, col);
			} else {
				if (col < 16) {
					poly->base_colour = col + 15;
				} else if ((col >= 16) && (col < 26)) {
					poly->base_colour = 0xb7 + ((col - 16) * 8);
				} else {
					poly->base_colour = col;
				}
				poly->colour = polygon_colour(poly, poly->base_colour);
			}
		} else {
			if (shape_version == 1) {
				poly->base_colour = poly->colour = col;
				set_poly_texture1(s, poly);
			} else {
				poly->base_colour = poly->colour = 0x100 + (col >> 8);
				set_poly_texture2(s, poly);
			}
		}
		*pp++ = poly++;
    }
}

static sShape *load_shape(FILE *fp)
{
    sShape *s;

    s = block_alloc(sizeof(sShape));
    if (s) {
        s->npts = read_shape_short(fp);
        load_points(fp, s);
        s->npolygons = read_shape_short(fp);
        load_polys(fp, s);
    }
    return s;
}

static sShape_def *load_shape_def(FILE *fp, int nlevels)
{
    sShape_def *sd;
    int n, scale;

    if (nlevels == 0)
        return NULL;

    sd = block_alloc(sizeof(sShape_def));
    if (sd) {
        /* visible_dist in meters */
        sd->visible_dist = read_shape_long(fp);
        sd->visible_dist = M(sd->visible_dist);

        sd->s = load_shape(fp);
        scale = find_scale(sd->visible_dist, SCALE_LIMIT);
        rescale_shape(sd->s, scale);
		if (presort_on)
			presort_shape(sd);

        sd->down = load_shape_def(fp, nlevels-1);
    }
    return sd;
}

static void get_extension(char *file, char *ext)
{
	int i, c;

	for (i = 0; i < 30; i++) {
		c = *file++;
		if ((c == '.') || (c == '\0'))
			break;
	}
	while ((c = *file++) != '\0') {
		if (c >= 97)
			*ext++ = c - 32;
		else
			*ext++ = c;
	}
	*ext = '\0';
}

static void replace_extension(char *file, char *new_ext)
{
	int i, c;

	for (i = 0; i < 26; i++) {
		c = *file++;
		if ((c == '.') || (c == '\0'))
			break;
	}
	if (c == '\0')
		*(file - 1) = '.';
	while ((c = *new_ext++) != '\0') {
		*file++ = c;
	}
	*file = '\0';
}

/* Name is either nil or full file name
 * including .FSD extension or .FSB extension
 * Shape file is binary if binary_shape_file or .FSB extension
 */
sShape_def *load_shape_file(char *name)
{
    FILE *fp;
    char filetype[32];
	char filename[32];
	char extension[32];
    sShape_def *sd;
    int n;

	saved_binary_shape_file = binary_shape_file;
	sd = NULL;
	if (strcmp(name, "nil") == 0)
		return NULL;

    strcpy(shape_name, name); /* shape_name used in error reporting */
    strcpy(filename, name);
	debug("Load shape %s\n", name);
	get_extension(filename, extension);
    if (strcmp(extension, "FSB") == 0) {
		binary_shape_file = TRUE;
	}
	if (binary_shape_file) {
		fp = fopen(filename, "rb");
	} else {
    	fp = fopen(filename, "r");
	}
    if (fp != NULL) {
		if (binary_shape_file) {
			shape_version = read_binary_short(fp);
			strcpy(filetype, "FSTShape");
		} else {
			fscanf(fp, "%d %s", &shape_version, filetype);
		}
        if (strcmp(filetype, "FSTShape") == 0) {
            if ((shape_version == 1) || (shape_version == 2)) {
				n = read_shape_short(fp);
                sd = load_shape_def(fp, n);
                if (sd)
                    add_named_shape(sd, name);
            } else {
				fclose(fp);
				binary_shape_file = saved_binary_shape_file;
                stop("Shape file is not version 1 or 2");
            }
        } else {
			fclose(fp);
			binary_shape_file = saved_binary_shape_file;
			stop("Not Shape File");
        }
		fclose(fp);
	}

/*	end_texture_load();*/
	binary_shape_file = saved_binary_shape_file;
    return sd;
}

/* -----------------------
 * Sorting
 * ----------------------- */

#define XPOS 4
#define YPOS 2
#define ZPOS 1

#define X_MAJOR 8
#define Y_MAJOR 16
#define Z_MAJOR 32

/*static sPolygon **copy_polygons1(sPolygon **polys, int npolys)
{
	int i;
	sPolygon **new_polys, **pp;

	new_polys = pp = heap_alloc(npolys * sizeof(sPolygon *));
	for (i = 0; i < npolys; i++) {
		*pp++ = *polys++;
    }
	return new_polys;
}*/

static sPolygon **copy_polygons(sPolygon **polys, int npolys, int *n_copied, int axis)
{
	int i, n, invisible;
	sList *l;
	sPolygon *p, **new_polys, **pp;

	l = new_list();
	n = 0;
	for (i = 0; i < npolys; i++) {
		p = *polys++;
		invisible = FALSE;
		if ((p->norm.x == -SFACT) && (p->norm.y == 0) && (p->norm.z == 0)) {
			if (axis & XPOS) invisible = TRUE;
		} else if ((p->norm.x == SFACT) && (p->norm.y == 0) && (p->norm.z == 0)) {
			if (!(axis & XPOS)) invisible = TRUE;
		} else if ((p->norm.x == 0) && (p->norm.y == -SFACT) && (p->norm.z == 0)) {
			if (axis & YPOS) invisible = TRUE;
		} else if ((p->norm.x == 0) && (p->norm.y == SFACT) && (p->norm.z == 0)) {
			if (!(axis & YPOS)) invisible = TRUE;
		} else if ((p->norm.x == 0) && (p->norm.y == 0) && (p->norm.z == -SFACT)) {
			if (axis & ZPOS) invisible = TRUE;
		} else if ((p->norm.x == 0) && (p->norm.y == 0) && (p->norm.z == SFACT)) {
			if (!(axis & ZPOS)) invisible = TRUE;
		}
		if (invisible)
			invisible_poly_count++;
		if (!invisible) {
			copied_poly_count++;
			list_add_last(p, l);
			n++;
		}
    }
	new_polys = pp = heap_alloc(n * sizeof(sPolygon *));
	if ((n > 0) && (new_polys == NULL))
		shape_stop("Failed to allocate memory for copied polygons");
	for (i = 0; i < n; i++) {
		*pp++ = list_remove_first(l);
    }
	list_destroy_all(l);
	*n_copied = n;
	return new_polys;
}

static int swap_x(sShape *s, sPolygon *p1, sPolygon *p2)
{
	int min1, min2, max1, max2, max_swap;

	poly_x(s->pts, p1, &min1, &max1);
	poly_x(s->pts, p2, &min2, &max2);
	max_swap = max1 < max2;
	if ((max1 <= min2) && (min1 < min2))
		return TRUE;
	if ((min1 >= max2) && (max1 > max2))
		return FALSE;
	return max_swap;
}

static int swap_y(sShape *s, sPolygon *p1, sPolygon *p2)
{
	int min1, min2, max1, max2, max_swap;

	poly_y(s->pts, p1, &min1, &max1);
	poly_y(s->pts, p2, &min2, &max2);
	max_swap = max1 < max2;
	if ((max1 <= min2) && (min1 < min2))
		return TRUE;
	if ((min1 >= max2) && (max1 > max2))
		return FALSE;
	return max_swap;
}

static int swap_z(sShape *s, sPolygon *p1, sPolygon *p2)
{
	int min1, min2, max1, max2, max_swap;

	poly_z(s->pts, p1, &min1, &max1);
	poly_z(s->pts, p2, &min2, &max2);
	max_swap = max1 < max2;
	if ((max1 <= min2) && (min1 < min2))
		return TRUE;
	if ((min1 >= max2) && (max1 > max2))
		return FALSE;
	return max_swap;
}

static int swap_dist(sShape *s, sPolygon *p1, sPolygon *p2)
{
	int min1, min2, max1, max2, max_swap;

	poly_dist(p1, &min1, &max1);
	poly_dist(p2, &min2, &max2);
	max_swap = max1 < max2;
	if ((max1 <= min2) && (min1 < min2))
		return TRUE;
	if ((min1 >= max2) && (max1 > max2))
		return FALSE;
	return max_swap;
}

static int presort_polygons(sShape *s, sPolygon **polys, int npolys,
    int (*swap_fn)(sShape *s, sPolygon *p1, sPolygon *p2))
{
	int i, j, d, swapped, max_i, max_j;
	sPolygon *p;

	/* Rough initial sort */
	for (d = npolys / 2; d > 0; d >>= 1) {
		for (i = 0; i < (npolys - d); i++) {
			sort_count++;
			if ((*swap_fn)(s, polys[i], polys[i + d])) {
				p = polys[i];
				polys[i] = polys[i + d];
				polys[i + d] = p;
			}
		}
	}
	for (d = npolys / 2; d > 0; d >>= 1) {
    	for (i = npolys - 1; i >= d; i--) {
			sort_count++;
			if (!(*swap_fn)(s, polys[i], polys[i - d])) {
				p = polys[i];
				polys[i] = polys[i - d];
				polys[i - d] = p;
			}
    	}
	}
	/* Finish off with bubble sort */
	max_j = 5;
	max_i = npolys - 1;
	for (j = 0, swapped = TRUE; swapped && (j < max_j); j++) {
		for (i = 0, swapped = FALSE; i < max_i; i++) {
			sort_count++;
			if ((*swap_fn)(s, polys[i], polys[i + 1])) {
				p = polys[i];
				polys[i] = polys[i + 1];
				polys[i + 1] = p;
				swapped = TRUE;
			}
		}
	}
	return(!swapped);
}

/* Assumes start of Vertex overlays sVector */
static void rotate_points(Vertex *pts, int npts, sVector *r)
{
    int i;
    Vertex *tp;
	Transdata trans;

	setup_trans(r, &trans);
	for (i = npts, tp = tpts; i; i--, pts++, tp++) {
        rot3d((sVector *)pts, (sVector *)tp, &trans);
    }
}

static sPolygon **presort_axis(sShape_def *sd, int *pnp, int rx, int ry, int axis)
{
	sVector r;
	sPolygon **polys;

	strcpy(shape_name, sd->name);
	polys = copy_polygons(sd->first_polys, sd->n_first_polys, pnp, axis);
	r.x = rx; r.y = ry; r.z = 0;
	rotate_points(sd->s->pts, sd->s->npts, &r);
	presort_polygons(sd->s, polys, *pnp, swap_dist);
	return polys;
}

static void presort_shape(sShape_def *sd)
{
	sd->first_polys = sd->s->polygons;
	sd->n_first_polys = sd->s->npolygons;
	if (fast_presort_on) {
		sd->xp_yp_zp = NULL;
		sd->xp_yp_zm = NULL;
		sd->xp_ym_zp = NULL;
		sd->xp_ym_zm = NULL;
		sd->xm_yp_zp = NULL;
		sd->xm_yp_zm = NULL;
		sd->xm_ym_zp = NULL;
		sd->xm_ym_zm = NULL;
	} else {
		sd->xp_yp_zp = presort_axis(sd, &sd->n_xp_yp_zp, DEG(-45), DEG(-135), XPOS | YPOS | ZPOS);
		sd->xp_yp_zm = presort_axis(sd, &sd->n_xp_yp_zm, DEG(45), DEG(-45), XPOS | YPOS);
		sd->xp_ym_zp = presort_axis(sd, &sd->n_xp_ym_zp, DEG(45), DEG(-135), XPOS | ZPOS);
		sd->xp_ym_zm = presort_axis(sd, &sd->n_xp_ym_zm, DEG(-45), DEG(-45), XPOS);
		sd->xm_yp_zp = presort_axis(sd, &sd->n_xm_yp_zp, DEG(-45), DEG(135), YPOS | ZPOS);
		sd->xm_yp_zm = presort_axis(sd, &sd->n_xm_yp_zm, DEG(45), DEG(45), YPOS);
		sd->xm_ym_zp = presort_axis(sd, &sd->n_xm_ym_zp, DEG(45), DEG(135), ZPOS);
		sd->xm_ym_zm = presort_axis(sd, &sd->n_xm_ym_zm, DEG(-45), DEG(45), 0);
	}
}

static void choose_presorted(sShape_def *sd, sVector *pos, sVector *rot)
{
    int axis, i, sorted;
    sVector p, ap;
    Transdata trans;

    setup_trans(rot, &trans);
	p.x = (viewer_p->x - pos->x) >> SPEED_SHIFT;
    p.y = (viewer_p->y - pos->y) >> SPEED_SHIFT;
    p.z = (viewer_p->z - pos->z) >> SPEED_SHIFT;
    invrot3d(&p, &p, &trans);
    ap.x = ABS(p.x);
    ap.y = ABS(p.y);
    ap.z = ABS(p.z);

    axis = 0;
    if (p.x >= 0)
        axis |= XPOS;
    if (p.y >= 0)
        axis |= YPOS;
    if (p.z >= 0)
        axis |= ZPOS;
	switch (axis) {
		case XPOS | YPOS | ZPOS:
			if (sd->xp_yp_zp == NULL)
				sd->xp_yp_zp = presort_axis(sd, &sd->n_xp_yp_zp, DEG(-45), DEG(-135), XPOS | YPOS | ZPOS);
			sd->s->polygons = sd->xp_yp_zp;
			sd->s->npolygons = sd->n_xp_yp_zp;
			break;
		case XPOS | YPOS:
			if (sd->xp_yp_zm == NULL)
				sd->xp_yp_zm = presort_axis(sd, &sd->n_xp_yp_zm, DEG(45), DEG(-45), XPOS | YPOS);
			sd->s->polygons = sd->xp_yp_zm;
			sd->s->npolygons = sd->n_xp_yp_zm;
			break;
		case XPOS | ZPOS:
			if (sd->xp_ym_zp == NULL)
				sd->xp_ym_zp = presort_axis(sd, &sd->n_xp_ym_zp, DEG(45), DEG(-135), XPOS | ZPOS);
			sd->s->polygons = sd->xp_ym_zp;
			sd->s->npolygons = sd->n_xp_ym_zp;
			break;
		case XPOS:
			if (sd->xp_ym_zm == NULL)
				sd->xp_ym_zm = presort_axis(sd, &sd->n_xp_ym_zm, DEG(-45), DEG(-45), XPOS);
			sd->s->polygons = sd->xp_ym_zm;
			sd->s->npolygons = sd->n_xp_ym_zm;
			break;
		case YPOS | ZPOS:
			if (sd->xm_yp_zp == NULL)
				sd->xm_yp_zp = presort_axis(sd, &sd->n_xm_yp_zp, DEG(-45), DEG(135), YPOS | ZPOS);
			sd->s->polygons = sd->xm_yp_zp;
			sd->s->npolygons = sd->n_xm_yp_zp;
			break;
		case YPOS:
			if (sd->xm_yp_zm == NULL)
				sd->xm_yp_zm = presort_axis(sd, &sd->n_xm_yp_zm, DEG(45), DEG(45), YPOS);
			sd->s->polygons = sd->xm_yp_zm;
			sd->s->npolygons = sd->n_xm_yp_zm;
			break;
		case ZPOS:
			if (sd->xm_ym_zp == NULL)
				sd->xm_ym_zp = presort_axis(sd, &sd->n_xm_ym_zp, DEG(45), DEG(135), ZPOS);
			sd->s->polygons = sd->xm_ym_zp;
			sd->s->npolygons = sd->n_xm_ym_zp;
			break;
		case 0:
			if (sd->xm_ym_zm == NULL)
				sd->xm_ym_zm = presort_axis(sd, &sd->n_xm_ym_zm, DEG(-45), DEG(45), 0);
			sd->s->polygons = sd->xm_ym_zm;
			sd->s->npolygons = sd->n_xm_ym_zm;
			break;
	}
}

/* TRUE if p1 and p2 should swap (p1 is nearer than p2) */

static int swap_test(sShape *s, sPolygon *p1, sPolygon *p2, int aspect)
{
	int min1, min2, max1, max2, max_swap;

	poly_dist(p1, &min1, &max1);
	poly_dist(p2, &min2, &max2);
	max_swap = max1 < max2;
	if ((max1 <= min2) && (min1 < min2))
		return TRUE;
	if ((min1 >= max2) && (max1 > max2))
		return FALSE;
	return max_swap;
}

/* Good for ground objects like carriers */
static int swap_test2(sShape *s, sPolygon *p1, sPolygon *p2, int aspect)
{
	int min1, min2, max1, max2, max_swap;

	poly_y(s->pts, p1, &min1, &max1);
	poly_y(s->pts, p2, &min2, &max2);
	if (aspect & YPOS) {
		if (min1 > min2)
			return TRUE;
		return FALSE;
	} else {
		if ((max1 <= max2) && (min1 < min2))
			return TRUE;
		return FALSE;
	}
	poly_dist(p1, &min1, &max1);
	poly_dist(p2, &min2, &max2);
	max_swap = max1 < max2;
	if ((max1 <= min2) && (min1 < min2))
		return TRUE;
	if ((min1 >= max2) && (max1 > max2))
		return FALSE;
	return max_swap;
}

static int sort_polygons(sShape *s,
    int (*swap_fn)(sShape *s, sPolygon *p1, sPolygon *p2, int aspect), int aspect)
{
	int i, j, d, swapped, max;
	sPolygon *p, **polys;

	max = s->npolygons - 1;
	polys = s->polygons;
	for (j = 0, swapped = TRUE; swapped && (j < 5); j++) {
		for (i = 0, swapped = FALSE; i < max; i++) {
			sort_count++;
			if ((*swap_fn)(s, polys[i], polys[i + 1], aspect)) {
				p = polys[i];
				polys[i] = polys[i + 1];
				polys[i + 1] = p;
				swapped = TRUE;
			}
		}
	}
	return(!swapped);
}

static void sort(sShape_def *sd, sVector *pos, sVector *rot)
{
    int axis, i, sorted;
	int (*swap_fn)(sShape *s, sPolygon *p1, sPolygon *p2, int aspect);
    sVector p, ap;
    Transdata trans;

	p.x = (viewer_p->x - pos->x) >> SPEED_SHIFT;
    p.y = (viewer_p->y - pos->y) >> SPEED_SHIFT;
    p.z = (viewer_p->z - pos->z) >> SPEED_SHIFT;
    setup_trans(rot, &trans);
    invrot3d(&p, &p, &trans);
    ap.x = ABS(p.x);
    ap.y = ABS(p.y);
    ap.z = ABS(p.z);
	if (pos->y == 0)
		swap_fn = swap_test2;
	else
		swap_fn = swap_test;

    axis = 0;
    if (p.x >= 0)
        axis |= XPOS;
    if (p.y >= 0)
        axis |= YPOS;
    if (p.z >= 0)
        axis |= ZPOS;
    if (ap.x >= ap.y) {
        if (ap.x >= ap.z) {
            axis = X_MAJOR;
        } else {
            axis = Z_MAJOR;
        }
    } else {
        if (ap.y >= ap.z) {
            axis = Y_MAJOR;
        } else {
            axis = Z_MAJOR;
        }
    }
    if (axis != sd->flags) {
		sd->flags = axis;
		rotate_shape(sd->s);
		sorted = sort_polygons(sd->s, swap_fn, axis);
		if (!sorted)
			sd->flags = 0;
    }
}

int draw_shape_def(sShape_def *sd, sVector *sp, sVector *sr, int shaded)
{
	int32 adx, adz;
    int ardx, ardy, radius, scale;
	int32 dist, range;
	sVector d;
	sVector v;

    d.x = sp->x - viewer_p->x;
    d.y = sp->y - viewer_p->y;
    d.z = sp->z - viewer_p->z;

    dist = sd->visible_dist;
	if ((ABS(d.x) > dist) || (ABS(d.z) > dist) || (ABS(d.y) > dist)) {
        /* Do not draw */
        return FALSE;
	} else {
		/* Calc range to shape and recurse to correct level */
    	adx = ABS(d.x);
    	adz = ABS(d.z);
    	if (adx > adz)
        	range = (adx + ((ABS(d.y)+adz) >> 2));
		else
    		range = (adz + ((ABS(d.y)+adx) >> 2));
		while (sd->down && range < sd->down->visible_dist)
			sd = sd->down;

        scale = sd->s->scale;
		v.x = (int)(d.x >> scale);
		v.y = (int)(d.y >> scale);
		v.z = (int)(d.z >> scale);
        radius = sd->s->radius;
		radius += radius >> 1;
		push_matrix();
    	translate_abs(v.x, v.y, v.z);
		v.z = (int)(matrix->l >> SSHIFT);
    	if ((v.z + radius) > VIEW_MIN) {
			v.x = (int)(matrix->j >> SSHIFT);
			v.y = (int)(matrix->k >> SSHIFT);
			ardx = ABS(v.x);
			ardy = ABS(v.y);
            if (((ardy - radius) < v.z) && ((ardx - radius) < v.z)) {
                roty(-sr->y);
                rotx(-sr->x);
                rotz(-sr->z);
				if (shaded) {
					recolour_shape(sd->s, sr);
				}
				if (presort_on)
					choose_presorted(sd, sp, sr);
				else
					sort(sd, sp, sr);
                if (v.z < radius) {
                    if (radius < FULL_CLIP_SIZE) {
                        shape3(sd->s, 0);
                    } else {
                        near_shape3(sd->s, 0);
					}
                } else {
                    far_shape3(sd->s, 0);
                }
                pop_matrix();
                return TRUE;
            }
    	}
    	pop_matrix();
	}
    return FALSE;
}

int draw_shape(sShape *s, sVector *sp, sVector *sr, int shaded)
{
    int ardx, ardy, radius, scale;
	sVector *p;
	sVector d;
	sVector v;

	p = sp;
    d.x = p->x - viewer_p->x;
    d.y = p->y - viewer_p->y;
    d.z = p->z - viewer_p->z;

    scale = s->scale;
    v.x = (int)(d.x >> scale);
    v.y = (int)(d.y >> scale);
    v.z = (int)(d.z >> scale);
    radius = s->radius;
    radius += radius >> 1;
    push_matrix();
    translate_abs(v.x, v.y, v.z);
    v.z = (int)(matrix->l >> SSHIFT);
    if ((v.z + radius) > VIEW_MIN) {
        v.x = (int)(matrix->j >> SSHIFT);
        v.y = (int)(matrix->k >> SSHIFT);
        ardx = ABS(v.x);
        ardy = ABS(v.y);
            if (((ardy - radius) < v.z) && ((ardx - radius) < v.z)) {
                roty(-sr->y);
                rotx(-sr->x);
                rotz(-sr->z);
                if (v.z < radius) {
                    if (radius < FULL_CLIP_SIZE)
                        shape3(s, 0);
                    else
                        near_shape3(s, 0);
                } else {
                    far_shape3(s, 0);
                }
                pop_matrix();
                return TRUE;
            }
    }
    pop_matrix();

    return FALSE;
}

static Vertex flat_pts[MAX_SHAPE_PTS];
static sShape flat_shape = {0, flat_pts, 0, NULL, {0,0,0}, {0,0,0}, 0, 0};

static sShape *flatten_shape(sShape *s, sVector *r)
{
	int i, npts;
	Vertex *pts1, *pts2;
	Transdata t;

	setup_trans(r, &t);
	npts = s->npts;
	for (i = npts, pts1 = s->pts, pts2 = flat_pts; i; i--, pts1++, pts2++) {
		pts2->x = (int)((MUL(pts1->x,t.a) + MUL(pts1->y,t.b) + MUL(pts1->z,t.c)) >> SSHIFT);
		pts2->z = (int)((MUL(pts1->x,t.g) + MUL(pts1->y,t.h) + MUL(pts1->z,t.i)) >> SSHIFT);
		pts2->y = 0;
	}
	flat_shape.npts = npts;
	flat_shape.npolygons = s->npolygons;
	flat_shape.polygons = s->polygons;
	flat_shape.scale = s->scale;
	return &flat_shape;
}

static unsigned int shadow_cols[256];

int draw_shadow(sShape_def *sd, sVector *sp, sVector *sr)
{
	int32 adx, adz;
    int ardx, ardy, radius, scale, i;
	int32 dist, range;
	unsigned *saved_cols;
	sShape *s;
	sVector d;
	sVector v;

    d.x = sp->x - viewer_p->x;
    d.y = sp->y - viewer_p->y;
    d.z = sp->z - viewer_p->z;

    dist = sd->visible_dist;
	if ((ABS(d.x) > dist) || (ABS(d.z) > dist) || (ABS(d.y) > dist)) {
        /* Do not draw */
        return FALSE;
	} else {
		/* Calc range to shape */
    	adx = ABS(d.x);
    	adz = ABS(d.z);
    	if (adx > adz)
        	range = (adx + ((ABS(d.y)+adz) >> 2));
		else
    		range = (adz + ((ABS(d.y)+adx) >> 2));
		/* Always draw level 2 shape */
		if (sd->down)
			sd = sd->down;

        scale = sd->s->scale;
		v.x = (int)(d.x >> scale);
		v.y = (int)(d.y >> scale);
		v.z = (int)(d.z >> scale);
        radius = sd->s->radius;
		radius += radius >> 1;
		push_matrix();
    	translate_abs(v.x, v.y, v.z);
		v.z = (int)(matrix->l >> SSHIFT);
    	if ((v.z + radius) > VIEW_MIN) {
			v.x = (int)(matrix->j >> SSHIFT);
			v.y = (int)(matrix->k >> SSHIFT);
			ardx = ABS(v.x);
			ardy = ABS(v.y);
            if (((ardy - radius) < v.z) && ((ardx - radius) < v.z)) {
				saved_cols = cols;
				cols = shadow_cols;
				s = flatten_shape(sd->s, sr);
                if (v.z < radius) {
                    if (radius < FULL_CLIP_SIZE) {
                        shape3(s, 0);
                    } else {
                        near_shape3(s, 0);
					}
                } else {
                    far_shape3(s, 0);
                }
				cols = saved_cols;
                pop_matrix();
                return TRUE;
            }
    	}
    	pop_matrix();
	}
    return FALSE;
}

/* Texture loading */
/* Polygon colours > 0xff are texture indices into <shape>.pcx file */

static char texture_shape_name[32] = "";

/* Copies a region from bitmap into texture */
void grab_texture(uchar *tex, sBitmap *pic, int pic_width, int offset, int x, int y, int width, int height)
{
	sBitmap *p;
	int i, y_max;
	uchar c;

	for (y_max = y + height; y < y_max; y++) {
		p = pic + (pic_width * y) + x;
		for (i = 0; i < width; i++) {
			c = *p++;
			if (c)
				*tex++ = c + offset;
			else
				*tex++ = 0;
		}
	}
}

static void resize_texture(Texture *t)
{
	int x, y;
	uchar *p;

	p = t->map + (t->width * t->height) - 1;
	for (y = t->height - 1; y > 0; y--) {
		for (x = t->width; x > 0; x--) {
			if (*p-- != 0) {
				t->height = y + 1;
				return;
			}
		}
	}
	t->height = 1;
}

/* Uses polygon outline for texture points */
static void outline_texture(sShape *s, sPolygon *poly, Texture *tex)
{
	int i, pt, x, y, a, sa, ca, size;
	sVector min, max, pts[4];

	if (poly->npoints > 4)
		return;
	pt = poly->points[0];
	pts[0].x = s->pts[pt].x;
	pts[0].y = s->pts[pt].y;
	pts[0].z = s->pts[pt].z;
	/* Point 0 is origin */
	for (i = 1; i < poly->npoints; i++) {
		pt = poly->points[i];
		pts[i].x = s->pts[pt].x - pts[0].x;
		pts[i].y = s->pts[pt].y - pts[0].y;
		pts[i].z = s->pts[pt].z - pts[0].z;
	}
	pts[0] = zero;
/*	for (i = 0; i < poly->npoints; i++)
		printf("a pt %d: %d %d %d\n", i, pts[i].x, pts[i].y, pts[i].z);
	printf("\n");*/

	/* Yaw align point 1 along x axis */
	if ((pts[1].z != 0) || (pts[1].x < 0)) {
		a = fast_atan(-pts[1].z, pts[1].x);
		sa = SIN(a); ca = COS(a);
		for (i = 1; i < poly->npoints; i++) {
			x = pts[i].x;
			y = pts[i].z;
			pts[i].x = (int)SR((MUL(x,ca) - MUL(y,sa)), SSHIFT);
			pts[i].z = (int)SR((MUL(x,sa) + MUL(y,ca)), SSHIFT);
		}
	}
/*	printf("angle %d\n", a / DEG_1);
	for (i = 0; i < poly->npoints; i++)
		printf("b pt %d: %d %d %d\n", i, pts[i].x, pts[i].y, pts[i].z);
	printf("\n");*/

	/* Pitch align point 1 along x axis */
	if ((pts[1].y != 0) || (pts[1].x < 0)) {
		a = fast_atan(-pts[1].y, pts[1].x);
		sa = SIN(a); ca = COS(a);
		for (i = 1; i < poly->npoints; i++) {
			x = pts[i].x;
			y = pts[i].y;
			pts[i].x = (int)SR((MUL(x,ca) - MUL(y,sa)), SSHIFT);
			pts[i].y = (int)SR((MUL(x,sa) + MUL(y,ca)), SSHIFT);
		}
	}
/*	printf("angle %d\n", a / DEG_1);
	for (i = 0; i < poly->npoints; i++)
		printf("c pt %d: %d %d %d\n", i, pts[i].x, pts[i].y, pts[i].z);
	printf("\n");*/

	/* Roll align point 2 along x axis */
	if ((pts[2].y != 0) || (pts[2].z > 0)) {
		a = fast_atan(-pts[2].y, -pts[2].z);
		sa = SIN(a); ca = COS(a);
		for (i = 1; i < poly->npoints; i++) {
			x = -pts[i].z;
			y = pts[i].y;
			pts[i].z = -(int)SR((MUL(x,ca) - MUL(y,sa)), SSHIFT);
			pts[i].y = (int)SR((MUL(x,sa) + MUL(y,ca)), SSHIFT);
		}
	}
/*	printf("angle %d\n", a / DEG_1);
	for (i = 0; i < poly->npoints; i++)
		printf("d pt %d: %d %d %d\n", i, pts[i].x, pts[i].y, pts[i].z);
	printf("\n");*/

	/* Find 2D bounding box in x/z plane */
	min.x = min.z = 32000;
	max.x = max.z = -32000;
	for (i = 0; i < poly->npoints; i++) {
		if (pts[i].x < min.x)
			min.x = pts[i].x;
		if (pts[i].x > max.x)
			max.x = pts[i].x;
		if (pts[i].z < min.z)
			min.z = pts[i].z;
		if (pts[i].z > max.z)
			max.z = pts[i].z;
	}
	/* Reoriginate at corner of bounding box */
	for (i = 0; i < poly->npoints; i++) {
		pts[i].x -= min.x;
		pts[i].z -= max.z;
	}
/*	printf("min.x %d max.x %d: min.z %d max.z %d\n", min.x, max.x, min.z, max.z);
	for (i = 0; i < poly->npoints; i++)
		printf("e pt %d: %d %d %d\n", i, pts[i].x, pts[i].y, pts[i].z);
	printf("\n");*/

	if (pts[1].x != 0) {
		size = MAX(max.x - min.x, max.z - min.z);
		for (i = 0; i < poly->npoints; i++) {
			tex->pts[i].x = (pts[i].x * (tex->width - 1)) / size;
			tex->pts[i].y = (-pts[i].z * (tex->width - 1)) / size;
			if ((tex->pts[i].x >= tex->width) || (tex->pts[i].y >= tex->width)) {
/*				stop("Oversize textured polygon in shape %s", shape_name);*/
				printf("Oversize textured polygon in shape %s: %d %d\n", shape_name);
			}
		}
	}
/*	for (i = 0; i < poly->npoints; i++)
		printf("tex %d: %d %d\n", i, tex->pts[i].x, tex->pts[i].y);
	printf("\n");*/
}

#define TEX_WIDTH 64
#define TEX_HEIGHT 64
#define TEX_FILE_HEIGHT 480
#define MAX_TEXTURES ((640 / TEX_WIDTH) * (TEX_FILE_HEIGHT / TEX_HEIGHT))
#define TEX_COLOURS 64

static void set_poly_texture1(sShape *s, sPolygon *poly)
{
	int base_col, index, found, x, y, tiles_across;
	Texture *t;
	static char texture_file[32];

	base_col = 0x90;
	index = poly->colour - 0x100;
	if (index >= MAX_TEXTURES)
		stop("More than %d textures in shape %s", MAX_TEXTURES, shape_name);
	if (texture_pic == NULL)
		begin_texture_load();
	if (strcmp(shape_name, texture_shape_name) != 0) {
		strcpy(texture_shape_name, shape_name);
		strcpy(texture_file, shape_name);
		replace_extension(texture_file, "PCX");
		if (texture_palette_loaded) {
			found = load_pic(texture_file, texture_pic, 640, TEX_FILE_HEIGHT, FALSE, TEX_COLOURS);
		} else {
			found = load_pic(texture_file, texture_pic, 640, TEX_FILE_HEIGHT, TRUE, TEX_COLOURS);
			if (found) {
				read_palette(texture_palette, 0x00, TEX_COLOURS - 1);
				write_palette(texture_palette, base_col, base_col + TEX_COLOURS - 1);
				save_as_base_palette(base_col, base_col + TEX_COLOURS - 1);
				texture_palette_loaded = TRUE;
			}
		}
		if (!found) {
			stop("Texture file %s not found", texture_file);
		}
	}
	t = (Texture *)heap_alloc(sizeof(Texture));
	if (t == NULL)
		stop("No memory for texture from %s", texture_file);
	t->width = TEX_WIDTH;
	t->height = TEX_HEIGHT;
	t->map = (uchar *)heap_alloc(TEX_WIDTH * TEX_HEIGHT);
	if (t->map == NULL)
		stop("No memory for texture map from %s", texture_file);
	tiles_across = PCX_info.width / TEX_WIDTH;
	y = (index / tiles_across) * TEX_HEIGHT;
	x = (index % tiles_across) * TEX_WIDTH;
	grab_texture(t->map, texture_pic, PCX_info.width, base_col, x, y, t->width, t->height);
	resize_texture(t);
	t->pts[0].x = 0;
	t->pts[0].y = 0;
	t->pts[1].x = t->width - 1;
	t->pts[1].y = 0;
	t->pts[2].x = t->width - 1;
	t->pts[2].y = t->height - 1;
	t->pts[3].x = 0;
	t->pts[3].y = t->height - 1;
	if (!quad_textures_on) {
		outline_texture(s, poly, t);
	}
	poly->base_colour = add_texture(t) + 0x100;
	poly->colour = poly->base_colour;
}

typedef struct {
	int id, npts;
	sVector2 pts[4];
} Texture_def;

static sList *texture_defs = NULL;

static void clear_texture_defs(void)
{
	sList *l;
	Texture_def *td;

	if (texture_defs != NULL) {
		for (l = texture_defs->next; l != texture_defs; l = l->next) {
			td = l->item;
			free(td);
		}
		list_destroy(texture_defs);
	}
}

static Texture_def *get_texture_def(int id)
{
	sList *l;
	Texture_def *td;

	for (l = texture_defs->next; l != texture_defs; l = l->next) {
		td = l->item;
		if (td->id == id) {
			return td;
		}
	}
	return NULL;
}

static int load_texture_file(char *texture_file)
{
	int i, n, x, y, version, width, height, id, npts;
	FILE *fp;
	Texture_def *td;
	char s[32];

	fp = fopen(texture_file, "r");
	if (fp != NULL) {
		if (texture_defs == NULL)
			texture_defs = new_list();
		fscanf(fp, "%s %d %d %d\n", s, &version, &width, &height);
		do {
			n = fscanf(fp, "%d %d", &id, &npts);
			if (n > 0) {
				td = heap_alloc(sizeof(Texture_def));
				if (td == NULL)
					stop("No memory for texture def");
				td->id = id;
				td->npts = npts;
				for (i = 0; i < npts; i++) {
					n = fscanf(fp, "%d %d", &x, &y);
					td->pts[i].x = x >> 16;
					td->pts[i].y = y >> 16;
				}
				list_add(td, texture_defs);
			}
		} while (n > 0);
		fclose(fp);
		return TRUE;
	}
	return FALSE;
}

static void set_tex_position(Texture *tex, char *texture_file, int base_col)
{
	int i;
	sVector2 min, max;

	min.x = min.y = 32000;
	max.x = max.y = 0;
	for (i = 0; i < 4; i++) {
		if (tex->pts[i].x < min.x)
			min.x = tex->pts[i].x;
		if (tex->pts[i].x > max.x)
			max.x = tex->pts[i].x;
		if (tex->pts[i].y < min.y)
			min.y = tex->pts[i].y;
		if (tex->pts[i].y > max.y)
			max.y = tex->pts[i].y;
	}
	for (i = 0; i < 4; i++) {
		tex->pts[i].x -= min.x;
		tex->pts[i].y -= min.y;
	}
	tex->width = max.x - min.x;
	tex->height = max.y - min.y;
	tex->map = (uchar *)heap_alloc(tex->width * tex->height);
	if (tex->map == NULL)
		stop("No memory for texture map from %s", texture_file);
	grab_texture(tex->map, texture_pic, PCX_info.width, base_col, min.x, min.y, tex->width, tex->height);
}

static void set_poly_texture2(sShape *s, sPolygon *poly)
{
	int found, i, x, y, tex_id, base_col;
	Texture *tex;
	Texture_def *td;
	static char texture_file[32];
	static char pic_file[32];

	base_col = 0x90;
	tex_id = poly->colour - 0x100;
	if (poly->npoints > 4)
		stop("Polygon with %d points, tex id %d in %s", poly->npoints, tex_id, shape_name);
	if (strcmp(shape_name, texture_shape_name) != 0) {
		strcpy(texture_shape_name, shape_name);
		strcpy(pic_file, shape_name);
		replace_extension(pic_file, "PCX");
		if (texture_palette_loaded) {
			found = load_pic(pic_file, texture_pic, 640, TEX_FILE_HEIGHT, FALSE, TEX_COLOURS);
		} else {
			found = load_pic(pic_file, texture_pic, 640, TEX_FILE_HEIGHT, TRUE, TEX_COLOURS);
			if (found) {
				read_palette(texture_palette, 0x00, TEX_COLOURS - 1);
				write_palette(texture_palette, base_col, base_col + TEX_COLOURS - 1);
				save_as_base_palette(base_col, base_col + TEX_COLOURS - 1);
				texture_palette_loaded = TRUE;
			}
		}
		if (!found) {
			stop("Texture file %s not found", texture_file);
		}
		strcpy(texture_file, shape_name);
		replace_extension(texture_file, "FTI");
		clear_texture_defs();
		found = load_texture_file(texture_file);
		if (!found) {
			stop("Texture file %s not found", texture_file);
		}
	}
	td = get_texture_def(tex_id);
	if (td == NULL)
		stop("Bad texture id in %s", shape_name);
	tex = (Texture *)heap_alloc(sizeof(Texture));
	if (tex == NULL)
		stop("No memory for texture from %s", texture_file);
	for (i = 0; i < td->npts; i++) {
		tex->pts[i] = td->pts[i];
	}
	for (i = td->npts; i < 4; i++) {
		tex->pts[i] = tex->pts[0];
	}
	set_tex_position(tex, texture_file, base_col);
	poly->base_colour = add_texture(tex) + 0x100;
	poly->colour = poly->base_colour;
}

void begin_texture_load(void)
{
	if (texture_pic == NULL)
		texture_pic = (sBitmap *)heap_alloc(640 * TEX_FILE_HEIGHT);
	if (texture_pic == NULL)
		stop("No memory for texture load bitmap");
}

void end_texture_load(void)
{
	if (texture_pic != NULL)
		heap_free(texture_pic);
	strcpy(texture_shape_name, "");
}

void init_shape(void)
{
}

void setup_shape(void)
{
	int i;

	begin_texture_load();
	for (i = 0; i < 256; i++)
    	shadow_cols[i] = EXPAND_COL(0x1e);
    reset_shapes_list();
}

void reset_shape(void)
{
    reset_shapes_list();
}

void tidyup_shape(void)
{
}

void shapes_loaded(void)
{
    sList *l;
    sShape_def *sd;

    Map (l, shapes_list) {
        sd = l->item;
        debug("Shape %s\n",sd->name);
    }
}

