/* --------------------------------------------------------------------------
 * WORLD		Copyright (c) Simis Ltd 1993,994,1995
 * ---			All Rights Reserved
 *
 * File:		explo.c
 * Ver:			1.00
 * Desc:		Implements explosion object
 *
 * -------------------------------------------------------------------------- */
#include <stdlib.h>
#include "world.h"
#include "explo.h"
#include "ground.h"
#include "bangs.h"

/* Declare Class Pointer */
ExploClass *Explo_class = NULL;
ExploClass *Debris_class = NULL;

#define DRAW_EXPLO_DETAIL Meter(4000)

static int c_tracer1 = 0;
static int c_tracer2 = 0;


#define BULLET_SIZE 256*5 /* 1M at scale 8 */
static int check_coarse_hit(sObject *self, sObject *obj, sShape *s, sVector *p, sVector *r)
{
	Explo *b;
    unsigned rad, d1, d2;
    int i, hit, scale;
    sVector p1, p2, p3, *min, *max;
    Transdata t;

	b = self->data;

	scale = s->scale;
    rad = MUL(ABS(b->speed), frame_time) + BULLET_SIZE + s->radius;
    p1.x = (int)((b->p.x - p->x) >> scale);
    p1.y = (int)((b->p.y - p->y) >> scale);
    p1.z = (int)((b->p.z - p->z) >> scale);
    if ((ABS(p1.x) < rad) && (ABS(p1.y) < rad) && (ABS(p1.z) < rad)) {
        p2.x = (int)((b->op.x - p->x) >> scale);
        p2.y = (int)((b->op.y - p->y) >> scale);
        p2.z = (int)((b->op.z - p->z) >> scale);
        setup_trans(&b->r, &t);
        invrot3d(&p1, &p1, &t);
        invrot3d(&p2, &p2, &t);
        min = &s->min_size;
        max = &s->max_size;
        d1 = ABS(p1.x) + ABS(p1.y) + ABS(p1.z);
        d2 = ABS(p2.x) + ABS(p2.y) + ABS(p2.z);
        p3.x = (p1.x + p2.x) >> 1;
        p3.y = (p1.y + p2.y) >> 1;
        p3.z = (p1.z + p2.z) >> 1;
        for (hit = FALSE, i = 0; i < 9; i++) {
            if ((p3.x > min->x) && (p3.x < max->x) &&
                    (p3.y > min->y) && (p3.y < max->y) &&
                    (p3.z > min->z) && (p3.z < max->z)) {
				hit = TRUE;
                break;
            }
            if (d1 <= d2) {
                p2 = p3;
                d2 = ABS(p2.x) + ABS(p2.y) + ABS(p2.z);
            } else {
                p1 = p3;
                d1 = ABS(p1.x) + ABS(p1.y) + ABS(p1.z);
            }
            p3.x = (p1.x + p2.x) >> 1;
            p3.y = (p1.y + p2.y) >> 1;
            p3.z = (p1.z + p2.z) >> 1;
        }
        if (hit) {
            return TRUE;
        }
    }
    return FALSE;
}


static int collide(sObject *self, sObject *c)
{
	World *o;
	sShape *s;

	o = c->data;
	if (o->shape) {
		s = o->shape->s;
		if (check_coarse_hit(self, c, s, &o->p, &o->r))
			return 1;
	}
	return 0;
}


/* Create internal pointer structure for Shape2
 * imitating (Shape*) with 2 polygons of n points
 */
static void internal_link_shape2(sShape2 *s2, int n)
{
	if (n == 2) {
		s2->pts = s2->point_data;
    	s2->polygons = s2->polygon_pointers;
    	s2->polygons[0] = &s2->polygon_data1;
    	s2->polygon_data1.points = s2->points1;

    	s2->npts = n;
    	s2->npolygons = 1;
    	s2->polygon_data1.npoints = n;
	} else {
		s2->pts = s2->point_data;
    	s2->polygons = s2->polygon_pointers;
    	s2->polygons[0] = &s2->polygon_data1;
    	s2->polygons[1] = &s2->polygon_data2;
    	s2->polygon_data1.points = s2->points1;
    	s2->polygon_data2.points = s2->points2;

    	s2->npts = n;
    	s2->npolygons = 2;
    	s2->polygon_data1.npoints = n;
    	s2->polygon_data2.npoints = n;
	}
}

