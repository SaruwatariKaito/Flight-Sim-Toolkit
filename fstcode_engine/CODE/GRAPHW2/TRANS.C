/* 3D Graphics transforms */

#include <math.h>
#include <stdlib.h>

#include "graph.h"

int pshift = 8;
int zoom_shift = 0;

#define UNIT (1 << SSHIFT)
static Transdata identity = {UNIT,0,0, 0,UNIT,0, 0,0,UNIT, 0L,0L,0L};

#define MAX_MATRIX 10
static Transdata matrix_stack[MAX_MATRIX];
static int matrix_index = 0;
Transdata *matrix = &matrix_stack[0];

void zero_matrix(void)
{
    *matrix = identity;
}

void reset_matrix(void)
{
    matrix = &matrix_stack[0];
	zero_matrix();
}

void push_matrix(void)
{
	matrix_index++;
    matrix++;
    if (matrix_index >= MAX_MATRIX)
        stop("Matrix stack overflow");
    matrix_stack[matrix_index] = matrix_stack[matrix_index - 1];
}

void pop_matrix(void)
{
    if (matrix_index <= 0)
        stop("Matrix stack underflow");
    matrix_index--;
    matrix--;
}

void scale_matrix(int shift)
{
	if (shift >= 0) {
		matrix->j <<= shift;
		matrix->k <<= shift;
		matrix->l <<= shift;
	} else {
		shift = -shift;
		matrix->j >>= shift;
		matrix->k >>= shift;
		matrix->l >>= shift;
	}
}

void translate(int32 x, int32 y, int32 z)
{
	matrix->j += (x*matrix->a) + (y*matrix->d) + (z*matrix->g);
    matrix->k += (x*matrix->b) + (y*matrix->e) + (z*matrix->h);
    matrix->l += (x*matrix->c) + (y*matrix->f) + (z*matrix->i);
}

void translate_abs(int32 x, int32 y, int32 z)
{
    matrix->j = MUL(x,matrix->a) + MUL(y,matrix->d) + MUL(z,matrix->g);
    matrix->k = MUL(x,matrix->b) + MUL(y,matrix->e) + MUL(z,matrix->h);
    matrix->l = MUL(x,matrix->c) + MUL(y,matrix->f) + MUL(z,matrix->i);
}

void rotx(int angle)
{
    register int ca, sa;
    register int t;

	if (angle != 0) {
		ca = COS(angle); sa = SIN(angle);
        t = (int)((MUL(ca,matrix->d) + MUL(sa,matrix->g)) >> SSHIFT);
        matrix->g = (int)((MUL(ca,matrix->g) - MUL(sa,matrix->d)) >> SSHIFT);
    	matrix->d = t;
        t = (int)((MUL(ca,matrix->e) + MUL(sa,matrix->h)) >> SSHIFT);
        matrix->h = (int)((MUL(ca,matrix->h) - MUL(sa,matrix->e)) >> SSHIFT);
    	matrix->e = t;
        t = (int)((MUL(ca,matrix->f) + MUL(sa,matrix->i)) >> SSHIFT);
        matrix->i = (int)((MUL(ca,matrix->i) - MUL(sa,matrix->f)) >> SSHIFT);
    	matrix->f = t;
	}
}

void roty(int angle)
{
    register int ca, sa;
    register int t;

	if (angle != 0) {
    	ca = COS(angle); sa = SIN(angle);
    	t = (int)((MUL(ca,matrix->a) - MUL(sa,matrix->g)) >> SSHIFT);
    	matrix->g = (int)((MUL(sa,matrix->a) + MUL(ca,matrix->g)) >> SSHIFT);
    	matrix->a = t;
    	t = (int)((MUL(ca,matrix->b) - MUL(sa,matrix->h)) >> SSHIFT);
    	matrix->h = (int)((MUL(sa,matrix->b) + MUL(ca,matrix->h)) >> SSHIFT);
    	matrix->b = t;
    	t = (int)((MUL(ca,matrix->c) - MUL(sa,matrix->i)) >> SSHIFT);
    	matrix->i = (int)((MUL(sa,matrix->c) + MUL(ca,matrix->i)) >> SSHIFT);
    	matrix->c = t;
	}
}

void rotz(int angle)
{
    register int ca, sa;
    register int t;

	if (angle != 0) {
    	ca = COS(angle); sa = SIN(angle);
        t = (int)((MUL(ca,matrix->a) + MUL(sa,matrix->d)) >> SSHIFT);
        matrix->d = (int)((MUL(ca,matrix->d) - MUL(sa,matrix->a)) >> SSHIFT);
    	matrix->a = t;
        t = (int)((MUL(ca,matrix->b) + MUL(sa,matrix->e)) >> SSHIFT);
        matrix->e = (int)((MUL(ca,matrix->e) - MUL(sa,matrix->b)) >> SSHIFT);
    	matrix->b = t;
        t = (int)((MUL(ca,matrix->c) + MUL(sa,matrix->f)) >> SSHIFT);
        matrix->f = (int)((MUL(ca,matrix->f) - MUL(sa,matrix->c)) >> SSHIFT);
    	matrix->c = t;
	}
}

