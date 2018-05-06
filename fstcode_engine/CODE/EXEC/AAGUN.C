/* --------------------------------------------------------------------------
 * EXEC 		Copyright (c) Simis Ltd 1993,994,1995
 * ---			All Rights Reserved
 *
 * File:		aagun.c
 * Ver:			2.00
 * Desc:		AAGun class
 *
 * Anti Aircraft gun - gun attempts to aquire target from opposite team,
 * when aquired gun will fire bullets at object.
 *
 * -------------------------------------------------------------------------- */
#include <stdlib.h>
#include "aagun.h"
#include "bullets.h"
#include "explo.h"
#include "defs.h"

#define SHELL_SPEED FIXSPD(1200)

/* Declare Class Pointer */
AAGunClass *AAGun_class = NULL;


/* -------------------
 * GLOBAL aim_gun
 *
 * gun must be a sub-class of World
 * target must be a sub-class of Mobile
 *
 */
int32 aim_gun(sObject *gun, sObject *target, int rot, int fixed)
{
	int32 range, dist, offset;
	int time;
	sVector d, ad, p;
	sVector r;
	World *o;
	Mobile *t;

	o = gun->data;
	t = target->data;
	d.x = t->p.x - o->p.x;
	d.y = t->p.y - o->p.y;
	d.z = t->p.z - o->p.z;
	ad.x = ABS(d.x);
	ad.y = ABS(d.y);
	ad.z = ABS(d.z);
	if (ad.x >= ad.z)
		dist = (ad.x + ad.z) - (ad.z >> 1);
	else
		dist = (ad.x + ad.z) - (ad.x >> 1);
	range = MAX(dist, ad.y);
	if (dist < M(2000)) {
		range = magnitude(&d);
		time = (int)DIV(range, SHELL_SPEED + t->speed);
		/* predict target position */
		d.x += MUL(t->wv.x, time);
		d.y += MUL(t->wv.y, time);
		d.z += MUL(t->wv.z, time);
		ad.x = ABS(d.x);
		ad.y = ABS(d.y);
		ad.z = ABS(d.z);
		if (ad.x >= ad.z)
			dist = (ad.x + ad.z) - (ad.z >> 1);
		else
			dist = (ad.x + ad.z) - (ad.x >> 1);
		range = magnitude(&d);
		time = (int)DIV(range, SHELL_SPEED + t->speed);

		d.y += MUL(GRAVITY, (time * time) / 2);
		d.y += M(1);
		r.x = fast_atan(d.y, dist);
		if (fixed)
			r.y = o->r.y;
		else
			r.y = fast_atan(-d.x, d.z);
		r.z = 0;
		if (r.x < DEG(1))
    		r.x = DEG(1);
		else if (r.x > DEG(60))
    		r.x = DEG(60);

		if (rot)
    		o->r.y = r.y;

		offset = M(10);
		dist = o->p.y;
		o->p.y += M(5);
		fire_bullet(gun, &r, offset, SHELL_SPEED, time+(time>>3), TRUE);
		o->p.y = dist;
	}
	return range;
}

void update_aagun(sObject *self)
{
    int o_team;
    int32 range;
    sVector d;
	AAGun *o;
	World *tgt;

	o = self->data;
	if (o->shape == o->dead) {
        o->timer = SECS(20);
		return;
	}

    if (o->count <= 0) {
        o->reload = SECS(o->reload_time);
        o->count = o->burst_size;
		return;
    }
	if (o->reload > 0) {
		o->reload -= frame_time;
		return;
	}
	if ((o->rate -= frame_time) <= 0) {
		o->rate = o->burst_delay;
        o->count -= 1;
		if ((o->target == NULL) || (total_time > o->next_target_time)) {
			o->next_target_time = total_time + SECS(5);
			o->target = get_nearest_target(self, TRUE, M(2500), TRUE);
		}
		if (o->target) {
			range = aim_gun(self, o->target, TRUE, o->fixed);
			ground_target(self, range);
		}
    }
}

static void aagun_detaching(sObject *self, sObject *old)
{
	AAGun *o;

    o = self->data;
	if (o->target == old) {
		o->target = NULL;
	}
}

/* Method used to read in an inital world state */
void file_read(sObject *self, FILE *fp)
{
	Basic_data data;
    AAGun *o;
	float freq;
	int   b_rate, b_time;

    o = self->data;

	file_read_basic(&data, fp);

	/* class data - none */

	o->p.x = M(data.x); o->p.y = M(data.y); o->p.z = M(data.z);
	o->flags = data.f;
	o->r.y = DEG(data.ry);
    o->alive = o->shape = get_shape(data.shape);
	o->dead  = get_shape(data.dead);
	o->init_strength = o->strength = data.strength;
	set_object_radius(self);

	if (world_file_version >= 3) {
		fscanf(fp, "%d %d %d %d", &o->fixed, &b_rate, &b_time, &o->reload_time);
	} else {
		fscanf(fp, "%d %d %d", &b_rate, &b_time, &o->reload_time);
		o->fixed = FALSE;
	}
	if (b_rate > 0 && b_rate <= 100)
		o->burst_delay = 100 / b_rate;
	else
		o->burst_delay = 50;

	if (b_rate > 0 && b_rate <= 100)
		o->burst_size  = b_time * b_rate;
	else
		o->burst_size  = 20;

	/* setup rest of iv's */
	o->count = o->burst_size;
	o->reload = fst_random() << 3; /* 0 - 800 = 0 - 8 seconds */
	o->rate = o->burst_delay;
	o->target = NULL;
	o->next_target_time = 0;
}

static void kill(sObject *o)
{
    AAGun *aa;

    aa = o->data;
	create_explo(aa->p, zero, aa->shape, FIXSPD(8));

	remove_target(o);
	if (aa->dead) {
		aa->shape = aa->dead;
	} else {
    	remove_object(o);
	}
}

static void resupply(sObject *o)
{
    AAGun *aa;

    aa = o->data;
	if (aa->alive) {
		aa->shape = aa->alive;
		aa->strength = aa->init_strength;
	}
}

void register_aagun(void)
{
    AAGun_class = new_class(sizeof(AAGunClass), "AAGun", sizeof(AAGun), Cultural_class);

	AAGun_class->file_read = file_read;
	AAGun_class->update    = update_aagun;
	AAGun_class->kill	   = kill;
	AAGun_class->detaching = aagun_detaching;

	add_message(AAGun_class, "resupply", 0, resupply);
}

