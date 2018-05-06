/* Definitions and externals for world library */

#ifndef world_h
#define world_h

#include <stdlib.h>

#include "graph.h"

#include "wdefs.h"

#include "mem.h"
#include "lists.h"
#include "symbol.h"
#include "shape.h"

#include "objects.h"

/* --------------------------------------------------------------------------
 * World Class Definition
 * ======================
 *
 * Class      : World
 * Superclass : Base
 *
 */

/* Define Class and Instance Templates */

#define World_Methods Base_Methods                \
    /* Update Methods */                          \
    int  (*detach)(sObject *self);                 \
    int  (*attach)(sObject *self);                 \
    void (*system_update)(sObject *self);          \
    void (*update)(sObject *self);                 \
    /* Drawing Methods */                         \
    int  (*draw)(sObject *self);                   \
    int  (*draw_2d)(sObject *self);                \
    /* Collision/Event Methods */                 \
    sObject* (*check_collide)(sObject *self);       \
    int  (*hit)(sObject *self, sObject *o, int kp); \
    int  (*collide)(sObject *self, sObject *c);     \
    void (*kill)(sObject *self);					  \
    void (*detaching)(sObject *self, sObject *old);

#define World_Vars Base_Vars	\
    int timer;					\
    sVector r;					\
    unsigned int radius, flat_radius;	\
	void *tp;					\
	int strength;				\
    sShape_def *shape;


/* Declare Class Structure typedefs */
typedef struct {        \
    World_Methods       \
} WorldClass;           \
                        \
typedef struct {        \
    World_Vars          \
} World;


/* Class Externs */
extern WorldClass *World_class;
extern void register_World(void);

extern sObject *new_world_obj(long int x, long int y, long int z, sShape_def *sd);

#define WorldMethod(O, M) Method(O, World, M)

/* -------------------------------------------------------------------------- */

#define viewer_p Posn(viewer)

/* extern sVector *viewer_p; */

/* Object Flags
 * ------------
 * bits 0-7  World library use
 * bits 8-16 application use
 */
#define ATTACHED  0x01 /* Set if object is attached to world database */
#define DETACHING 0x02 /* Set if object is about to be detached  */
#define DRAW_FIRST 0x04 /* Used by ground.c */
#define MOBILE    0x08 /* Set for Mobile class objects */

/* World Library
 * -------------
 * o Object  : Create/Destroy Attach/Detach
 * o Shape   : Load / Draw
 * o Horizon : Draw / Time of Day - colour control
 * o World   : Draw from viewobj, Update
 * o Collision Detection
 * o Bangs
 * o Bullets
 */

extern void init_world(void);
extern void setup_world(void);
extern void reset_world(void);
extern void tidyup_world(void);

extern void update_world(void);
extern void remove_objects(void);
extern void broadcast_detaching(sObject *old);

/* Adds object to world database immediately */
extern int attach_object(sObject *o);
/* Removes object from world database at end of simulation cycle */
extern void detach_object(sObject *o);
/* Removes object from world database and destroys object at
 * end of simulation cycle */
extern void remove_object(sObject *o);

extern int draw_world(sObject *view, int azimuth, int elevation, int roll,
    void (*ground)(sObject *o, int dir));

extern int view_elevation;

extern void recolour_world(void);

extern void set_object_radius(sObject *self);

#define SHADING_RANGE 7
extern int shading_on;
extern int shaded_ground;
extern int shading_scale;
extern int horizon_polygons;
extern sObject *viewer;
extern void set_viewer(sObject *o);
extern int frame_time;

extern int32 get_range(sVector *p1, sVector *p2, sVector *d);
extern int fst_random(void);

extern sObject *check_collisions(sObject *self);
extern void moving_system_update(sObject *o);

extern int scoring_on;
extern void player_scored_hit(sObject *hitter);
extern void target_scoring(sObject *killed, sObject *killer);
extern void write_score(void);

/* Base method for detaching object from world */
extern int world_detach(sObject *o);

extern int sun_colour;

#endif
