/* 3D collision primitives */

#include <math.h>
#include <stdlib.h>

#include "graph.h"

#define CSHIFT SSHIFT

static sVector tpts[MAX_SHAPE_PTS + 64];
static sVector *tp = tpts;
static sVector *zpts[MAX_SHAPE_PTS], *xpts[MAX_SHAPE_PTS];

static Collide_box box;

/*static int x_clip1(sVector **pp1, sVector **pp2)
{
    register sVector *p1, *p2;

	p1 = *pp1;
	p2 = *pp2;
	if ((p1->x >= box.left) && (p1->x <= box.right))
        return 1;
	if ((p2->x >= box.left) && (p2->x <= box.right))
        return 1;
	return 0;
}

static int y_clip1(sVector **pp1, sVector **pp2)
{
    register sVector *p1, *p2;

	p1 = *pp1;
	p2 = *pp2;
	if ((p1->y >= box.bottom) && (p1->y <= box.top))
        return 1;
	if ((p2->y >= box.bottom) && (p2->y <= box.top))
        return 1;
	return 0;
}

static int z_clip1(sVector **pp1, sVector **pp2)
{
    register sVector *p1, *p2;

	p1 = *pp1;
	p2 = *pp2;
	if ((p1->z >= box.back) && (p1->z <= box.front))
        return 1;
	if ((p2->z >= box.back) && (p2->z <= box.front))
        return 1;
	return 0;
}*/

static int x_clip(sVector **pp1, sVector **pp2)
{
    register sVector *p1, *p2;
	register int i;
    sVector n1, n2, n3;

	p1 = *pp1;
	p2 = *pp2;
    if (p2->x >= p1->x) {
	    if (p1->x > box.right)
            return 0;
	    if (p2->x < box.left)
            return 0;
        if (p1->x < box.left) {
    		n1 = *p1;
    		n2 = *p2;
			for (i = 0; (n1.x < box.left) && (i < 16); i++) {
        		n3.x = (n1.x + n2.x) >> 1;
        		n3.y = (n1.y + n2.y) >> 1;
        		n3.z = (n1.z + n2.z) >> 1;
        		if (n3.x <= box.left)
            		n1 = n3;
        		else
            		n2 = n3;
    		}
			p1 = ++tp;
    		*p1 = n1;
    		p1->x = box.left;
			*pp1 = p1;
		}
        if (p2->x <= box.right)
            return 1;
    	n1 = *p1;
    	n2 = *p2;
		for (i = 0; (n2.x > box.right) && (i < 16); i++) {
        	n3.x = (n1.x + n2.x) >> 1;
        	n3.y = (n1.y + n2.y) >> 1;
        	n3.z = (n1.z + n2.z) >> 1;
        	if (n3.x < box.right)
            	n1 = n3;
        	else
            	n2 = n3;
    	}
		p2 = ++tp;
    	*p2 = n2;
    	p2->x = box.right;
		*pp2 = p2;
    	return 2;
    }
	n1.x = x_clip(pp2, pp1);
	if (n1.x == 1)
		return 2;
	return n1.x;
}

static int y_clip(sVector **pp1, sVector **pp2)
{
    register sVector *p1, *p2;
	register int i;
    sVector n1, n2, n3;

	p1 = *pp1;
	p2 = *pp2;
    if (p2->y >= p1->y) {
	    if (p1->y > box.top)
            return 0;
	    if (p2->y < box.bottom)
            return 0;
        if (p1->y < box.bottom) {
    		n1 = *p1;
    		n2 = *p2;
			for (i = 0; (n1.y < box.bottom) && (i < 16); i++) {
        		n3.x = (n1.x + n2.x) >> 1;
        		n3.y = (n1.y + n2.y) >> 1;
        		n3.z = (n1.z + n2.z) >> 1;
        		if (n3.y <= box.bottom)
            		n1 = n3;
        		else
            		n2 = n3;
    		}
			p1 = ++tp;
    		*p1 = n1;
    		p1->y = box.bottom;
			*pp1 = p1;
		}
        if (p2->y <= box.top)
            return 1;
    	n1 = *p1;
    	n2 = *p2;
		for (i = 0; (n2.y > box.top) && (i < 16); i++) {
        	n3.x = (n1.x + n2.x) >> 1;
        	n3.y = (n1.y + n2.y) >> 1;
        	n3.z = (n1.z + n2.z) >> 1;
        	if (n3.y < box.top)
            	n1 = n3;
        	else
            	n2 = n3;
    	}
		p2 = ++tp;
    	*p2 = n2;
    	p2->y = box.top;
		*pp2 = p2;
    	return 2;
    }
	n1.y = y_clip(pp2, pp1);
	if (n1.y == 1)
		return 2;
	return n1.y;
}

