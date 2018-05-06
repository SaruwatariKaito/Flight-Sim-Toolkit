/* --------------------------------------------------------------------------
 * WORLD		Copyright (c) Simis Ltd 1993,994,1995
 * ---			All Rights Reserved
 *
 * File:		bangs.c
 * Ver:			1.00
 * Desc:		Implements particle explosion
 *
 * -------------------------------------------------------------------------- */
#include <stdlib.h>
#include "bangs.h"

/* Declare Class Pointers */
BangsClass *Bangs_class       = NULL;
BangsClass *GroundBangs_class = NULL;

/* --------------------------------------------------------------------------
 * Bangs:
 * ------
 * speed - the speed at which the bang expands
 * life  - the duration of the bang in the world
 * n     - number of particles in bang
 * bang shape can be sherical or half-spherical (ground bang)
 * (the initial velocity of each particle will determine shape)
 *
 * Max bang size is a function of speed and line
 */

#define BIG_BANG_N 200
#define MEDIUM_BANG_N 120
#define SMALL_BANG_N 60
#define BANG_SPEED1 FIXSPD(10)
#define BANG_SPEED2 FIXSPD(6)
#define BANG_SPEED3 FIXSPD(3)
#define BANG_SPEED4 FIXSPD(2)
#define BANG_LIFE1 16
#define BANG_LIFE2 8
#define BANG_LIFE3 2


#define C_BANG0 0x14
#define C_BANG1 0x14
#define C_BANG2 0x15
#define C_BANG3 0x15
#define C_BANG4 0x16
#define C_BANG5 0x17
#define C_BANG6 0x1c
#define C_BANG7 0x1b

static int bang_cols1[BANG_LIFE1] = {
        C_BANG7,C_BANG7,C_BANG6,C_BANG6,C_BANG5,C_BANG5,C_BANG4,C_BANG4,
        C_BANG3,C_BANG3,C_BANG2,C_BANG2,C_BANG1,C_BANG1,C_BANG0,C_BANG0};

static int bang_cols2[BANG_LIFE2] = {
        C_BANG7,C_BANG6,C_BANG5,C_BANG4,C_BANG3,C_BANG2,C_BANG1,C_BANG0};

static int blast_col = C_BANG0;

/* ---------------------------------------------------------------------------
 * Particles
 * --------- */
#define INIT_PARTICLES 160

static Particle *free_particles = NULL;

static void more_particles(int n)
{
    int i;
    Particle *p;

    p = (Particle *)heap_alloc(n * (unsigned)sizeof(Particle));
    if (p == NULL) {
        stop("No memory in more_particles");
    } else {
        for (i = 0; i < (n - 1); i++) {
            p[i].next = &p[i + 1];
        }
        p[n - 1].next = NULL;
    }
    free_particles = p;
}

static Particle *alloc_particle(void)
{
    Particle *t;

    t = free_particles;
    if (t == NULL) {
		more_particles(50);
    	t = free_particles;
	}
    if (t != NULL) {
        free_particles = free_particles->next;
        t->next = NULL;
    }
    return t;
}

static void unlink_particle(Particle **l, Particle *b)
{
    if (*l == b)
        *l = b->next;
    else
        b->prev->next = b->next;
    if (b->next != NULL)
        b->next->prev = b->prev;
    b->next = free_particles;
    free_particles = b;
}

/* ---------------------------------------------------------------------------
 * Class behaviour for Bangs
 * ------------------------- */
static void remove_particles(sObject *self)
{
    Bangs *bang;
    Particle *b, *dead, *bits;

    bang = self->data;
    bits = bang->particles;
    for (b = bits; b != NULL;) {
        dead = b;
        b = b->next;
        unlink_particle(&bits, dead);
    }
    bang->particles = NULL;
}

/* Remove bang object from world */
static void destroy_bang(sObject *self)
{
    BaseClass *c;

    SafeMethod(self, Bangs, detach)(self);

    remove_particles(self);

    c = self->class;
    block_free(self->data, c->sizeof_data);
}

static void update_bang(sObject *self)
{
    Particle *b, *dead, *particles;
    Bangs *bang;

    bang  = self->data;
    bang->count += 1;
    particles = bang->particles;
    for (b = particles; b != NULL;) {
        if (--b->life < 0) {
            dead = b;
            b = b->next;
            unlink_particle(&particles, dead);
        } else {
            b->p.x += b->v.x;
            b->p.y += b->v.y;
            b->p.z += b->v.z;
            b = b->next;
        }
    }
    bang->particles = particles;
    if (particles == NULL) {
        remove_object(self);
    }
}

