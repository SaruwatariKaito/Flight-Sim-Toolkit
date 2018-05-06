/* --------------------------------------------------------------------------
 * WORLD		Copyright (c) Simis Ltd 1993,994,1995
 * ---			All Rights Reserved
 *
 * File:		mobile.c
 * Ver:			1.00
 * Desc:		Mobile class - inherits from world 
 * and adds velocity and speed instance variables.
 * Implements collision detection.
 *
 * -------------------------------------------------------------------------- */
#include <stdlib.h>
#include "mobile.h"

/* Externed from world.c */
extern void moving_system_update(sObject *o);

/* Declare Class Pointer */
MobileClass *Mobile_class = NULL;

/* Check for collision between moving object obj and static
 * shape s at posn p */
static int fine_collision(sObject *obj, sShape *s, sVector *p, sVector *r)
{
	Mobile *o;
    register int hit, scale;
    register unsigned spd;
	sVector d;
	Collide_box b;

	o = obj->data;
	spd = (unsigned)(MUL(ABS(o->speed), frame_time) >> s->scale);

	/* find position of shape p */
    d.x = (p->x - o->p.x) >> s->scale;
    d.y = (p->y - o->p.y) >> s->scale;
    d.z = (p->z - o->p.z) >> s->scale;
	scale = s->scale - o->shape->s->scale;
	if (scale > 0) {
		b.left   = o->shape->s->min_size.x >> scale;
		b.right  = o->shape->s->max_size.x >> scale;
		b.bottom = o->shape->s->min_size.y >> scale;
		b.top    = o->shape->s->max_size.y >> scale;
		b.back   = o->shape->s->min_size.z >> scale;
		b.front  = (o->shape->s->max_size.z >> scale) + spd;
	} else {
		scale    = -scale;
		b.left   = o->shape->s->min_size.x << scale;
		b.right  = o->shape->s->max_size.x << scale;
		b.bottom = o->shape->s->min_size.y << scale;
		b.top    = o->shape->s->max_size.y << scale;
		b.back   = o->shape->s->min_size.z << scale;
		b.front  = (o->shape->s->max_size.z << scale) + spd;
	}
	push_matrix();
	zero_matrix();
    rotz(o->r.z);
    rotx(o->r.x);
    roty(o->r.y);
    translate(d.x, d.y, d.z);
    roty(-r->y);
    rotx(-r->x);
    rotz(-r->z);
	hit = collide3(s, &b);
	pop_matrix();
    return hit;
}

/* Check for collision between mobile object o and fixed object
 * shape s at posn p with rotation r
 */
int check_collision(sObject *obj, sShape *s, sVector *p, sVector *r, int fine_check)
{
	Mobile *o;
    register int i, radius, hit, scale;
    register unsigned d1, d2;
    int32 rad;
	sObject *n;
    sVector  d;
	sVector *vs_min, *vs_max;
    sVector p1, p2, p3;
    Transdata t;

	o = obj->data;
    rad = MUL(ABS(o->speed), frame_time) + (o->shape->s->radius<<o->shape->s->scale) + (s->radius<<s->scale);
    d.x = o->p.x - p->x;
    d.y = o->p.y - p->y;
    d.z = o->p.z - p->z;
    if ((ABS(d.x) < rad) && (ABS(d.z) < rad) && (ABS(d.y) < rad)) {
        scale = s->scale;
		p1.x = (int)(d.x >> scale);
    	p1.y = (int)(d.y >> scale);
    	p1.z = (int)(d.z >> scale);
        p2.x = (int)((o->op.x - p->x) >> scale);
        p2.y = (int)((o->op.y - p->y) >> scale);
        p2.z = (int)((o->op.z - p->z) >> scale);
        setup_trans(r, &t);
        invrot3d(&p1, &p1, &t);
        invrot3d(&p2, &p2, &t);
        d1 = ABS(p1.x) + ABS(p1.y) + ABS(p1.z);
        d2 = ABS(p2.x) + ABS(p2.y) + ABS(p2.z);
        p3.x = (p1.x + p2.x) >> 1;
        p3.y = (p1.y + p2.y) >> 1;
        p3.z = (p1.z + p2.z) >> 1;
		vs_min = &s->min_size;
		vs_max = &s->max_size;
		radius = o->shape->s->radius;
        for (hit = FALSE, i = 0; i < 9; i++) {
            if (((p3.x + radius) > vs_min->x) &&
                ((p3.x - radius) < vs_max->x) &&
                ((p3.y + radius) > vs_min->y) &&
                ((p3.y - radius) < vs_max->y) &&
                ((p3.z + radius) > vs_min->z) &&
                ((p3.z - radius) < vs_max->z)) {
				if (fine_check) {
                    hit = fine_collision(obj, s, p, r);
				} else {
					hit = TRUE;
				}
				if (hit) {
            		return TRUE;
				}
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
    }
    return FALSE;
}

/* Returns number of kp caused by collision between self and
 * object c - default 1 */
static int collide(sObject *self, sObject *c)
{
	Mobile *m;
	World  *o;
	sShape  *s;

	m = self->data;
	o = c->data;
	if (m->shape && o->shape) {
		s = o->shape->s;
		if (check_collision(self, s, &o->p, &o->r, FALSE))
			return 1;
	}
	return 0;
}

static int draw_shaded(sObject *self)
{
    World *o;

    o = self->data;
    if (o->shape) {
        return(draw_shape_def((sShape_def *)o->shape, &o->p, &o->r, TRUE));
    }
	return FALSE;
}

static void reset_mobile(sObject *self)
{
    World *o;

    o = self->data;
	o->flags |= MOBILE;
}

void register_mobile(void)
{
    Mobile_class = new_class(sizeof(MobileClass), "Mobile", sizeof(Mobile), World_class);

    Mobile_class->reset = reset_mobile;
	Mobile_class->check_collide = check_collisions;
	Mobile_class->collide = collide;
    Mobile_class->system_update= moving_system_update;
	Mobile_class->draw = draw_shaded;
}
