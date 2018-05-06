/* 3D Graphics primitives */

/* Full 3D clipping has been eliminated which simplifies 3D texture
 * and shading clipping.
 * 3D polygon normal backface removal has been added instead of the
 * 2D clockwise test. This is slower but reduces small polygon flicker.
 */

#include <math.h>
#include <stdlib.h>

#include "graph.h"

// Default to perspective correction on
int perspective_texturing = TRUE;

/* Useful for performance measuring */
int shape_count = 0, far_shape_count = 0, near_shape_count = 0;

int xcentre = 160, ycentre = 100;
int clockwise_test = TRUE;

int sun_size = 35;
sVector sun_position = {0, 1 << SSHIFT, 0};

#define NEAR_CLIP (64L << SSHIFT)
#define ZSHIFT SSHIFT

/* tpts holds transformed vertices for current shape */
static Cache_pt tpts[MAX_SHAPE_PTS];
/* clip_pts are used when needed for 3D clipping of vertices */
static Cache_pt clip_pts[32];
static Cache_pt *clip_pt = clip_pts;


void set_sun_angle(int rx, int ry)
{
	sVector v, r;
	Transdata trans;

	v.x = 0;
	v.y = 0;
	v.z = 1 << SSHIFT;
	r.x = rx;
	r.y = ry;
	r.z = 0;
	setup_trans(&r, &trans);
	rot3d(&v, &sun_position, &trans);
}

void draw_sun(int colour)
{
    int x, y, r;
	int32 zf;
    sVector s;

	trans_point(&sun_position, &s);
	if (s.z > NEAR_CLIP) {
    	zf = s.z >> pshift;
    	if (zf > 0) {
        	x = xcentre + (int)(s.x / zf);
        	y = ycentre - (int)(s.y / zf);
			r = (int)(((int32)sun_size << (SSHIFT + 2)) / zf);
			if (r < 100)
        		circle_f(x, y, r, colour);
    	}
    }
}

void point3(sVector *p, int colour)
{
    int x, y;
	int32 zf;
    sVector s;

    trans_point(p, &s);
    zf = s.z >> pshift;
    if (zf > 0) {
        x = xcentre + (int)(s.x / zf);
        y = ycentre - (int)(s.y / zf);
        pixel_prim(x, y, COL(colour));
    }
}

void block3(sVector *p, int colour)
{
    int x, y;
	int32 zf;
    sVector s;

    trans_point(p, &s);
    zf = s.z >> pshift;
    if (zf > 0) {
        x = xcentre + (int)(s.x / zf);
        y = ycentre - (int)(s.y / zf);
        block_prim(x, y, COL(colour));
    }
}

void sphere3(sVector *p, int radius, int colour)
{
    int x, y, r;
	int32 zf;
    sVector s;

	s.x = MUL(p->x,matrix->a) + MUL(p->y,matrix->d) + MUL(p->z,matrix->g);
    s.y = MUL(p->x,matrix->b) + MUL(p->y,matrix->e) + MUL(p->z,matrix->h);
    s.z = MUL(p->x,matrix->c) + MUL(p->y,matrix->f) + MUL(p->z,matrix->i);
	if (s.z > NEAR_CLIP) {
    	zf = s.z >> pshift;
    	if (zf > 0) {
        	x = xcentre + (int)(s.x / zf);
        	y = ycentre - (int)(s.y / zf);
			r = (int)(((int32)radius << SSHIFT) / zf);
            if (r < 1) {
				colour = COL(colour);
				hline_prim(x - 1, x + 1, y - 1, colour);
				hline_prim(x - 1, x + 1, y, colour);
				hline_prim(x - 1, x + 1, y + 1, colour);
            } else if (r < 2) {
				colour = COL(colour);
				pixel_prim(x, y - 2, colour);
				hline_prim(x - 1, x + 1, y - 1, colour);
				hline_prim(x - 2, x + 2, y, colour);
				hline_prim(x - 1, x + 1, y + 1, colour);
				pixel_prim(x, y + 2, colour);
            } else if (r < 100) {
        		circle_f(x, y, r, colour);
            }
    	}
    }
}

void smoke3(sVector *p, int radius, int colour, uchar *map)
{
    int x, y, r;
	int32 zf;
    sVector s;

    trans_point(p, &s);
	if (s.z > NEAR_CLIP) {
    	zf = s.z >> pshift;
    	if (zf > 0) {
        	x = xcentre + (int)(s.x / zf);
        	y = ycentre - (int)(s.y / zf);
			r = (int)(((int32)radius << SSHIFT) / zf);
            if (r < 2) {
                block_prim(x, y, COL(colour));
            } else if (r < 100) {
        		circle_r(x, y, r, map);
            }
    	}
    }
}