static void flatten_shape(sShape2 *s2)
{
	sShape *s;
    int n;

	s = (sShape*)s2;
    for (n = 0; n < s->npts; n++) {
        s->pts[n].y = 0;
    }
}

/* METHOD : Destroy
 * deallocate instance data
 */
static void destroy_explo(sObject *self)
{
    ExploClass *c;

    c = self->class;
    block_free(self->data, c->sizeof_data);
}

static void update_debris(sObject *self)
{
    remove_object(self);
}

static void update_explo(sObject *self)
{
    Explo *e;
	long int ground_height;

    e = self->data;

	/* When explo reaches ground - flatten shape and set timer
	 * to random value after which object is removed from world
	 */
	ground_height = get_terrain_height(e->p.x, e->p.z);
    if (e->p.y < ground_height) {
		e->r.x = e->r.z = 0;
		e->p.y 		= ground_height;
        flatten_shape(&e->explo_shape);
		e->timer 	= 1000+(fst_random()<<3), rand_vect();
        self->class = Debris_class;
        return;
    }

    e->r.x = ADDANGLE(e->r.x, MUL(e->dr.x, frame_time));
    e->r.y = ADDANGLE(e->r.y, MUL(e->dr.y, frame_time));
    e->r.z = ADDANGLE(e->r.z, MUL(e->dr.z, frame_time));

    e->wv.y -= BULLET_GRAVITY * frame_time;
    e->op = e->p;
    e->p.x += MUL(e->wv.x, frame_time);
    e->p.y += MUL(e->wv.y, frame_time);
    e->p.z += MUL(e->wv.z, frame_time);
}

int draw_explo(sObject *self)
{
    Explo *e;
    register int dist;
    sVector v;
    sVector2 s1, s2;

    e = self->data;

    v.x = (int)((e->p.x - viewer_p->x) >> SPEED_SHIFT);
    v.y = (int)((e->p.y - viewer_p->y) >> SPEED_SHIFT);
    v.z = (int)((e->p.z - viewer_p->z) >> SPEED_SHIFT);
    dist = fast_perspect(&v, &s1);
    if (dist > 40) {
        if (dist < DRAW_EXPLO_DETAIL) {
            return draw_shape((sShape*)(&(e->explo_shape)), &e->p, &e->r, FALSE);
        } else {
            pixel_prim(s1.x, s1.y, COL(c_tracer2));
        }
    }
    return TRUE;
}