int draw_bang(sObject *self)
{
    register Particle *b;
    register int *bangcols;
    register int dist;
    register Particle *particles;
    int      radius, rad;
    sVector d;
    sVector v, p;
    sVector2 s1, s2;
    Bangs *bang;

    bang  = self->data;
    /* d is Vector from viewer to centre of bang */
    d.x = bang->p.x - viewer_p->x;
    d.y = bang->p.y - viewer_p->y;
    d.z = bang->p.z - viewer_p->z;
    if ((ABS(d.x) < M(4000)) && (ABS(d.y) < M(4000)) && (ABS(d.z) < M(4000))) {
        /* v - */
        v.x = (int)(d.x >> SPEED_SHIFT);
        v.y = (int)(d.y >> SPEED_SHIFT);
        v.z = (int)(d.z >> SPEED_SHIFT);
        rotate(&v, &p);
        radius = Meter(100);
        /* Check bang is in viewing cone */
        if (((p.z + radius) > VIEW_MIN) && ((p.y - radius) < p.z) && ((p.x - radius) < p.z)) {
            /* Draw sphere at centre of bang */
            if (bang->count <= 1) {
				d.x >>= SPEED_SHIFT;
				d.y >>= SPEED_SHIFT;
				d.z >>= SPEED_SHIFT;
				sphere3(&d, Meter(2), blast_col);
			}
            particles = bang->particles;
            bangcols = bang->cols;
            for (b = particles; b != NULL; b = b->next) {
                /* s1 is transformed position of particle on screen */
                p.x = v.x + (int)(b->p.x >> SPEED_SHIFT);
                p.y = v.y + (int)(b->p.y >> SPEED_SHIFT);
                p.z = v.z + (int)(b->p.z >> SPEED_SHIFT);
                dist = fast_perspect(&p, &s1);
                if (dist > 5) {
                    if (dist > Meter(500)) {
                        pixel_prim(s1.x, s1.y, COL(bangcols[b->life]));
                    } else if (dist > Meter(400)) {
                        block_prim(s1.x, s1.y, COL(bangcols[b->life]));
                    } else {
                        block_prim(s1.x, s1.y, COL(bangcols[b->life]));
                        p.x = v.x + (int)((b->p.x + b->v.x) >> SPEED_SHIFT);
                        p.y = v.y + (int)((b->p.y + b->v.y) >> SPEED_SHIFT);
                        p.z = v.z + (int)((b->p.z + b->v.z) >> SPEED_SHIFT);
                        fast_perspect(&p, &s2);
                        line(s1.x, s1.y, s2.x, s2.y, bangcols[b->life]);
                    }
                }
            }
        }
    }
    return TRUE;
}

int draw_groundbang(sObject *self)
{
    register Particle *b;
    register int *bangcols;
    register int dist;
    register Particle *particles;
    int      radius, rad;
    sVector d;
    sVector v, p;
    sVector2 s1, s2;
    Bangs *bang;

    bang  = self->data;
    /* d is Vector from viewer to centre of bang */
    d.x = bang->p.x - viewer_p->x;
    d.y = bang->p.y - viewer_p->y;
    d.z = bang->p.z - viewer_p->z;
    if ((ABS(d.x) < M(4000)) && (ABS(d.y) < M(4000)) && (ABS(d.z) < M(4000))) {
        /* v - */
        v.x = (int)(d.x >> SPEED_SHIFT);
        v.y = (int)(d.y >> SPEED_SHIFT);
        v.z = (int)(d.z >> SPEED_SHIFT);
        rotate(&v, &p);
        radius = Meter(100);
        /* Check bang is in viewing cone */
        if (((p.z + radius) > VIEW_MIN) && ((p.y - radius) < p.z) && ((p.x - radius) < p.z)) {
            /* Draw sphere at centre of bang */
            if (bang->count <= 1) {
				d.x >>= SPEED_SHIFT;
				d.y >>= SPEED_SHIFT;
				d.z >>= SPEED_SHIFT;
				sphere3(&d, Meter(3), blast_col);
			}
            particles = bang->particles;
            bangcols = bang->cols;
            for (b = particles; b != NULL; b = b->next) {
                /* s1 is transformed position of particle on screen */
                p.x = v.x + (int)(b->p.x >> SPEED_SHIFT);
                p.y = v.y + (int)(b->p.y >> SPEED_SHIFT);
                p.z = v.z + (int)(b->p.z >> SPEED_SHIFT);
                dist = fast_perspect(&p, &s1);
                if (dist > 5) {
                    if (dist > Meter(500)) {
                        pixel_prim(s1.x, s1.y, COL(bangcols[b->life]));
                    } else if (dist > Meter(400)) {
                        block_prim(s1.x, s1.y, COL(bangcols[b->life]));
                    } else {
                        block_prim(s1.x, s1.y, COL(bangcols[b->life]));
                        fast_perspect(&v, &s2);
                        line(s1.x, s1.y, s2.x, s2.y, bangcols[b->life]);
                    }
                }
            }
        }
    }
    return TRUE;
}