/* Checks for clockwise 2 D polygon */
int clockwise2(sVector2 *s)
{
	return(MUL(s[1].y - s[0].y , s[2].x - s[1].x) <=
    	   MUL(s[1].x - s[0].x , s[2].y - s[1].y));
}

/* Arg coords are left shifted SSHIFT */
static int z_clip_v(sVector *p1, sVector *p2)
{
    register int dz, z1;
/*    sVector n1, n2, n3;*/

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
		dz = (int)((p2->z - p1->z) >> ZSHIFT);
        z1 = (int)((NEAR_CLIP - p1->z) >> ZSHIFT);
        if (dz == 0) {
            p1->x += MUL(((p2->x - p1->x) >> ZSHIFT), z1) << ZSHIFT;
            p1->y += MUL(((p2->y - p1->y) >> ZSHIFT), z1) << ZSHIFT;
        }
        else {
            p1->x += (int32)MULDIV(((p2->x - p1->x) >> ZSHIFT), z1, dz) << ZSHIFT;
            p1->y += (int32)MULDIV(((p2->y - p1->y) >> ZSHIFT), z1, dz) << ZSHIFT;
        }
/*        if (dz == 0) {
            p1->x += (((p2->x - p1->x) >> ZSHIFT) * z1) << ZSHIFT;
            p1->y += (((p2->y - p1->y) >> ZSHIFT) * z1) << ZSHIFT;
        }
        else {
            p1->x += ((((p2->x - p1->x) >> ZSHIFT) * z1) / dz) << ZSHIFT;
            p1->y += ((((p2->y - p1->y) >> ZSHIFT) * z1) / dz) << ZSHIFT;
        }*/
/*    	n1 = *p1;
    	n2 = *p2;
    	while (n1.z < NEAR_CLIP) {
        	n3.x = (n1.x + n2.x) >> 1;
        	n3.y = (n1.y + n2.y) >> 1;
        	n3.z = (n1.z + n2.z) >> 1;
        	if (n3.z <= NEAR_CLIP)
            	n1 = n3;
        	else
            	n2 = n3;
    	}
    	*p1 = n1;*/
    	p1->z = NEAR_CLIP;
    	return 2;
    }
	return z_clip_v(p2, p1);
}

static int z_clip(Cache_pt **pp1, Cache_pt **pp2)
{
/*    register int dz, z1;*/
    register Cache_pt *p1, *p2;
    sVector n1, n2, n3;

	p1 = *pp1;
	p2 = *pp2;
	if (p2->z >= p1->z) {
        if (p1->z >= NEAR_CLIP)
            return 1;
        if (p2->z < NEAR_CLIP)
            return 0;
        if (p2->z == NEAR_CLIP) {
			p1 = clip_pt++;
            p1->x = p2->x;
            p1->y = p2->y;
        	p1->z = NEAR_CLIP;
			*pp1 = p1;
        	return 1;
        }
/*		dz = (int)((p2->z - p1->z) >> ZSHIFT);
        z1 = (int)((NEAR_CLIP - p1->z) >> ZSHIFT);
		p1 = clip_pt++;
        if (dz == 0) {
            p1->x += MUL(((p2->x - (*pp1)->x) >> ZSHIFT), z1) << ZSHIFT;
            p1->y += MUL(((p2->y - (*pp1)->y) >> ZSHIFT), z1) << ZSHIFT;
        } else {
            p1->x += (int32)MULDIV(((p2->x - (*pp1)->x) >> ZSHIFT), z1, dz) << ZSHIFT;
            p1->y += (int32)MULDIV(((p2->y - (*pp1)->y) >> ZSHIFT), z1, dz) << ZSHIFT;
        }*/
    	n1 = *((sVector *)p1);
    	n2 = *((sVector *)p2);
    	while (n1.z < NEAR_CLIP) {
        	n3.x = (n1.x + n2.x) >> 1;
        	n3.y = (n1.y + n2.y) >> 1;
        	n3.z = (n1.z + n2.z) >> 1;
        	if (n3.z <= NEAR_CLIP)
            	n1 = n3;
        	else
            	n2 = n3;
    	}
		p1 = clip_pt++;
    	p1->x = n1.x;
    	p1->y = n1.y;
    	p1->z = NEAR_CLIP;
		*pp1 = p1;
    	return 1;
    } else {
        if (p2->z >= NEAR_CLIP)
            return 1;
        if (p1->z < NEAR_CLIP)
            return 0;
        if (p1->z == NEAR_CLIP) {
			p2 = clip_pt++;
            p2->x = p1->x;
            p2->y = p1->y;
        	p2->z = NEAR_CLIP;
			*pp2 = p2;
        	return 2;
        }
/*		dz = (int)((p1->z - p2->z) >> ZSHIFT);
        z1 = (int)((NEAR_CLIP - p2->z) >> ZSHIFT);
		p2 = clip_pt++;
        if (dz == 0) {
            p2->x += MUL(((p1->x - (*pp2)->x) >> ZSHIFT), z1) << ZSHIFT;
            p2->y += MUL(((p1->y - (*pp2)->y) >> ZSHIFT), z1) << ZSHIFT;
        } else {
            p2->x += (int32)MULDIV(((p1->x - (*pp2)->x) >> ZSHIFT), z1, dz) << ZSHIFT;
            p2->y += (int32)MULDIV(((p1->y - (*pp2)->y) >> ZSHIFT), z1, dz) << ZSHIFT;
        }*/
    	n1 = *((sVector *)p2);
    	n2 = *((sVector *)p1);
    	while (n1.z < NEAR_CLIP) {
        	n3.x = (n1.x + n2.x) >> 1;
        	n3.y = (n1.y + n2.y) >> 1;
        	n3.z = (n1.z + n2.z) >> 1;
        	if (n3.z <= NEAR_CLIP)
            	n1 = n3;
        	else
            	n2 = n3;
    	}
		p2 = clip_pt++;
    	p2->x = n1.x;
    	p2->y = n1.y;
    	p2->z = NEAR_CLIP;
		*pp2 = p2;
    	return 2;
	}
}

