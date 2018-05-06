//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
// GRAPH3.C -- 'FST-98' Terrain Viewer
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Windows 3d graphics functions
//
// Changes:
//
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

#include <math.h>
#include <stdlib.h>

#include "graph.h"
#include "defs.h"

int xcentre = 160, ycentre = 100;
int clockwise_test;

Vector  zero  = {0, 0, 0};
Svector szero = {0, 0, 0};

int cols[256];

#define VDIST 24L
#define SVDIST (VDIST << SSHIFT)
#define ZSHIFT SSHIFT

static Vector2 spts[MAX_SHAPE_PTS];

//static Vector tpts[MAX_SHAPE_PTS + 64];
//static Vector *tp = tpts;
//static Vector *zpts[MAX_SHAPE_PTS], *xzpts[MAX_SHAPE_PTS];

int pshift = 8;

#define UNIT (1 << SSHIFT)
static Transdata identity = {UNIT,0,0, 0,UNIT,0, 0,0,UNIT, 0L,0L,0L};

#define MAX_MATRIX 10
static Transdata matrix_stack[MAX_MATRIX];
static int matrix_index = 0;
Transdata *matrix = &matrix_stack[0];

int sun_size = 35;
Svector sun_position = {0, 1 << SSHIFT, 0};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void set_sun_angle(int rx, int ry)
{
	Svector v, r;
	Transdata trans;

	v.x = 0;
	v.y = 0;
	v.z = 1 << SSHIFT;
	r.x = rx;
	r.y = ry;
	r.z = 0;
	setup_trans(&r, &trans);
	s_rot3d(&v, &sun_position, &trans);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void zero_matrix(void)
{
	*matrix = identity;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void reset_matrix(void)
{
	matrix = &matrix_stack[0];
	zero_matrix();
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void push_matrix(void)
{
	matrix_index++;
	matrix++;
	if (matrix_index >= MAX_MATRIX) {
		stop("Matrix stack overflow");
     }
	matrix_stack[matrix_index] = matrix_stack[matrix_index - 1];
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void pop_matrix(void)
{
    if (matrix_index <= 0) {
        stop("Matrix stack underflow");
    }
    matrix_index--;
    matrix--;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void scale_matrix(int shift)
{
	if (shift >= 0) {
		matrix->j <<= shift;
		matrix->k <<= shift;
		matrix->l <<= shift;
	}
     else {
		shift = -shift;
		matrix->j >>= shift;
		matrix->k >>= shift;
		matrix->l >>= shift;
	}
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void translate(int32 x, int32 y, int32 z)
{
	matrix->j += (x*matrix->a) + (y*matrix->d) + (z*matrix->g);
	matrix->k += (x*matrix->b) + (y*matrix->e) + (z*matrix->h);
	matrix->l += (x*matrix->c) + (y*matrix->f) + (z*matrix->i);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void rotx(int angle)
{
	register int cra, sra;
	register int d, e, f, g, h, i;

	if (angle != 0) {
		cra = COS(angle); sra = SIN(angle);
		d = (int)((MUL(cra,matrix->d) + MUL(sra,matrix->g)) >> SSHIFT);
		e = (int)((MUL(cra,matrix->e) + MUL(sra,matrix->h)) >> SSHIFT);
		f = (int)((MUL(cra,matrix->f) + MUL(sra,matrix->i)) >> SSHIFT);
		g = (int)((MUL(cra,matrix->g) - MUL(sra,matrix->d)) >> SSHIFT);
		h = (int)((MUL(cra,matrix->h) - MUL(sra,matrix->e)) >> SSHIFT);
		i = (int)((MUL(cra,matrix->i) - MUL(sra,matrix->f)) >> SSHIFT);
		matrix->d = d;
		matrix->e = e;
		matrix->f = f;
		matrix->g = g;
		matrix->h = h;
		matrix->i = i;
	}
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void roty(int angle)
{
	register int cra, sra;
	register int a, b, c, g, h, i;

	if (angle != 0) {
		cra = COS(angle); sra = SIN(angle);
		a = (int)((MUL(cra,matrix->a) - MUL(sra,matrix->g)) >> SSHIFT);
		b = (int)((MUL(cra,matrix->b) - MUL(sra,matrix->h)) >> SSHIFT);
		c = (int)((MUL(cra,matrix->c) - MUL(sra,matrix->i)) >> SSHIFT);
		g = (int)((MUL(sra,matrix->a) + MUL(cra,matrix->g)) >> SSHIFT);
		h = (int)((MUL(sra,matrix->b) + MUL(cra,matrix->h)) >> SSHIFT);
		i = (int)((MUL(sra,matrix->c) + MUL(cra,matrix->i)) >> SSHIFT);
		matrix->a = a;
		matrix->b = b;
		matrix->c = c;
		matrix->g = g;
		matrix->h = h;
		matrix->i = i;
	}
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void rotz(int angle)
{
	register int cra, sra;
	register int a, b, c, d, e, f;

	if (angle != 0) {
		cra = COS(angle); sra = SIN(angle);
		a = (int)((MUL(cra,matrix->a) + MUL(sra,matrix->d)) >> SSHIFT);
		b = (int)((MUL(cra,matrix->b) + MUL(sra,matrix->e)) >> SSHIFT);
		c = (int)((MUL(cra,matrix->c) + MUL(sra,matrix->f)) >> SSHIFT);
		d = (int)((MUL(cra,matrix->d) - MUL(sra,matrix->a)) >> SSHIFT);
		e = (int)((MUL(cra,matrix->e) - MUL(sra,matrix->b)) >> SSHIFT);
		f = (int)((MUL(cra,matrix->f) - MUL(sra,matrix->c)) >> SSHIFT);
		matrix->a = a;
		matrix->b = b;
		matrix->c = c;
		matrix->d = d;
		matrix->e = e;
		matrix->f = f;
	}
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void rotate(Vector *v1, Vector *v2)
{
	Vector v;

	v = *v1;
	v2->x = (v.x*matrix->a + v.y*matrix->d + v.z*matrix->g) >> SSHIFT;
	v2->y = (v.x*matrix->b + v.y*matrix->e + v.z*matrix->h) >> SSHIFT;
	v2->z = (v.x*matrix->c + v.y*matrix->f + v.z*matrix->i) >> SSHIFT;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void invrotate(Vector *v1, Vector *v2)
{
	Vector v;

	v = *v1;
	v2->x = (v.x*matrix->a + v.y*matrix->b + v.z*matrix->c) >> SSHIFT;
	v2->y = (v.x*matrix->d + v.y*matrix->e + v.z*matrix->f) >> SSHIFT;
	v2->z = (v.x*matrix->g + v.y*matrix->h + v.z*matrix->i) >> SSHIFT;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void coord_rotate(Scoord *v1, Scoord *v2)
{
	Scoord v;

	v = *v1;
	v2->x = (int)((MUL(v.x,matrix->a) + MUL(v.y,matrix->d) + MUL(v.z,matrix->g)) >> SSHIFT);
	v2->y = (int)((MUL(v.x,matrix->b) + MUL(v.y,matrix->e) + MUL(v.z,matrix->h)) >> SSHIFT);
	v2->z = (int)((MUL(v.x,matrix->c) + MUL(v.y,matrix->f) + MUL(v.z,matrix->i)) >> SSHIFT);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void invrot3d(Vector *v1, Vector *v2, Transdata *t)
{
	int32 x, y, z;

	x = v1->x; y = v1->y; z = v1->z;
	v2->x = (x*t->a + y*t->d + z*t->g) >> SSHIFT;
	v2->y = (x*t->b + y*t->e + z*t->h) >> SSHIFT;
	v2->z = (x*t->c + y*t->f + z*t->i) >> SSHIFT;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void s_rot3d(Svector *v1, Svector *v2, Transdata *t)
{
	int x, y, z;

	x = v1->x; y = v1->y; z = v1->z;
	v2->x = (int)((MUL(x,t->a) + MUL(y,t->b) + MUL(z,t->c)) >> SSHIFT);
	v2->y = (int)((MUL(x,t->d) + MUL(y,t->e) + MUL(z,t->f)) >> SSHIFT);
	v2->z = (int)((MUL(x,t->g) + MUL(y,t->h) + MUL(z,t->i)) >> SSHIFT);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void setup_trans(Svector *r, Transdata *t)
{
	int crx, cry, crz, srx, sry, srz;

	crx = COS(r->x); cry = COS(r->y); crz = COS(r->z);
	srx = SIN(r->x); sry = SIN(r->y); srz = SIN(r->z);
	t->a = (int)((MUL(cry,crz) - MUL((int)(MUL(sry,srx) >> SSHIFT),srz)) >> SSHIFT);
	t->b = (int)((MUL(cry,srz) + MUL((int)(MUL(sry,srx) >> SSHIFT),crz)) >> SSHIFT);
	t->c = (int)(MUL(-sry,crx) >> SSHIFT);
	t->d = (int)(MUL(-crx,srz) >> SSHIFT);
	t->e = (int)(MUL(crx,crz) >> SSHIFT);
	t->f = srx;
	t->g = (int)((MUL(sry,crz) + MUL((int)(MUL(cry,srx) >> SSHIFT),srz)) >> SSHIFT);
	t->h = (int)((MUL(sry,srz) - MUL((int)(MUL(cry,srx) >> SSHIFT),crz)) >> SSHIFT);
	t->i = (int)(MUL(cry,crx) >> SSHIFT);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void s_normalise(Svector *v)
{
	int32 size;
	Svector av;

	av.x = ABS(v->x);
	av.y = ABS(v->y);
	av.z = ABS(v->z);
	if (av.x >= av.y) {
		if (av.x > av.z) {
			size = av.x + (av.z >> 2) + (av.y >> 2);
          }
		else {
			size = av.z + (av.x >> 2) + (av.y >> 2);
          }
	}
     else {
		if (av.y > av.z) {
			size = av.y + (av.z >> 2) + (av.x >> 2);
          }
		else {
			size = av.z + (av.y >> 2) + (av.x >> 2);
          }
	}
	if (size > 0) {
		v->x = (int)DIV((int32)v->x << SSHIFT, size);
		v->y = (int)DIV((int32)v->y << SSHIFT, size);
		v->z = (int)DIV((int32)v->z << SSHIFT, size);
	}
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void poly_normal(Scoord *p1, Scoord *p2, Scoord *p3, Svector *v)
{
	Svector a, b;

	a.x = p3->x - p1->x;
	a.y = p3->y - p1->y;
	a.z = p3->z - p1->z;
	s_normalise(&a);
	b.x = p2->x - p1->x;
	b.y = p2->y - p1->y;
	b.z = p2->z - p1->z;
	s_normalise(&b);
	v->x = (int)((MUL(a.y, b.z) - MUL(a.z, b.y)) >> SSHIFT);
	v->y = (int)((MUL(a.z, b.x) - MUL(a.x, b.z)) >> SSHIFT);
	v->z = (int)((MUL(a.x, b.y) - MUL(a.y, b.x)) >> SSHIFT);
	s_normalise(v);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Leaves coords left shifted SSHIFT
void trans_point(Vector *v1, Vector *v2)
{
	Vector v;

	v = *v1;
	v2->x = ((v.x*matrix->a + v.y*matrix->d + v.z*matrix->g) + matrix->j);
	v2->y = ((v.x*matrix->b + v.y*matrix->e + v.z*matrix->h) + matrix->k);
	v2->z = ((v.x*matrix->c + v.y*matrix->f + v.z*matrix->i) + matrix->l);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void trans_sub(Svector *v1, Vector *v2)
{
	Svector v;

	v = *v1;
	v2->x = MUL(v.x,matrix->a) + MUL(v.y,matrix->d) + MUL(v.z,matrix->g) + matrix->j;
	v2->y = MUL(v.x,matrix->b) + MUL(v.y,matrix->e) + MUL(v.z,matrix->h) + matrix->k;
	v2->z = MUL(v.x,matrix->c) + MUL(v.y,matrix->f) + MUL(v.z,matrix->i) + matrix->l;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
int fast_perspect(Svector *v1, Vector2 *v2)
{
	int32 zf;
	Svector v;
	Vector s;

	v = *v1;
	s.x = MUL(v.x,matrix->a) + MUL(v.y,matrix->d) + MUL(v.z,matrix->g);
	s.y = MUL(v.x,matrix->b) + MUL(v.y,matrix->e) + MUL(v.z,matrix->h);
	s.z = MUL(v.x,matrix->c) + MUL(v.y,matrix->f) + MUL(v.z,matrix->i);
	zf = s.z >> pshift;
	if (zf > 0) {
		v2->x = xcentre + (int)(s.x / zf);
		v2->y = ycentre - (int)(s.y / zf);
	}
     else {
		v2->x = xcentre + (int)s.x;
		v2->y = ycentre - (int)s.y;
     }
	return (int)(s.z >> SSHIFT);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
int32 perspect(Vector *v1, Vector2 *v2)
{
	int32 zf;
	Vector v, s;

	v = *v1;
	s.x = ((v.x*matrix->a + v.y*matrix->d + v.z*matrix->g) + matrix->j);
	s.y = ((v.x*matrix->b + v.y*matrix->e + v.z*matrix->h) + matrix->k);
	s.z = ((v.x*matrix->c + v.y*matrix->f + v.z*matrix->i) + matrix->l);
	zf = s.z >> pshift;
	if (zf > 0) {
		v2->x = xcentre + (int)(s.x / zf);
		v2->y = ycentre - (int)(s.y / zf);
	}
     else {
		v2->x = xcentre + (int)s.x;
		v2->y = ycentre - (int)s.y;
	}
	return (s.z >> SSHIFT);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void zoom_view(int n)
{
	switch(n) {

		case 1:
          	pshift = 8;
               break;

		case 2:
			pshift = 9;
               break;

		case 4:
          	pshift = 10;
               break;

		case 8:
          	pshift = 11;
               break;

		default:
          	pshift = 8;
               break;
	}
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void far_polygons(int npolys, Poly **polys, Vector2 cpts[])
{
	int i, p, noclip;
	int pt, npts;	//, pt2;
	Pindex *points;
	int sx, sy;
	int x0, x1, x2, y0, y1, y2;
	Poly *poly;
	Vector2 spts[MAX_PTS];

	for (p = 0; p < npolys; p++) {
		poly = polys[p];
		points = poly->points;
		npts = poly->npoints;
		if (npts == 2) {
			//pt = points[0];
			//pt2 = points[1];
               //if (grid_on) {
			//	draw_line(cpts[pt].x >> 2, cpts[pt].y >> 2, cpts[pt2].x >> 2, cpts[pt2].y >> 2, poly->colour);
               //}
		}
		else if (npts > 2) {
			noclip = TRUE;
			pt = points[0];
			sx = spts[0].x = (x0 = cpts[pt].x) >> 2;
			sy = spts[0].y = (y0 = cpts[pt].y) >> 2;
			if ((sx < clip_region.left) || (sx > clip_region.right) || (sy < clip_region.top) || (sy > clip_region.bottom)) {
				noclip = FALSE;
			}
			pt = points[1];
			sx = spts[1].x = (x1 = cpts[pt].x) >> 2;
			sy = spts[1].y = (y1 = cpts[pt].y) >> 2;
			if ((sx < clip_region.left) || (sx > clip_region.right) || (sy < clip_region.top) || (sy > clip_region.bottom)) {
				noclip = FALSE;
			}
			pt = points[2];
			sx = spts[2].x = (x2 = cpts[pt].x) >> 2;
			sy = spts[2].y = (y2 = cpts[pt].y) >> 2;
			if ((sx < clip_region.left) || (sx > clip_region.right) || (sy < clip_region.top) || (sy > clip_region.bottom)) {
				noclip = FALSE;
			}
			for (i = 3; i < npts; i++) {
				pt = points[i];
				sx = spts[i].x = cpts[pt].x >> 2;
				sy = spts[i].y = cpts[pt].y >> 2;
				if ((sx < clip_region.left) || (sx > clip_region.right) || (sy < clip_region.top) || (sy > clip_region.bottom)) {
					noclip = FALSE;
				}
			}
			if (clockwise_test) {
				if (MUL((x1 - x0), (y2 - y1)) > MUL((y1 - y0), (x2 - x1))) {
					if (noclip) {
						draw_polygon(npts, spts, poly->colour);
                         }
					else {
						draw_polygon(npts, spts, poly->colour);
                         }
				}
			}
               else {
				if (noclip) {
					draw_polygon(npts, spts, poly->colour);
                    }
				else {
					draw_polygon(npts, spts, poly->colour);
                    }
			}
			//draw_line(spts[npts-1].x, spts[npts-1].y, spts[0].x, spts[0].y, poly->colour);
   			//for (i = 0; i < (npts - 1); i++) {
			//	draw_line(spts[i].x, spts[i].y, spts[i+1].x, spts[i+1].y, poly->colour);
			//}
		}
	}
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void far_shape3(Shape *shape)
{
	register int i, xc, yc;
	int npts;
	int32 zf;
	register Scoord *pts;
	Vector v;
	register Vector2 *sp;

	npts = shape->npts;
	xc = xcentre << 2;
	yc = ycentre << 2;
	for (i = npts, pts = shape->pts, sp = spts; i; i--, pts++, sp++) {
		v.x = (MUL(pts->x,matrix->a) + MUL(pts->y,matrix->d) + MUL(pts->z,matrix->g)) + matrix->j;
		v.y = (MUL(pts->x,matrix->b) + MUL(pts->y,matrix->e) + MUL(pts->z,matrix->h)) + matrix->k;
		v.z = (MUL(pts->x,matrix->c) + MUL(pts->y,matrix->f) + MUL(pts->z,matrix->i)) + matrix->l;
	     zf = v.z >> (pshift + 2);
     	if (zf > 0) {
			sp->x = xc + (int)DIV(v.x, zf);
			sp->y = yc - (int)DIV(v.y, zf);
		}
          else {
			sp->x = xc + (int)v.x;
			sp->y = yc - (int)v.y;
		}
	}
	far_polygons(shape->npolygons, shape->polygons, spts);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void setup_graphics2(void)
{
	int i;

	for (i = 0; i < 256; i++) {
		cols[i] = i;
	}
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void setup_graphics3(void)
{
	reset_matrix();
	xcentre = screen_width / 2;
	ycentre = screen_height / 2;
}

