/* --------------------------------------------------------------------------
 * EXEC 		Copyright (c) Simis Ltd 1993,994,1995
 * ---			All Rights Reserved
 *
 * File:		brick.c
 * Ver:			2.00
 * Desc  		Flying brick player for database exploration
 *
 * -------------------------------------------------------------------------- */

#include <stdlib.h>
#include "ground.h"
#include "brick.h"
#include "defs.h"
#include "bangs.h"
#include "explo.h"
#include "bullets.h"
#include "rocket.h"

#include "sound.h"

static sSymbol *become_viewer;

/* Declare Class Pointer */
BrickClass *Brick_class = NULL;

extern int pitchin, yawin, rollin, throttlein;
#define EYE_HEIGHT 0L

static void set_globals(Brick *o)
{
	int temp;

	aircraft_asi = KNOTS(o->speed);
	aircraft_vsi = o->wv.y;
	aircraft_alt = (int)((((o->p.y - EYE_HEIGHT) >> SPEED_SHIFT) * 19L) / 48L);
	aircraft_heading = 360 - ((unsigned short)o->r.y / DEG_1);
	if (aircraft_heading == 360)
		aircraft_heading = 0;
}

static void update_brick(sObject *self)
{
    sObject *obj;
    sVector v;
    sVector dr;
    Transdata trans;
	Brick *o;

	if (self != Player)
		return;

	o = self->data;
    dr.x = SR(pitchin * frame_time,  2);
    dr.y = SR(yawin * frame_time,  3);
    dr.z = (rollin * frame_time);
    o->dr.x = (int)SR(MUL(o->dr.x, 6), 4);
    o->dr.y = (int)SR(MUL(o->dr.y, 6), 4);
    o->dr.z = (int)SR(MUL(o->dr.z, 6), 4);
    ac_axis_rotate(&o->r, &dr);
	/* roll/yaw coupling */
	if (!o->autostab_on)
		o->r.y = SUBANGLE(o->r.y, ((((int32)SIN(o->r.z) << 10) >> SSHIFT) * frame_time) >> 5);
    setup_trans(&o->r, &trans);
    o->speed = (throttlein * ABS(throttlein)) >> 3;
    v.x = 0; v.y = 0; v.z = o->speed;
    rot3d(&v, &o->wv, &trans);
    o->op = o->p;
    o->p.x += MUL(o->wv.x, frame_time);
    o->p.y += MUL(o->wv.y, frame_time);
    o->p.z += MUL(o->wv.z, frame_time);
    if (o->p.y <= EYE_HEIGHT) {
        o->p.y = EYE_HEIGHT;
        /* ac_landed = TRUE; */
        if (o->wv.y < 0) {
            o->wv.y = 0;
            /* aircraft_vsi = 0; */
        }
        if ((short)o->r.x < 0) {
            o->r.x = 0;
            o->dr.x = 0;
        }
        o->r.z = 0;
    }
	o->ground_height = get_terrain_height(o->p.x, o->p.z);
	if (o->p.y < (o->ground_height + M(1)))
		o->p.y = o->ground_height + M(1);
	set_globals(o);
}

/* Method used to read in an inital world state */
static void file_read(sObject *self, FILE *fp)
{
	Basic_data data;
    Brick *o;
	char cockpit[32];

    o = self->data;

	file_read_basic(&data, fp);

	/* class data */
    fscanf(fp, "%s\n", cockpit);

	o->p.x = M(data.x); o->p.y = M(data.y); o->p.z = M(data.z);
	o->flags = data.f;
	o->r.y = DEG(data.ry);
    o->shape = get_shape(data.shape);
	o->speed = FIXSPD(200);
	o->cockpit = read_gauges(cockpit);
	o->autostab_on = FALSE;
	set_object_radius(self);
}

static int draw_cockpit(sObject *o)
{
	Brick *b;

	b = o->data;
 	if (look_direction == 0 && look_elevation == 0) {
      draw_gauges(b->cockpit);
		return TRUE;
	}
	return FALSE;
}

/* --------------------------------------------
 * Method called every time a variable
 * affecting the state of the viewer is changed,
 * look_direction, look_elevation
 */
void brick_viewer(sObject *o)
{
  Brick *b;

  b = o->data;
  if (look_direction == 0 && look_elevation == 0) {
	set_cockpit_state(b->cockpit, draw_full_cockpit);
  	change_cockpit(b->cockpit);
  } else {
    change_cockpit(NULL);
  }

  set_viewer(o);
}

static void kill_brick(sObject *o)
{
  sSymbol *s;
  Brick *brick;

  big_bang(Posn(o));
  s = find_symbol(object_symbol_table, "Tracker");
  if (s && s->value) {
	  brick = o->data;
      message(s->value, become_viewer);
      create_explo(brick->p, brick->wv, brick->shape, FIXSPD(8));
	  remove_object(o);
	  if (o == Player) {
		player_symbol->value = s->value;
	  }
	  die_sound();
  }
}

static int brick_autopilot(sObject *self)
{
	Brick *o;

	o = self->data;
	o->autostab_on = !o->autostab_on;
	return TRUE;
}

static int fire1_control(sObject *self)
{
	Brick *o;

	o = self->data;
	cannon_noise();
	fire_bullet(self, &o->r, M(5), o->speed+FIXSPD(1200), SECS(20), TRUE);

	return TRUE;
}

static int draw(sObject *self)
{
    Brick *o;
	sVector p;

	o = self->data;
    if (o->shape) {
		p.x = o->p.x;
		p.y = o->ground_height;
		p.z = o->p.z;
        draw_shadow((sShape_def *)o->shape, &p, &o->r);
        return(draw_shape_def((sShape_def *)o->shape, &o->p, &o->r, TRUE));
    }
	return FALSE;
}

void register_brick(void)
{
    Brick_class = new_class(sizeof(BrickClass), "Brick", sizeof(Brick), Mobile_class);

	Brick_class->update    = update_brick;
	Brick_class->file_read = file_read;
	Brick_class->draw_2d   = draw_cockpit;
	Brick_class->kill      = kill_brick;
	Brick_class->draw      = draw;

	add_message(Brick_class, "primary_fire", 0, fire1_control);
	add_message(Brick_class, "become_viewer", 0, brick_viewer);
	add_message(Brick_class, "autopilot_control",  	0, brick_autopilot);

    SetupMessage(become_viewer);
}

sObject *new_brick(int32 x, int32 y, int32 z, sShape_def *sd, sSymbol *cockpit)
{
    sObject *o;
    Brick *m;

    o = new_object(Brick_class);
    m = o->data;
    m->p.x = x;
    m->p.y = y;
    m->p.z = z;
    m->shape = sd;
	m->cockpit = cockpit;
	m->autostab_on = FALSE;

	return o;
}