/* Creates a linked particle list with profile set by arguments to
 * be attached to a new bang object
 */
static Particle *start_bang(int n, int speed, int life, int upwards)
{
    int i;
    int vy;
    Particle *b, *particles;
    sVector *rand;

    particles = NULL;
    for (i = 0; i < n; i++) {
        b = alloc_particle();
        if (b != NULL) {
            b->p = zero;
            rand = rand_vect();
            vy = rand->y;
            if (upwards) {
                vy <<= 1;
                if (vy < 0)
                    vy = -vy;
            }
            b->life = life - (int)ABS(vy >> 2);
            b->v.x = MUL(rand->x, speed) * (int32)frame_time;
            b->v.y = MUL(vy, speed) * (int32)frame_time;
            b->v.z = MUL(rand->z, speed) * (int32)frame_time;
            b->p.x += b->v.x;
            b->p.y += b->v.y;
            b->p.z += b->v.z;
            if (particles != NULL)
                particles->prev = b;
            b->next = particles;
            b->prev = NULL;
            particles = b;
        }
    }
    return particles;
}

static sObject *make_bang(sVector *pos, Class *class, int n, int speed, int life, int upwards, int *cols)
{
    sObject *o;
    Bangs *bang;

    o = new_object(class);
    if (o != NULL) {
        bang = o->data;
        bang->count = 0;
        bang->p = *pos;
        bang->particles = start_bang(n, speed, life, upwards);
        bang->cols = cols;
    }
    attach_object(o);

    return o;
}

/* Spherical Bangs */
sObject *big_bang(sVector *pos)
{
    return make_bang(pos, Bangs_class, BIG_BANG_N, BANG_SPEED1, BANG_LIFE1, FALSE, bang_cols1);
}

sObject *medium_bang(sVector *pos)
{
    return make_bang(pos, Bangs_class, MEDIUM_BANG_N, BANG_SPEED2, BANG_LIFE2, FALSE, bang_cols2);
}

sObject *small_bang(sVector *pos)
{
    sObject *bang;

    bang = make_bang(pos, Bangs_class, SMALL_BANG_N, BANG_SPEED3, BANG_LIFE2, FALSE, bang_cols2);

    return bang;
}

sObject *big_ground_bang(sVector *pos)
{
    sObject *bang;

    bang = make_bang(pos, GroundBangs_class, BIG_BANG_N, BANG_SPEED1, BANG_LIFE1, TRUE, bang_cols1);

    return bang;
}

sObject *medium_ground_bang(sVector *pos)
{
    sObject *bang;

    bang = make_bang(pos, GroundBangs_class, MEDIUM_BANG_N, BANG_SPEED2, BANG_LIFE2, TRUE, bang_cols2);

    return bang;
}

sObject *small_ground_bang(sVector *pos)
{
    sObject *bang;

    bang = make_bang(pos, GroundBangs_class, SMALL_BANG_N, BANG_SPEED3, BANG_LIFE2, TRUE, bang_cols2);

    return bang;
}

static void reset_bang(sObject *self)
{
    Bangs *bang;

    bang = self->data;
    bang->count      = 0;
    bang->radius     = (int)Meter(10);
    bang->cols       = bang_cols1;
    bang->particles  = NULL;
}

static void init_bangs(Class *c)
{
    more_particles(INIT_PARTICLES);
}

void register_bangs(void)
{
    Bangs_class = new_class(sizeof(BangsClass), "Bangs", sizeof(Bangs), World_class);

    Bangs_class->class_init = init_bangs;

    /* Class methods */
    Bangs_class->reset   = reset_bang;
    Bangs_class->destroy = destroy_bang;
    Bangs_class->update  = update_bang;
    Bangs_class->draw    = draw_bang;

    GroundBangs_class = copy_class(Bangs_class, "GroundBangs");

    GroundBangs_class->class_init = NULL;
    GroundBangs_class->draw       = draw_groundbang;
}