static void medium_line(Cache_pt *s1, Cache_pt *s2, int colour)
{
    int x1, y1, x2, y2;
	int32 zf;

    if (z_clip(&s1, &s2) > 0) {
        zf = s1->z >> pshift;
        if (zf > 0) {
            x1 = xcentre + (int)(s1->x / zf);
            y1 = ycentre - (int)(s1->y / zf);
        } else {
			stop("Perspective overflow in medium_line");
            x1 = xcentre + (int)s1->x;
            y1 = ycentre - (int)s1->y;
        }
        zf = s2->z >> pshift;
        if (zf > 0) {
            x2 = xcentre + (int)(s2->x / zf);
            y2 = ycentre - (int)(s2->y / zf);
        } else {
			stop("Perspective overflow in medium_line");
            x2 = xcentre + (int)s2->x;
            y2 = ycentre - (int)s2->y;
        }
        line(x1, y1, x2, y2, colour);
	}
}

void line3(sVector *p1, sVector *p2, int colour)
{
    Cache_pt s1, s2;

	clip_pt = clip_pts;
	trans_point(p1, (sVector *)&s1);
    trans_point(p2, (sVector *)&s2);
    medium_line(&s1, &s2, colour);
}

void polygon3(int npts, sVector pts[], int col)
{
    int i, c, n;
	int32 zf;
    sVector p1, p2, np;
    sVector2 spts[MAX_PTS];

    trans_point(&pts[npts - 1], &np);
    for (i = 0, n = 0; i < npts; i++) {
        p1 = np;
        trans_point(&pts[i], &np);
        p2 = np;
        c = z_clip_v(&p1, &p2);
        if (c > 0) {
            zf = p1.z >> pshift;
			if (zf > 0) {
    			spts[n].x = xcentre + (int)(p1.x / zf);
            	spts[n++].y = ycentre - (int)(p1.y / zf);

        	} else {
				stop("Perspective overflow in polygon3");
	            	spts[n].x = xcentre + (int)p1.x;
            	spts[n++].y = ycentre - (int)p1.y;
        	}
            if (c > 1) {
                if (p2.z != np.z) {
                    zf = p2.z >> pshift;
					if (zf > 0) {
	                    	spts[n].x = xcentre + (int)(p2.x / zf);
                    	spts[n++].y = ycentre - (int)(p2.y / zf);
        			} else {
						stop("Perspective overflow in polygon3");
	           			spts[n].x = xcentre + (int)p2.x;
            			spts[n++].y = ycentre - (int)p2.y;
        			}
                }
            }
        }
    }
    if (n > 2) {
        if (clockwise_test) {
            if (clockwise2(spts))
                polygon(n, spts, col);
        }
        else {
            polygon(n, spts, col);
        }
    }
}

