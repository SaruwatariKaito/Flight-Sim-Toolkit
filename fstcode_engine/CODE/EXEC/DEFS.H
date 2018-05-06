/* FST exec general header */
#ifndef defs_h
#define defs_h

#include "world.h"
#include "io.h"
#include "symbol.h"

/* ------------------
 * Colour definitions
 * ------------------ */
#define BLACK  0x1a
#define RED    0x10
#define GREEN  0x11
#define BLUE   0x12
#define WHITE  0x13
#define YELLOW 0x14
#define ORANGE 0x15
#define AMBER  0x16
#define D_RED	0x17
#define D_BLUE  0x18
#define BROWN  0x19
#define GREY5  0x1b
#define GREY7  0x1c
#define GREY9  0x1d
#define SHADOW 0x1e

#define SKY 0x2f

/* object flags
 * ------------
 * bits 0-7  reserved for internal use
 * bits 8-15 FST specific
 */
#define RED_TEAM_TYPE 0x0100
#define BLUE_TEAM_TYPE 0x0400
#define RUNWAY_TYPE 0x0200
#define TARGET_TAG  0x1000
#define TEAM_FLAGS (RED_TEAM_TYPE | BLUE_TEAM_TYPE)

typedef struct {
  int n;
  sList *wpts;
  sSymbol *supply;
} Path;

typedef struct {
  sVector p;
  int spd, active;
} Wpt;

typedef struct {
	sVector  p;
	sVector r, wv;
	int32   speed;
} Pylon;


/* main.c
 * ------ */
extern int program_version;
extern int world_file_version;
extern int hires_graphics;
extern int view_base;
extern int look_direction, look_elevation, look_roll;
extern int timer_on;
extern int fast_time;
extern int player_die;
extern int paused;

void show_message(char *s);
void show_messages(void);
#define Player ((sObject*)player_symbol->value)
extern sSymbol *player_symbol;
extern sSymbol *missile_symbol;
extern sSymbol *ejector_symbol;
extern void end_fst(void);

/* load.c
 * ------ */
extern Symbol_Table *object_symbol_table;
extern Symbol_Table *value_symbol_table;
extern Symbol_Table *path_symbol_table;

/* file_read_basic reads the common data defining all
 * objects in FST, should be used in object file_read
 * method prior to class specific reading */
typedef struct {
	long int x, y, z;
	int ry, f, strength;
	char shape[16];
	char dead[16];
} Basic_data;
extern void file_read_basic(Basic_data *bd, FILE *fp);

extern int load_worldfile(char *name);

/* control.c
 * --------- */
extern int draw_full_cockpit;
extern int padlock_view_on;
extern int target_track_on;

/* Macro used in setup functions, M is name of message and
 * name of local Symbol* variable */
#define SetupMessage(M) M = get_symbol(message_symbol_table, #M);

extern void set_controler(sObject *o);
extern void control_set_viewer(sObject *self);
extern void set_view_orientation(int abs, int direction, int elevation);

/* gauges.c
 * -------- */
/* Globals */
extern int aircraft_alt;
extern int aircraft_asi;
extern int aircraft_vsi;
extern int aircraft_rpm;
extern int aircraft_heading;
extern int aircraft_slip, aircraft_roll, aircraft_pitch, aircraft_g, aircraft_aoa;
extern int aircraft_fuel, gear_down, brakes_on, airbrake_on;
extern int aircraft_stalled, low_fuel, engine_fail, hyd_fail, radar_fail, hud_fail, electric_fail;
extern int aircraft_flap_angle;
extern int aircraft_damage;
extern int aircraft_grounded;

extern int panel_height;

/* externs : */
extern sSymbol* read_gauges(char *file_name);
extern void reset_gauges(void);
extern void setup_gauges(void);
extern void init_gauges(void);
extern void tidyup_gauges(void);
extern void draw_gauges(sSymbol *cs);
extern void draw_hud_ccip(sObject *o, sVector *ccip);
extern void set_cockpit_state(sSymbol *cs, int state);
extern void change_cockpit(sSymbol *cs);

/* menu.c
 * ------ */
extern void alert_box(int cx, int cy, char *message);
extern void process_menus(void);
extern void setup_menus(void);
extern void load_menu_defaults(void);

/* inter.c
 * ------- */

#define WT_GUN 		0
#define WT_ROCKET	1
#define WT_AA		2
#define WT_AG		3
#define WT_BOMB		4
#define WT_CLSTR	5

/* Weapon track type */
#define W_NOTRACK 	1
#define W_HEAT 		2
#define W_GROUND 	3

extern int 	selected_range;
extern sObject *selected;
extern void check_targets(void);
extern void select_target(sObject *o, int weapon_type);
extern void air_target(sObject *o, int32 range);
extern void ground_target(sObject *o, int32 range);
extern void remove_target(sObject *);
extern sObject *get_nearest_target(sObject *targeter, int air_flag, int32 max_range, int include_player);
extern int over_shape(sVector *p, sShape *s, sVector *sp, sVector *sr);
extern void remove_supported(sObject *self);
extern void clear_selection(void);
extern void reset_inter(void);
extern void setup_inter(void);

extern int weapon_type;
extern int weapon_lock;
extern int weapon_n;
extern int weapon_track;

extern sVector ccip;
extern void inter_cycle_weapon(sObject *self);
extern void set_weapon_selected(int type);

/* plane.c */
extern int calc_stall_speed(sVector2 *points, float wing_eff, float stall_angle, int real_weight);
extern int calc_top_speed(sVector2 *points, int real_thrust, float drag_factor, int real_weight);

#endif