void rotate(sVector *v1, sVector *v2)
{
    sVector v;

    v = *v1;
    v2->x = (v.x*matrix->a + v.y*matrix->d + v.z*matrix->g) >> SSHIFT;
    v2->y = (v.x*matrix->b + v.y*matrix->e + v.z*matrix->h) >> SSHIFT;
    v2->z = (v.x*matrix->c + v.y*matrix->f + v.z*matrix->i) >> SSHIFT;
}

void invrotate(sVector *v1, sVector *v2)
{
    sVector v;

    v = *v1;
    v2->x = (v.x*matrix->a + v.y*matrix->b + v.z*matrix->c) >> SSHIFT;
    v2->y = (v.x*matrix->d + v.y*matrix->e + v.z*matrix->f) >> SSHIFT;
    v2->z = (v.x*matrix->g + v.y*matrix->h + v.z*matrix->i) >> SSHIFT;
}

void vertex_rotate(Vertex *v1, Vertex *v2)
{
    Vertex v;

    v.x = v1->x;
    v.y = v1->y;
    v.z = v1->z;
    v2->x = (int)(MUL(v.x,matrix->a) + MUL(v.y,matrix->d) + MUL(v.z,matrix->g) >> SSHIFT);
    v2->y = (int)(MUL(v.x,matrix->b) + MUL(v.y,matrix->e) + MUL(v.z,matrix->h) >> SSHIFT);
    v2->z = (int)(MUL(v.x,matrix->c) + MUL(v.y,matrix->f) + MUL(v.z,matrix->i) >> SSHIFT);
}

void transform(sVector *v1, sVector *v2)
{
    sVector v;

    v = *v1;
    v2->x = ((v.x*matrix->a + v.y*matrix->d + v.z*matrix->g) + matrix->j) >> SSHIFT;
    v2->y = ((v.x*matrix->b + v.y*matrix->e + v.z*matrix->h) + matrix->k) >> SSHIFT;
    v2->z = ((v.x*matrix->c + v.y*matrix->f + v.z*matrix->i) + matrix->l) >> SSHIFT;
}

void invtransform(sVector *v1, sVector *v2)
{
    sVector v;

    v = *v1;
    v2->x = ((v.x*matrix->a + v.y*matrix->b + v.z*matrix->c) + matrix->j) >> SSHIFT;
    v2->y = ((v.x*matrix->d + v.y*matrix->e + v.z*matrix->f) + matrix->k) >> SSHIFT;
    v2->z = ((v.x*matrix->g + v.y*matrix->h + v.z*matrix->i) + matrix->l) >> SSHIFT;
}

/* Leaves coords left shifted SSHIFT */

void trans_point(sVector *v1, sVector *v2)
{
    sVector v;

    v = *v1;
    v2->x = ((v.x*matrix->a + v.y*matrix->d + v.z*matrix->g) + matrix->j);
    v2->y = ((v.x*matrix->b + v.y*matrix->e + v.z*matrix->h) + matrix->k);
    v2->z = ((v.x*matrix->c + v.y*matrix->f + v.z*matrix->i) + matrix->l);
}

int fast_perspect(sVector *v1, sVector2 *v2)
{
    int32 zf;
	sVector v;
    sVector s;

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
    return (int)(s.z >> (SSHIFT + (pshift - 8)));
}

int32 perspect(sVector *v1, sVector2 *v2)
{
    int32 zf;
    sVector v, s;

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
    return (s.z >> (SSHIFT + (pshift - 8)));
}

void ac_axis_rotate(sVector *r, sVector *dr)
{
    int rr, pr, yr;
    int tanrx, srz, crz, crx;
    int pitch;

    srz = SIN(r->z); crz = COS(r->z); crx = COS(r->x);
    pitch = ABS((short)r->x);
    pr = (int)TRIG_SHIFT((MUL(dr->x, crz) + MUL(dr->y, srz)));
    if (pitch <= DEG(89)) {
        tanrx = TAN(r->x);
        rr = dr->z + (int)TRIG_SHIFT(MUL((int)TRIG_SHIFT(MUL(dr->x, tanrx)), srz)) -
                     (int)TRIG_SHIFT(MUL((int)TRIG_SHIFT(MUL(dr->y, tanrx)), crz));
        if (crx != 0)
            yr = MULDIV(dr->y, crz, crx) - MULDIV(dr->x, srz, crx);
        else
            yr = 0;
    } else {
        rr = dr->z - (dr->y * 7);
        yr = 0;
    }
    r->x = ADDANGLE(r->x, pr);
    r->y = ADDANGLE(r->y, yr);
    r->z = ADDANGLE(r->z, rr);
    pitch = (short)r->x;
    if (ABS(pitch) > DEG_90) {
		if (pitch > 0)
        	r->x = DEG_90 - (pitch - DEG_90);
		else
        	r->x = -DEG_90 - (pitch + DEG_90);
        r->y = ADDANGLE(r->y, DEG_180);
        r->z = ADDANGLE(r->z, DEG_180);
    }
}

