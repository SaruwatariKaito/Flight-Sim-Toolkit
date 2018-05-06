#ifndef weapons_h
#define weapons_h

#include "wdefs.h"
#include "world.h"

/* Class      : Weapons
 * Superclass : World
 *
 */

#define MAX_WEAPON_TYPE 7

#define WT_GUN 		0
#define WT_ROCKET	1
#define WT_AA		2
#define WT_AG       3
#define WT_BOMB     4
#define WT_CLUSTER	5
#define WT_TORPEDO	6

/* Define Class and Instance Templates */

#define Weapons_Methods World_Methods \
	void (*rearm)(sObject *self, int i, int n, int kp);	\
	int  (*fire)(sObject *self);		\
	int  (*cycle)(sObject *self);

#define Weapons_Vars World_Vars \
	sObject *carrier;			\
	int weapon_selected;		\
	int fire_timer;				\
	int current_pylon;			\
	int n[MAX_WEAPON_TYPE];		\
	int kp[MAX_WEAPON_TYPE];	\
	int waiting[MAX_WEAPON_TYPE];


/* Declare Class Structure typedefs */
typedef struct {
    Weapons_Methods
} WeaponsClass;

typedef struct {
    Weapons_Vars
} Weapons;


/* Class Externs */
extern WeaponsClass *Weapons_class;
extern void register_weapons(void);


extern sObject *new_weapons_system(sObject *carrier);

#endif