/* Z plane 3D clipping */
static void polygons(int npolys, sPolygon **polys, int shade)
{
    int i, p, c, n, a;
    int npts;
	Pindex *points;
    int32 zf;
    sPolygon *poly;
	sVector v1, norm;
    Cache_pt *p1, *p2, *np;
    sVector2 spts[MAX_PTS];

	for (p = 0; p < npolys; p++) {
		clip_pt = clip_pts;
		poly = polys[p];
		points = poly->points;
        npts = poly->npoints;
        if (npts == 2) {
            medium_line(&tpts[points[0]], &tpts[points[1]], poly->colour - shade);
        } else {
            np = &tpts[points[0]];
			if (clockwise_test) {
				rotate(&poly->norm, &norm);
				v1.x = (int)(np->x >> 10);
				v1.y = (int)(np->y >> 10);
				v1.z = (int)(np->z >> 10);
				a = (int)((MUL(norm.x, v1.x) + MUL(norm.y, v1.y) + MUL(norm.z, v1.z)) >> SSHIFT);
			} else {
				a = -1;
			}
			if (a < 0) {
				if (poly->colour >= 256) {
					if (perspective_texturing == TRUE)
						clipped_pctex_polygon(poly, tpts);
					else
						clipped_tex_polygon(poly, tpts);
				} else {
					stop ("huh");
            		for (i = 0, n = 0; i < npts; i++) {
                		p1 = np;
						if (i < (npts - 1))
							np = &tpts[points[i+1]];
						else
            				np = &tpts[points[0]];
                		p2 = np;
                		c = z_clip(&p1, &p2);
                		if (c > 0) {
                			zf = p1->z >> pshift;
                			if (zf > 0) {
                    			spts[n].x = (int)(xcentre + (p1->x / zf));
                    			spts[n++].y = (int)(ycentre - (p1->y / zf));
                			} else {
                    			stop("Perspective overflow in polygons");
                    			spts[n].x = (int)(xcentre + p1->x);
                    			spts[n++].y = (int)(ycentre - p1->y);
                			}
                    		if (c > 1) {
                        		if (p2->z != np->z) {
                					zf = p2->z >> pshift;
                					if (zf > 0) {
                    					spts[n].x = (int)(xcentre + (p2->x / zf));
                    					spts[n++].y = (int)(ycentre - (p2->y / zf));
                					} else {
                    					stop("Perspective overflow in polygons");
                    					spts[n].x = (int)(xcentre + p2->x);
                    					spts[n++].y = (int)(ycentre - p2->y);
                					}
                        		}
                    		}
                		}
            		}
            		if (n > 2) {
                   		polygon(n, spts, poly->colour - shade);
            		}
				}
			}
        }
    }
}

/* Only 2D clipping */
static void far_polygons(int npolys, sPolygon **polys, int shade)
{
    int i, p, clip;
	int pt, pt2, npts, a;
	Pindex *points;
    sPolygon *poly;
	sVector v1, norm;
    sVector2 spts[MAX_PTS], *sp;
	int sptsz [MAX_PTS], *spz;	// Z required for perspective correct texturing
	
    for (p = 0; p < npolys; p++) {
        poly = polys[p];
		points = poly->points;
        npts = poly->npoints;
        if (npts == 2) {
            pt = points[0];
            pt2 = points[1];
			line(tpts[pt].sx, tpts[pt].sy, tpts[pt2].sx, tpts[pt2].sy, poly->colour - shade);
        } else if (npts > 2) {
	        pt = points[0];
			if (clockwise_test) {
				rotate(&poly->norm, &norm);
				v1.x = (int)tpts[pt].x >> 10;
				v1.y = (int)tpts[pt].y >> 10;
				v1.z = (int)tpts[pt].z >> 10;
				a = (int)((MUL(norm.x, v1.x) + MUL(norm.y, v1.y) + MUL(norm.z, v1.z)) >> SSHIFT);
			} else {
				a = -1;
			}
			if (a < 0) {
				sp = spts;
				spz = sptsz;
				sp->x = tpts[pt].sx;
            	sp->y = tpts[pt].sy;
				*spz++ = tpts[pt].sz;
				sp++;
				clip = tpts[pt].flag;
	        	pt = points[1];
				sp->x = tpts[pt].sx;
            	sp->y = tpts[pt].sy;
				*spz++ = tpts[pt].sz;
				sp++;
				clip |= tpts[pt].flag;
	        	pt = points[2];
				sp->x = tpts[pt].sx;
            	sp->y = tpts[pt].sy;
				*spz++ = tpts[pt].sz;
				sp++;
				clip |= tpts[pt].flag;
            	for (i = 3; i < npts; i++) {
	        		pt = points[i];
					sp->x = tpts[pt].sx;
            		sp->y = tpts[pt].sy;
					*spz++ = tpts[pt].sz;
					sp++;
					clip |= tpts[pt].flag;
            	}
            	if (clip) {
					if (poly->colour < 256)
                    	polygon(npts, spts, poly->colour - shade);
					else
					{
						if (perspective_texturing == TRUE)
							pctex_polygon(npts, spts, sptsz, poly->colour - 256);
						else
                    		tex_polygon(npts, spts, poly->colour - 256);
					}
            	} else {
					if (poly->colour < 256)
                    	poly_prim(npts, spts, COL(poly->colour - shade));
					else
					{
						if (perspective_texturing == TRUE)
						  	pctex_poly_prim (npts, spts, sptsz, poly->colour - 256);
						else
                    		tex_poly_prim(npts, spts, poly->colour - 256);
					}
				}
        	}
       	}
    }
}