void zoom_view(int n)
{
	switch(n) {
		case 1: pshift = 8; zoom_shift = 0; break;
		case 2: pshift = 9; zoom_shift = 1; break;
		case 4: pshift = 10; zoom_shift = 2; break;
		case 8: pshift = 11; zoom_shift = 3; break;
		case 16: pshift = 12; zoom_shift = 4; break;
		case 32: pshift = 13; zoom_shift = 5; break;
		default: pshift = 8; zoom_shift = 0; break;
	}
}

void setup_trans(sVector *r, Transdata *t)
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

void rot3d(sVector *v1, sVector *v2, Transdata *t)
{
    int32 x, y, z;

    x = v1->x; y = v1->y; z = v1->z;
    v2->x = (x*t->a + y*t->b + z*t->c) >> SSHIFT;
    v2->y = (x*t->d + y*t->e + z*t->f) >> SSHIFT;
    v2->z = (x*t->g + y*t->h + z*t->i) >> SSHIFT;
}

void invrot3d(sVector *v1, sVector *v2, Transdata *t)
{
    int32 x, y, z;

    x = v1->x; y = v1->y; z = v1->z;
    v2->x = (x*t->a + y*t->d + z*t->g) >> SSHIFT;
    v2->y = (x*t->b + y*t->e + z*t->h) >> SSHIFT;
    v2->z = (x*t->c + y*t->f + z*t->i) >> SSHIFT;
}

int32 magnitude(sVector *v)
{
	sVector av;

	av.x = ABS(v->x);
	av.y = ABS(v->y);
	av.z = ABS(v->z);
	if (av.x >= av.y) {
		if (av.x > av.z)
			return(av.x + (av.z >> 2) + (av.y >> 2));
		else
			return(av.z + (av.x >> 2) + (av.y >> 2));
	} else {
		if (av.y > av.z)
			return(av.y + (av.z >> 2) + (av.x >> 2));
		else
			return(av.z + (av.y >> 2) + (av.x >> 2));
	}
}

int magnitude2(sVector2 *v)
{
	sVector2 av;

	av.x = ABS(v->x);
	av.y = ABS(v->y);
	if (av.x >= av.y) {
		return((av.x + av.y) - (av.y >> 1));
	}
	return((av.x + av.y) - (av.x >> 1));
}

void normalise(sVector *v)
{
	int32 size;
	sVector av;

	av.x = ABS(v->x);
	av.y = ABS(v->y);
	av.z = ABS(v->z);
	if (av.x >= av.y) {
		if (av.x > av.z)
			size = av.x + (av.z >> 2) + (av.y >> 2);
		else
			size = av.z + (av.x >> 2) + (av.y >> 2);
	} else {
		if (av.y > av.z)
			size = av.y + (av.z >> 2) + (av.x >> 2);
		else
			size = av.z + (av.y >> 2) + (av.x >> 2);
	}
	if (size > 0) {
		v->x = (v->x << SSHIFT) / size;
		v->y = (v->y << SSHIFT) / size;
		v->z = (v->z << SSHIFT) / size;
	}
}

void poly_normal(Vertex *p1, Vertex *p2, Vertex *p3, sVector *v)
{
    sVector a, b;

    a.x = p3->x - p1->x;
    a.y = p3->y - p1->y;
    a.z = p3->z - p1->z;
	normalise(&a);
    b.x = p2->x - p1->x;
    b.y = p2->y - p1->y;
    b.z = p2->z - p1->z;
	normalise(&b);
    v->x = (int)((MUL(a.y, b.z) - MUL(a.z, b.y)) >> SSHIFT);
    v->y = (int)((MUL(a.z, b.x) - MUL(a.x, b.z)) >> SSHIFT);
    v->z = (int)((MUL(a.x, b.y) - MUL(a.y, b.x)) >> SSHIFT);
	normalise(v);
}

int vector_azimuth(sVector *v)
{
    return ((short)fast_atan(-v->x, v->z));
}

int vector_elevation(sVector *v)
{
	int32 avx, avz;

	avx = ABS(v->x);
	avz = ABS(v->z);
	if (avx >= avz) {
		return((short)fast_atan(v->y, (avx + avz) - (avx >> 1)));
	} else {
		return((short)fast_atan(v->y, (avx + avz) - (avz >> 1)));
	}
}