static int z_clip(sVector **pp1, sVector **pp2)
{
    register sVector *p1, *p2;
	register int i;
    sVector n1, n2, n3;

	p1 = *pp1;
	p2 = *pp2;
    if (p2->z >= p1->z) {
	    if (p1->z > box.front)
            return 0;
	    if (p2->z < box.back)
            return 0;
        if (p1->z < box.back) {
    		n1 = *p1;
    		n2 = *p2;
			for (i = 0; (n1.z < box.back) && (i < 16); i++) {
        		n3.x = (n1.x + n2.x) >> 1;
        		n3.y = (n1.y + n2.y) >> 1;
        		n3.z = (n1.z + n2.z) >> 1;
        		if (n3.z <= box.back)
            		n1 = n3;
        		else
            		n2 = n3;
    		}
			p1 = ++tp;
    		*p1 = n1;
    		p1->z = box.back;
			*pp1 = p1;
		}
        if (p2->z <= box.front)
            return 1;
    	n1 = *p1;
    	n2 = *p2;
		for (i = 0; (n2.z > box.front) && (i < 16); i++) {
        	n3.x = (n1.x + n2.x) >> 1;
        	n3.y = (n1.y + n2.y) >> 1;
        	n3.z = (n1.z + n2.z) >> 1;
        	if (n3.z < box.front)
            	n1 = n3;
        	else
            	n2 = n3;
    	}
		p2 = ++tp;
    	*p2 = n2;
    	p2->z = box.front;
		*pp2 = p2;
    	return 2;
    }
	n1.z = z_clip(pp2, pp1);
	if (n1.z == 1)
		return 2;
	return n1.z;
}

static int hit_line(sVector *s1, sVector *s2)
{
    if ((z_clip(&s1, &s2) > 0) &&
        	(x_clip(&s1, &s2) > 0) &&
        	(y_clip(&s1, &s2) > 0)) {
		return TRUE;
	}
	return FALSE;
}

static int hit_polygons(int npolys, sPolygon **polys)
{
    int i, p, c, n, hit;
	int npts;
	Pindex *points;
    sPolygon *poly;
    sVector *p1, *p2, *np;

	hit = FALSE;
    for (p = 0; !hit && (p < npolys); p++) {
        poly = polys[p];
        points = poly->points;
        npts = poly->npoints;
        if (npts == 2) {
            p1 = &tpts[points[0]];
            p2 = &tpts[points[1]];
/*			vector_number(p1->x, 20, 10 + (p * 6), 0x63);
			vector_number(p1->y, 50, 10 + (p * 6), 0x63);
			vector_number(p1->z, 80, 10 + (p * 6), 0x63);
			hit = x_clip(&p1, &p2);
			vector_number(p1->x, 120, 10 + (p * 6), 0x63);
			vector_number(p1->y, 150, 10 + (p * 6), 0x63);
			vector_number(p1->z, 180, 10 + (p * 6), 0x63);
			hit = hit && z_clip(&p1, &p2);
			vector_number(p1->x, 220, 10 + (p * 6), 0x63);
			vector_number(p1->y, 250, 10 + (p * 6), 0x63);
			vector_number(p1->z, 280, 10 + (p * 6), 0x63);
			hit = hit && y_clip(&p1, &p2);*/
    		if ((x_clip(&p1, &p2) > 0) &&
        			(y_clip(&p1, &p2) > 0) &&
        			(z_clip(&p1, &p2) > 0)) {
				hit = TRUE;
			}
        } else {
            np = &tpts[points[npts - 1]];
            for (i = 0, n = 0; i < npts; i++) {
                p1 = np;
                p2 = np = &tpts[points[i]];
                c = x_clip(&p1, &p2);
                if (c > 0) {
                    xpts[n++] = p1;
                    if (c > 1) {
						xpts[n++] = p2;
                    }
                }
            }
            npts = n;
            np = xpts[npts - 1];
            for (i = 0, n = 0; i < npts; i++) {
                p1 = np;
                p2 = np = xpts[i];
                c = y_clip(&p1, &p2);
                if (c > 0) {
                    zpts[n++] = p1;
                    if (c > 1) {
						zpts[n++] = p2;
                    }
                }
            }
            npts = n;
            np = zpts[npts - 1];
            for (i = 0, n = 0; i < npts; i++) {
                p1 = np;
                p2 = np = zpts[i];
                c = z_clip(&p1, &p2);
                if (c > 0) {
                    xpts[n++] = p1;
                    if (c > 1) {
                        if (p2->z != np->z) {
                            xpts[n++] = p2;
                        }
                    }
                }
            }
            if (n > 2) {
				hit = TRUE;
            }
        }
    }
	return hit;
}

int collide3(sShape *shape, Collide_box *b)
{
    register int i, hit;
    int npts;
    register Vertex *pts;
	sVector v;

    npts = shape->npts;
    if (npts > MAX_SHAPE_PTS)
        stop("Too many points in shape in collide3");
	box = *b;
	if (box.left == 0)
		box.left = -1;
	if (box.bottom == 0)
		box.bottom = -1;
	if (box.back == 0)
		box.back = -1;
	for (i = npts, pts = shape->pts, tp = tpts; i; i--, pts++, tp++) {
    	v.x = (((int)pts->x*matrix->a + (int)pts->y*matrix->d + (int)pts->z*matrix->g) + matrix->j);
    	v.y = (((int)pts->x*matrix->b + (int)pts->y*matrix->e + (int)pts->z*matrix->h) + matrix->k);
    	v.z = (((int)pts->x*matrix->c + (int)pts->y*matrix->f + (int)pts->z*matrix->i) + matrix->l);
		tp->x = (int)(v.x >> SSHIFT);
		tp->y = (int)(v.y >> SSHIFT);
		tp->z = (int)(v.z >> SSHIFT);
	}
	hit = FALSE;
    if (shape->npolygons > 0) {
        hit = hit_polygons(shape->npolygons, shape->polygons);
    }
	return hit;
}