static void extract_polygon(sShape2 *s2, sShape *s, int n, sVector *pc)
{
    int     i, m;
    sShape   *ps;
    sPolygon *p;
	sVector max, min;
	Vertex  *pt;

    p = s->polygons[n];
    m = MIN(p->npoints, SHAPE2_MAXPOINTS);

    /* Reset shape structure */
	internal_link_shape2(s2, m);
    ps = (sShape*)s2;

    ps->npts      = m;
    ps->npolygons = (m == 2) ? 1 : 2;
    ps->scale     = s->scale;

    /* Copy points/polygon data */
    ps->polygons[0]->base_colour = p->base_colour;
    ps->polygons[0]->colour = p->colour;
    ps->polygons[0]->npoints = m;
    ps->polygons[0]->norm = p->norm;
	if (m > 2) {
    	ps->polygons[1]->base_colour = p->base_colour;
    	ps->polygons[1]->colour = 0;
    	ps->polygons[1]->npoints = m;
    	ps->polygons[1]->norm = p->norm;
	}

	max.x = max.y = max.z = min.z = min.y = min.z = 0;
    for (i = 0; i < m; i++) {
        ps->pts[i] = s->pts[p->points[i]];
        ps->polygons[0]->points[i] = i;

		/* Only create two polygons if source is not a line ! */
		if (m > 2)
        	ps->polygons[1]->points[i] = m-i-1;

		max.x = MAX(max.x, ps->pts[i].x);
		max.y = MAX(max.y, ps->pts[i].y);
		max.z = MAX(max.z, ps->pts[i].z);
		min.x = MIN(min.x, ps->pts[i].x);
		min.y = MIN(min.y, ps->pts[i].y);
		min.z = MIN(min.z, ps->pts[i].z);
    }

	poly_normal(&ps->pts[1], &ps->pts[0], &ps->pts[2], &ps->polygons[0]->norm);
	poly_normal(&ps->pts[2], &ps->pts[0], &ps->pts[1], &ps->polygons[1]->norm);

	/* Caclulate polygon centre - offset from source shape centre */
	pc->x = min.x + ((max.x - min.x)/2);
	pc->y = min.y + ((max.y - min.y)/2);
	pc->z = min.z + ((max.z - min.z)/2);

	/* Normalise polygon */
    for (i = 0; i < m; i++) {
        ps->pts[i].x -= pc->x;
        ps->pts[i].y -= pc->y;
        ps->pts[i].z -= pc->z;
    }
    ps->min_size.x = min.x - pc->x;
    ps->min_size.y = min.y - pc->y;
    ps->min_size.z = min.z - pc->z;
    ps->max_size.x = max.x - pc->x;
    ps->max_size.y = max.y - pc->y;
    ps->max_size.z = max.z - pc->z;
    ps->radius = MAX(ps->max_size.x, MAX(ps->max_size.y, ps->max_size.z));
/*	if ((ps->min_size.x - ps->max_size.x) == 0)
		ps->max_size.x = ps->min_size.x + (ps->radius >> 1);
	if ((ps->min_size.y - ps->max_size.y) == 0)
		ps->max_size.y = ps->min_size.y + (ps->radius >> 1);
	if ((ps->min_size.z - ps->max_size.z) == 0)
		ps->max_size.z = ps->min_size.z + (ps->radius >> 1);*/
}

static void new_explo(sShape *s, int n, sVector p, sVector v, int life, sVector *dr)
{
    sObject *o;
    Explo  *e;
	sVector  pc;

    o = new_object(Explo_class);
    if (o) {
        e = o->data;
        e->op = e->p = p;
        e->wv = v;
        e->dr = *dr;
        e->timer = 0;
        e->shape = NULL;

		extract_polygon(&e->explo_shape, s, n, &pc);

		/* Add offset of source polygon centre to explo element posn */
		e->p.x += pc.x << s->scale;
		e->p.y += pc.y << s->scale;
		e->p.z += pc.z << s->scale;
        e->op = e->p;

        attach_object(o);
    }
}

void create_explo(sVector p, sVector wv, sShape_def *sd, int speed)
{
    int i;
    sShape *s;
    sVector v;
    sVector *rand;

	if (sd->down) {
		sd = sd->down;
	}
    s = sd->s; /* Find medium detail level to explode */
	if (s == NULL)
		return;
	speed >>= 1;
	for (i = 0; i < s->npolygons; i++) {
        v = wv;
        rand = rand_vect();
        v.x = (int)MUL(rand->x, speed);
        v.y = (int)MUL(rand->y, speed);
        v.z = (int)MUL(rand->z, speed);
        if (v.y < 0)
            v.y = -v.y;
        new_explo(s, i, p, v, 1000+(fst_random()<<3), rand_vect());
    }
}

void register_explo(void)
{
    Explo_class = new_class(sizeof(ExploClass), "Explo", sizeof(Explo), Mobile_class);

    /* Class methods */
    Explo_class->update       = update_explo;
    Explo_class->draw         = draw_explo;
    Explo_class->destroy      = destroy_explo;
	Explo_class->check_collide = NULL;
    Explo_class->collide      = collide;

    Debris_class = copy_class(Explo_class, "Debris");
    Debris_class->update      = update_debris;
}
