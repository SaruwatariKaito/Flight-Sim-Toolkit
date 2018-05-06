/* --------------------------------------------------------------------------
 * EXEC 		Copyright (c) Simis Ltd 1993,994,1995
 * ---			All Rights Reserved
 *
 * File:		track.c
 * Ver:			1.00
 * Desc:		Player view tracker classes
 *
 * Tracker_class  - Padlock view
 * Tracker_classB - Tower view
 * -------------------------------------------------------------------------- */
#include <stdlib.h>
#include "track.h"
#include "defs.h"
#include "ground.h"

/* Declare Class Pointer */
TrackerClass *Tracker_class = NULL;  /* Padlock view */
TrackerClass *Tracker_classB = NULL; /* Tower view */

static sObject *tracker = NULL;

static bouncy_track = FALSE;

#define TRACK_X M(0)
#define TRACK_Y M(0)
#define TRACK_Z M(80)
#define EYE_HEIGHT M(5)

static void change_view(sObject *tracker, int direction, int elevation)
{
	Tracker *t;
	sVector d;
	sVector r;
	Transdata trans;

	t = tracker->data;
	r.x = elevation;
	r.y = direction;
	r.z = 0;
	setup_trans(&r, &trans);
	d.x = TRACK_X;
	d.y = TRACK_Y;
	d.z = TRACK_Z;
	rot3d(&d, &d, &trans);
    t->offset.x = -d.x;
	t->offset.y = -d.y;
    t->offset.z = -d.z;
}

static void setup_track(sObject *tracker, sObject *tracked, int rotate)
{
	Tracker  *t;
	Mobile *tgt;
	sVector v;
	Transdata trans;

	t   = tracker->data;
	tgt = tracked->data;
	setup_trans(&tgt->r, &trans);
	rot3d(&t->offset, &v, &trans);
	t->p.x = tgt->p.x + v.x;
	t->p.y = tgt->p.y + v.y;
	t->p.z = tgt->p.z + v.z;
	if (rotate) {
  		t->r   = tgt->r;
	} else {
		t->r.x = t->r.y = t->r.z = 0;
	}
	if (t->p.y < EYE_HEIGHT)
	t->p.y = EYE_HEIGHT;
	t->tracked = tracked;
}

static void update_track_tower(sObject *self)
{
	Tracker *tracker;
	Mobile *tgt;
    int32 range, dist;
    sVector d, ad;
	Transdata trans;

	tracker = self->data;
	if (tracker->tracked) {
		tgt = tracker->tracked->data;
		if (tgt) {
    		d.x = tgt->p.x - tracker->p.x;
    		d.y = tgt->p.y - tracker->p.y;
    		d.z = tgt->p.z - tracker->p.z;
    		ad.x = ABS(d.x);
    		ad.y = ABS(d.y);
    		ad.z = ABS(d.z);
    		dist = MAX(ad.x, ad.z);
    		range = MAX(dist, ad.y);
    		if (range < M(15000)) {
        		d.x >>= SPEED_SHIFT;
        		d.y >>= SPEED_SHIFT;
        		d.z >>= SPEED_SHIFT;
				dist >>= SPEED_SHIFT;
        		tracker->r.x = SUBANGLE(fast_atan(d.y, dist), DEG(5));
        		tracker->r.y = fast_atan(-d.x, d.z);
			}
		}
	}
}

