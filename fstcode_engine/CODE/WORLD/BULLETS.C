/* --------------------------------------------------------------------------
 * WORLD		Copyright (c) Simis Ltd 1993,994,1995
 * ---			All Rights Reserved
 *
 * File:		bullets.c
 * Ver:			1.00
 * Desc:        Implements bullet object
 *
 * -------------------------------------------------------------------------- */
#include <stdlib.h>

#include "bangs.h"
#include "bullets.h"

#include "ground.h"

#define INERT_BULLET     1
#define EXPLOSIVE_BULLET 2
#define BULLET_CHANGE Meter(425)
#define BULLET_MAX_DIST Meter(3000)


/* Declare Class Pointer */
BulletsClass *Bullets_class = NULL;

sObject *bullet_scoring_firer = NULL;

static int c_tracer1 = 20;
static int c_tracer2 = 21;

#define BULLET_SIZE 2 /* 1M at scale 8 */
static int check_coarse_hit(sObject *self, sObject *obj, sShape *s, sVector *p, sVector *r)
{
	Bullets *b;
    unsigned rad, d1, d2;
    int i, hit, scale;
    sVector p1, p2, p3, *min, *max;
    Transdata t;

	b = self->data;
	if (b->firer == obj)
		return FALSE;

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
    Bullets *b;
	sShape *s;

	o = c->data;
	if (o->shape) {
		s = o->shape->s;
		if (check_coarse_hit(self, c, s, &o->p, &o->r)) {
			b = self->data;
			if (b->firer == bullet_scoring_firer)
				player_scored_hit(self);
			SafeMethod(self, Bullets, kill)(self);
			return 1;
		}
	}
	return 0;
}


void fire_bullet(sObject *o, sVector *aspect, int32 offset, int speed, int life, int flag)
{
    sObject *obj;
    Base   *firer;
    Bullets *bullet;
    sVector p;
    Transdata t;

    firer = o->data;

    setup_trans(aspect, &t);
    p.x = p.y = 0;
    p.z = offset + MUL(speed, frame_time);
    rot3d(&p, &p, &t);

    obj = new_object(Bullets_class);
    if (obj) {
        bullet = obj->data;
        if (bullet != NULL) {
            bullet->firer = o;
            bullet->p.x = firer->p.x + p.x;
            bullet->p.y = firer->p.y + p.y;
            bullet->p.z = firer->p.z + p.z;
            bullet->op = firer->p;
            bullet->r = *aspect;
            bullet->life = life;
            if (flag)
                bullet->type = EXPLOSIVE_BULLET;
            else
                bullet->type = INERT_BULLET;
            bullet->wv.x = bullet->wv.y = 0;
            bullet->wv.z = speed;
            bullet->radius = MUL(bullet->wv.z, frame_time);
            rot3d(&bullet->wv, &bullet->wv, &t);
        }
		attach_object(obj);
    }
}

/* v1->y >= g->y, v2->y < g->y */
void ground_clip(sVector *v1, sVector *v2, sVector *g)
{
    sVector d;

    d.x = (v2->x - v1->x) >> 8;
    d.y = (v2->y - v1->y) >> 8;
    d.z = (v2->z - v1->z) >> 8;
    if (d.y == 0) {
        g->x = v2->x;
        g->z = v2->z;
    } else {
        g->x = v2->x - ((d.x * v2->y) / d.y);
        g->z = v2->z - ((d.z * v2->y) / d.y);
    }
}

void update_bullet(sObject *self)
{
    Bullets *b;
    sVector g;
	int32 ground_height;

    b = self->data;
    ground_height = get_terrain_height(b->p.x, b->p.z);
    if (((b->life -= frame_time) < 0) || (b->p.y < ground_height)) {
        if (b->p.y < ground_height) {
            small_ground_bang(&b->op);
        } else if (b->type & EXPLOSIVE_BULLET) {
            small_bang(&b->p);
        }
        remove_object(self);
     } else {
        b->wv.y -= BULLET_GRAVITY * frame_time;
        b->op = b->p;
        b->p.x += MUL(b->wv.x, frame_time);
        b->p.y += MUL(b->wv.y, frame_time);
        b->p.z += MUL(b->wv.z, frame_time);
    }
}

int draw_bullet(sObject *self)
{
    Bullets *b;
    register int dist;
    sVector v;
    sVector2 s1, s2;

    b = self->data;
    v.x = (int)((b->p.x - viewer_p->x) >> SPEED_SHIFT);
    v.y = (int)((b->p.y - viewer_p->y) >> SPEED_SHIFT);
    v.z = (int)((b->p.z - viewer_p->z) >> SPEED_SHIFT);
    dist = fast_perspect(&v, &s1);
    if ((dist > 40) && (dist < BULLET_MAX_DIST)) {
        if (dist < BULLET_CHANGE) {
            v.x = (int)((b->op.x - viewer_p->x) >> SPEED_SHIFT);
            v.y = (int)((b->op.y - viewer_p->y) >> SPEED_SHIFT);
            v.z = (int)((b->op.z - viewer_p->z) >> SPEED_SHIFT);
            if (fast_perspect(&v, &s2) > 40)
                line(s2.x, s2.y, s1.x, s1.y, c_tracer1);
            else
                pixel_prim(s1.x, s1.y, COL(c_tracer1));
        } else {
            pixel_prim(s1.x, s1.y, COL(c_tracer2));
        }
		return TRUE;
    }
	return FALSE;
}

void destroy_bullet(sObject *self)
{
    BaseClass *c;

    SafeMethod(self, Bullets, detach)(self);

    c = self->class;
    block_free(self->data, c->sizeof_data);
}

static void file_save(sObject *self, FILE *fp)
{
    Bullets *o;

    o = self->data;
    SaveVector(fp, o->p);
    SaveVector(fp, o->op);
    fprintf(fp, "%d %d %ld %ld\n", o->type, o->life, o->speed, o->radius);
    SaveVector(fp, o->wv);
    SaveVector(fp, o->r);

    fprintf(fp, "%d\n", object_id(o->firer));
}

static void file_load(sObject *self, FILE *fp)
{
    Bullets *o;
    int id;

    o = self->data;
    LoadVector(fp, o->p);
    LoadVector(fp, o->op);
    fscanf(fp, "%d %d %ld %ld\n", &o->type, &o->life, &o->speed, &o->radius);
    LoadVector(fp, o->wv);
    LoadVector(fp, o->r);

    fscanf(fp, "%d\n", &id);
    o->firer = get_object(id);

	attach_object(self);
}

static void kill(sObject *o)
{
	small_bang(Posn(o));
    remove_object(o);
}


void register_bullets(void)
{
    Bullets_class = new_class(sizeof(BulletsClass), "Bullets", sizeof(Bullets), Mobile_class);

    /* Class methods */
    Bullets_class->update  = update_bullet;
    Bullets_class->draw    = draw_bullet;

    Overide(Bullets_class, file_save, file_save);
    Overide(Bullets_class, file_load, file_load);
    Overide(Bullets_class, destroy,   destroy_bullet);
	Bullets_class->collide = collide;
	Bullets_class->kill    = kill;
}

