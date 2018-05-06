#ifndef track_h
#define track_h

#include "wdefs.h"
#include "mobile.h"

/* Class      : Tracker
 * Superclass : Mobile
 *
 */

/* Define Class and Instance Templates */

#define Tracker_Methods Mobile_Methods \
	void (*change_view)(sObject *self, int direction, int elevation);

#define Tracker_Vars Mobile_Vars \
	sObject *tracked;		   \
	sVector offset;


/* Declare Class Structure typedefs */
typedef struct {
    Tracker_Methods
} TrackerClass;

typedef struct {
    Tracker_Vars
} Tracker;


/* Class Externs */
extern TrackerClass *Tracker_class;
extern void register_track(void);

extern sObject *new_track(sObject *tracked, long int dx, long int dy, long int dz);
extern void set_track_view(sObject *tracked);
extern void set_outside_view(sObject *tracked);
#endif