static void update_track(sObject *self)
{
	Tracker *tracker;
	Mobile *tgt;
  	sVector v, d;
  	Transdata trans;
	int32 ground_height;

	debug("Update tracker\n");
	tracker = self->data;
	if (tracker->tracked) {
		tgt = tracker->tracked->data;
		if (tgt) {
			setup_trans(&tgt->r, &trans);
    		rot3d(&tracker->offset, &v, &trans);
			if (bouncy_track) {
				tracker->p.x += (tgt->wv.x * (int32)frame_time);
    			tracker->p.y += (tgt->wv.y * (int32)frame_time);
    			tracker->p.z += (tgt->wv.z * (int32)frame_time);
				v.x += tgt->p.x;
				v.y += tgt->p.y;
				v.z += tgt->p.z;
				d.x = tracker->p.x - v.x;
				d.y = tracker->p.y - v.y;
				d.z = tracker->p.z - v.z;
				tracker->p.x -= d.x / 7;
				tracker->p.y -= d.y / 7;
				tracker->p.z -= d.z / 7;
				if (d.x > M(16))
					tracker->p.x = v.x + M(14);
				else if (d.x < M(-16))
					tracker->p.x = v.x - M(14);
				if (d.y > M(16))
					tracker->p.y = v.y + M(14);
				else if (d.y < M(-16))
					tracker->p.y = v.y - M(14);
				if (d.z > M(16))
					tracker->p.z = v.z + M(14);
				else if (d.z < M(-16))
					tracker->p.z = v.z - M(14);
			} else {
				tracker->p.x = tgt->p.x + v.x;
				tracker->p.y = tgt->p.y + v.y;
				tracker->p.z = tgt->p.z + v.z;
			}
			tracker->r = tgt->r;
			ground_height = get_terrain_height(tracker->p.x, tracker->p.z);
			if (tracker->p.y < ground_height+EYE_HEIGHT)
				tracker->p.y = ground_height+EYE_HEIGHT;
		}
	}
}


/* -------------------
 * GLOBAL track_viewer
 * 
 * Called when the tracker receives the view focus or
 * when look_XXX changes
 */
void track_viewer(sObject *o)
{
  change_view(o, look_direction, look_elevation);
  change_cockpit(NULL);
  set_viewer(o);
}


/* -------------------
 * METHOD kill_track
 * 
 */
void kill_track(sObject *o)
{
  /* The tracker can not be killed *
   */
    ;
  debug("attempt to kill tracker\n");
}


/* --------------------
 * METHOD draw_track
 * 
 */ 
int draw_track(sObject *o)
{
  /* The tracker is not drawn
   */
    return FALSE;
}


/* --------------------
 * METHOD track_detaching
 * 
 */ 
static void track_detaching(sObject *self, sObject *old)
{
	Tracker *o;

    o = self->data;
	if (o->tracked == old) {
		o->tracked = NULL;
	}
}

void register_track(void)
{
    Tracker_class = new_class(sizeof(TrackerClass), "Tracker", sizeof(Tracker), Mobile_class);
	Tracker_class->update = update_track;
	Tracker_class->change_view = change_view;
	Tracker_class->check_collide = NULL;
	Tracker_class->kill   = kill_track;
	Tracker_class->draw   = draw_track;
	Tracker_class->detaching = track_detaching;

	add_message(Tracker_class, "become_viewer", 0, track_viewer);

	Tracker_classB = copy_class(Tracker_class, "TrackerB");
	Tracker_classB->update = update_track_tower;
}

/* -------------------
 * GLOBAL set_track_view
 * 
 * Create a tracker object, trailing specified object
 * change view to new track object
 * 
 */
void set_track_view(sObject *tracked)
{
	Tracker *t;

	/* If tracker does not exist then create */
	if (tracker == NULL) {
		tracker = new_object(Tracker_class);
		t = tracker->data;
    	t->shape    = NULL;
        attach_object(tracker);
	}
	if (tracker) {
		t = tracker->data;
		t->offset.x = 0L;
		t->offset.y = 0L;
		t->offset.z = M(-80);
		look_direction = 0;
		look_elevation = 0;
		tracker->class = Tracker_class;
		setup_track(tracker, tracked, TRUE);
 		change_view(tracker, look_direction, look_elevation);
  		change_cockpit(NULL);
  		set_viewer(tracker);
	}
}


/* -------------------
 * GLOBAL set_outside_view
 * 
 * Create a tracker object, fixed in space tracking the specified object
 * change view to new track object
 */
void set_outside_view(sObject *tracked)
{
	Tracker *t;

	/* If tracker does not exist then create */
	if (tracker == NULL) {
		tracker = new_object(Tracker_classB);
		t = tracker->data;
    	t->shape    = NULL;
        attach_object(tracker);
	}
	if (tracker) {
		t = tracker->data;
		t->offset.x = M(20L);
		t->offset.y = M(5L);
		t->offset.z = M(-50);
		t->r.x = t->r.y = t->r.z = 0;
		look_direction = 0;
		look_elevation = 0;
		tracker->class = Tracker_classB;
		setup_track(tracker, tracked, FALSE);
 		change_view(tracker, look_direction, look_elevation);
  		change_cockpit(NULL);
  		set_viewer(tracker);
	}
}
