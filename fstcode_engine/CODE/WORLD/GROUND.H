#ifndef ground_h
#define ground_h

#include "world.h"

/* world_objects - defined in world.c
 * list of all objects (class or subclass World)
 * which are 'attached' to world
 */
extern sList *world_objects;

/* add_to_tp()
 * Called from world.c when an object is added to world
 * or moves within world
 * returns TRUE if successful */
extern int add_to_tp(sObject *o, int force_draw);

/* remove_from_tp()
 * Called from world.c when an object moves or is 'detached'
 * from world */
extern void remove_from_tp(sObject *o);

/* move_in_tp()
 * Called from world.c when an object moves within world
 * returns TRUE if still in terrain */
extern int move_in_tp(sObject *o, int force_draw);

extern int draw_sea_on;
extern int draw_ground_on;
extern int ground_grid_on, ground_grid_size;

extern void ground_view_changed(sObject *view);
extern void draw_map_object(sObject *o, int colour);
extern void draw_ground_map(sObject *view);
extern void draw_ground(sObject *view, int dir);
extern void startup_ground(sObject *view);
extern void setup_ground(void);
extern void init_ground(void);
extern int32 get_terrain_height(int32 x, int32 z);
extern sList* get_tp_objects(sObject *o);
extern int check_intervisibility(sVector *p1, sVector *p2);

#endif