/* Shapes that will definitely not need 3D clipping */
/* 2D screen coords for each vertex done only once */
void far_shape3(sShape *shape, int shade)
{
    int i;
	int32 zf;
	register Cache_pt *tp;
    register Vertex *pts;

    for (i = shape->npts, pts = shape->pts, tp = tpts; i; i--, pts++, tp++) {
    	tp->x = (pts->x*matrix->a + pts->y*matrix->d + pts->z*matrix->g) + matrix->j;
    	tp->y = (pts->x*matrix->b + pts->y*matrix->e + pts->z*matrix->h) + matrix->k;
    	tp->z = (pts->x*matrix->c + pts->y*matrix->f + pts->z*matrix->i) + matrix->l;
        if ((zf = tp->z >> pshift) > 0) {
           	tp->sx = xcentre + (int)DIV(tp->x, zf);
           	tp->sy = ycentre - (int)DIV(tp->y, zf);
			tp->sz = tp->z;
        } else {
            tp->sx = xcentre + (int)tp->x;
            tp->sy = ycentre - (int)tp->y;
			tp->sz = tp->z;
        }
        if ((tp->sx < clip_region.left) || (tp->sx > clip_region.right) ||
            (tp->sy < clip_region.top) || (tp->sy > clip_region.bottom)) {
			tp->flag = TRUE;
        } else {
			tp->flag = FALSE;
        }
    }
	far_polygons(shape->npolygons, shape->polygons, shade);
}

/* Small shapes that only need Z plane 3D clipping */
void shape3(sShape *shape, int shade)
{
    register int i;
	register Cache_pt *tp;
    register Vertex *pts;

    for (i = shape->npts, pts = shape->pts, tp = tpts; i; i--, pts++, tp++) {
    	tp->x = (((int)pts->x*matrix->a + (int)pts->y*matrix->d + (int)pts->z*matrix->g) + matrix->j);
    	tp->y = (((int)pts->x*matrix->b + (int)pts->y*matrix->e + (int)pts->z*matrix->h) + matrix->k);
    	tp->z = (((int)pts->x*matrix->c + (int)pts->y*matrix->f + (int)pts->z*matrix->i) + matrix->l);
		tp->flag = FALSE;
	}
    polygons(shape->npolygons, shape->polygons, shade);
}

/* Shapes that used to need full 3D viewing cone clipping */
/* Scale and clipping distance tuning now allow shape3() method */
void near_shape3(sShape *shape, int shade)
{
    register int i;
	register Cache_pt *tp;
    register Vertex *pts;

    for (i = shape->npts, pts = shape->pts, tp = tpts; i; i--, pts++, tp++) {
    	tp->x = (((int)pts->x*matrix->a + (int)pts->y*matrix->d + (int)pts->z*matrix->g) + matrix->j);
    	tp->y = (((int)pts->x*matrix->b + (int)pts->y*matrix->e + (int)pts->z*matrix->h) + matrix->k);
    	tp->z = (((int)pts->x*matrix->c + (int)pts->y*matrix->f + (int)pts->z*matrix->i) + matrix->l);
		tp->flag = FALSE;
	}
    polygons(shape->npolygons, shape->polygons, shade);
}

void setup_graphics3(void)
{
	reset_matrix();
	xcentre = screen_width / 2;
	ycentre = screen_height / 2;
}