/* --------------------------------------------------------------------------
 * EXEC 		Copyright (c) Simis Ltd 1993,994,1995
 * ---			All Rights Reserved
 *
 * File:		control.c
 * Ver:			2.00
 * Desc:		Takes PC joystick, keyboard and mouse inputs (via IO library)
 * 				and passed control into simulation.
 *
 * Keyboard input - triggers events in keyboard_control
 * 				  - cursor keys passes to XXXXXin if keyboard flight controls
 * 				    enabled
 * Mouse input    - passed to XXXXXin if mouse enabled
 *
 * Joystick input - passed to XXXXXin if joystick enabled
 *
 * XXXXXin - pithcin, yawin, rollin, throttlein - control values read by
 * aircraft model, brick model etc.
 *
 * -------------------------------------------------------------------------- */
/* Keyboard and joystick control handling */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "shape.h"
#include "world.h"
#include "ground.h"

#include "ndb.h"
#include "vor.h"
#include "ils.h"
#include "track.h"

#include "keys.h"
#include "pic.h"
#include "io.h"
#include "text.h"
#include "sound.h"

#include "defs.h"

#define HASH 43

static int test_sample = 1;

extern int distance_factor;
extern int fast_time;
extern void select_obi_course(int n, int dg);

int draw_full_cockpit = TRUE;
int padlock_view_on = FALSE;
int target_track_on = FALSE;

/* Externs */
extern int use_save_path;
extern char save_path[80];

extern int draw_string(int x, int y, char *s, int col);
extern int look_direction, look_elevation;
extern void save_fst_screen(void);

#define RUDDER_LEFT COMMA
#define RUDDER_RIGHT DOT
#define ROLL_LEFT PAD_4
#define ROLL_RIGHT PAD_6
#define PITCH_UP PAD_2
#define PITCH_DOWN PAD_8
#define CENTRE_CONTROL PAD_0

/* -----------------------------------------------
 * Raw keycode -> ASCII code conversion table
 * ----------------------------------------------- */
static char ascii_table[0x40] = {
	0,0,'1','2', '3','4','5','6', '7','8','9','0', '_','+',0,0,
	'Q','W','E','R', 'T','Y','U','I', 'O','P','{','}' ,0,'0','A','S',
	'D','F','G','H', 'J','K','L',':' ,'@','~',0,'|', 'Z','X','C','V',
	'B','N','M','<', '>','?',0,'*', 0,' ',0,0, 0,0,0,0,
};


/* Used by tab entry of named object */
static char name[32] = "";

/* Control message symbols
 * -----------------------
 */
static sSymbol *become_viewer = NULL;

sSymbol *autopilot_control	= NULL;
sSymbol *airbrake_control	= NULL;

sSymbol *flaps_control		= NULL;
sSymbol *gear_control		= NULL;
sSymbol *engine_control		= NULL;
sSymbol *wheelbrake_control	= NULL;

sSymbol *flare			= NULL;
sSymbol *drop_chaff			= NULL;
sSymbol *weapon_cycle		= NULL;
sSymbol *primary_fire		= NULL;

sSymbol *jettison_stores		= NULL;
sSymbol *eject				= NULL;


/* PC joystick analog settings */
/* 0 = raw input, 1 = scaled input, 2 = centred input */
int analog_modes[4]    = {2, 2, 0, 1};
int analog_chan[4]     = {1,2,3,4}; /* mappings from logical channels (0-3) to
									 * actual channel+1 (1-4). (thus no map == 0)*/
int analog_polarity[4] = {1,1,1,1};
/* analog joystick card chanels */
static int analog[4] = {0, 0, 0, 0};

/* Selected continuous controller
 * ------------------------------
 */
int analog_on   = TRUE;
int mouse_on	= FALSE;
int keyboard_on = FALSE;
int adc_on		= FALSE;

/* intermediate control channels
 * set by keyboard, mouse, analog...
 *
 * Control mappings
 * ----------------
 * control[0] - analog channel 0 - default rollin
 * control[1] - analog channel 1 - default pitchin
 * control[2] - analog channel 2 - default yawin
 * control[3] - analog channel 3 - default throttlein
 */
static int control[4] = {0, 0, 0, 0};
static int buttons = 0, last_buttons = 0;

#define MAX_PITCH_TRIM 40
#define MIN_PITCH_TRIM -40
#define PITCH_TRIM_STEP 2
static int pitch_trim = 0;

#define MOUSE_RANGE 512
/* Mouse is set to cover -(range/2) to +(range/2) for control use
 * it must be set to screen_width and screen_height for pointer use
 */

/* Global aircraft control variables, set from control array
 * used by object update functions
 */
int pitchin = 0, yawin = 0, rollin = 0, throttlein = 0;

/* Joystick/analog control options
 * -------------------------------
 */
int standard_joystick		= TRUE;  /* analog channels 1 and 2  (of 1-4)*/
int bios_joystick			= FALSE; /* analog channels 1 and 2  (of 1-4)*/
int analog_rudder			= FALSE; /* assume analog chanel 3 (of 1-4) */
int analog_throttle			= FALSE; /* assume analog chanel 4 (of 1-4) */
int thrustmaster_stick		= FALSE; /* channels 1,2 + 3,4 */
int thrustmaster_throttle	= FALSE; /* keyboard events */
int flightstick_pro			= FALSE; /* channels 1,2,4 + buttons */


static void load_joystick_defaults(void);

/* Called by menu code to set control device
 */
void set_control_type(int n)
{
	switch (n) {
        case 0:
		    analog_on 	= TRUE;
			mouse_on 	= FALSE;
			keyboard_on = FALSE;
			adc_on 		= FALSE;
			reset_analog();
			break;
        case 1:
			mouse_on 	= TRUE;
			analog_on 	= FALSE;
			keyboard_on = FALSE;
			adc_on 		= FALSE;
			reset_mouse(MOUSE_RANGE, MOUSE_RANGE);
			break;
        case 2:
			keyboard_on = TRUE;
			analog_on 	= FALSE;
			mouse_on 	= FALSE;
			adc_on 		= FALSE;
			break;
        case 3:
			adc_on 		= TRUE;
		    analog_on 	= FALSE;
			mouse_on 	= FALSE;
			keyboard_on = FALSE;
			reset_adc();
			load_joystick_defaults();
			break;
	}
}

static int max_zoom = 2;
static int default_zoom = 1;
static int zoom_factor  = 1;

static void change_zoom(int dz)
{
	if ((dz > 0) && (zoom_factor < max_zoom)) {
		zoom_factor *= 2;
		zoom_view(zoom_factor);
	}
	if ((dz < 0) && (zoom_factor > 1)) {
		zoom_factor /= 2;
		zoom_view(zoom_factor);
	}
}


/* --------------------
 * LOCAL draw_map
 *
 * Draws in game terrain and object map
 */
static void draw_map(void)
{
	sList *l;
	sObject *o;
	World *w;
	Screen_clip sc;

	sc = clip_region;
	clipping(0, screen_width - 1, 0, screen_height - 1);
	clear_prim(0, screen_height - 1, COL(0));
	draw_ground_map(viewer);
	for (l = world_objects->next; l != world_objects; l = l->next) {
    	o = (sObject*)l->item;
		if (o != Player) {
			w = o->data;
			if (w->flags & RED_TEAM_TYPE)
				draw_map_object(o, RED);
			else if (w->flags & BLUE_TEAM_TYPE)
				draw_map_object(o, BLUE);
			else
				draw_map_object(o, GREY9);
		}
	}
	draw_map_object(Player, WHITE);
	swap_screen();
	wait_ascii();
	clipping(sc.left, sc.right, sc.top, sc.bottom);
	message(Player, become_viewer);
}

/*-------------
 * View Control
 * ------------ */

void set_view_direction(int abs, int direction)
{
	if (abs)
		look_direction = direction;
	else
		look_direction = ADDANGLE(look_direction, direction);
	message(viewer, become_viewer);
}

void set_view_elevation(int abs, int elev)
{
	if (abs)
		look_elevation = elev;
	else
		look_elevation = ADDANGLE(look_elevation, elev);
	message(viewer, become_viewer);
}

void set_view_orientation(int abs, int direction, int elevation)
{
	if (abs) {
		look_direction = direction;
		look_elevation = elevation;
	} else {
		look_direction = ADDANGLE(look_direction, direction);
		look_elevation = ADDANGLE(look_elevation, elevation);
	}
	message(viewer, become_viewer);
}

void control_set_viewer(sObject *self)
{
	if (self) {
		message(self, become_viewer);
		set_view_orientation(TRUE, DEG(0), DEG(0));
	}
}


/* ------------------------------------------------------------------
 * Joystick calibration procedures
 * ------------------------------------------------------------------ */

/* -------------------
 * GLOBAL calibrate_joystick
 *
 * Opens joystick calibration window and calls IO library
 * callibration function.
 */
void calibrate_joystick(void)
{
    Screen_clip s;

    s = clip_region;
	use_font("FONT57");
    clipping(0, screen_width - 1, 0, screen_height - 1);
	merge_screen();
	calibrate_analog(analog_modes, WHITE, GREY7, GREY9, GREY5);
	escape_flag = FALSE;
	split_screen();
    clipping(s.left, s.right, s.top, s.bottom);
	message(viewer, become_viewer);
}

void setup_analog_useage(void)
{
	/* setup analog channel mode use depending on current input
	 * devices selected */

	analog_modes[0] = analog_modes[1] = analog_modes[2] = analog_modes[3] = 0;

	if (standard_joystick || bios_joystick) {
		/* Set logical channels 1,2 to scaled input */
        analog_modes[analog_chan[0] - 1] = 2;
        analog_modes[analog_chan[1] - 1] = 2;
	} else if (thrustmaster_stick) {
        analog_modes[analog_chan[0] - 1] = 2;
        analog_modes[analog_chan[1] - 1] = 2;
        analog_modes[analog_chan[3] - 1] = 1; /* scaled input */
	}

	if (analog_rudder) {
        analog_modes[analog_chan[2] - 1] = 2;
	}

    if (analog_throttle) {
        analog_modes[analog_chan[3] - 1] = 1;
	}
}

void save_joystick_defaults(void)
{
	FILE *fp;
	int min[4], max[4];
	char file_name[100];

	sprintf(file_name, "%sJOYSTICK.CFG", save_path);
	fp = fopen(file_name, "w");
	if (fp != NULL) {
		fprintf(fp, "CHANNELS %d %d %d %d\n", analog_chan[0], analog_chan[1], analog_chan[2], analog_chan[3]);
		fprintf(fp, "POLARITY %d %d %d %d\n", analog_polarity[0], analog_polarity[1], analog_polarity[2], analog_polarity[3]);
		get_analog_limits(min, max);
		fprintf(fp, "MINIMA %d %d %d %d\n", min[0], min[1], min[2], min[3]);
		fprintf(fp, "MAXIMA %d %d %d %d\n", max[0], max[1], max[2], max[3]);
		fprintf(fp, "STANDARD %d %d %d %d\n", standard_joystick, bios_joystick, analog_rudder, analog_throttle);
		fprintf(fp, "THRUSTMASTER %d %d\n", thrustmaster_stick, thrustmaster_throttle);
		fprintf(fp, "FLIGHTSTICK_PRO %d\n", flightstick_pro);
		fclose(fp);
	}
}

static void load_joystick_defaults(void)
{
	FILE *fp;
	int min[4], max[4];
	char s[40];
	char file_name[100];

	sprintf(file_name, "%sJOYSTICK.CFG", save_path);
	fp = fopen(file_name, "r");
	if (fp != NULL) {
		fscanf(fp, "%s %d %d %d %d\n", s, &analog_chan[0], &analog_chan[1], &analog_chan[2], &analog_chan[3]);
		fscanf(fp, "%s %d %d %d %d\n", s, &analog_polarity[0], &analog_polarity[1], &analog_polarity[2], &analog_polarity[3]);
		fscanf(fp, "%s %d %d %d %d\n", s, &min[0], &min[1], &min[2], &min[3]);
		fscanf(fp, "%s %d %d %d %d\n", s, &max[0], &max[1], &max[2], &max[3]);
		set_analog_limits(min, max);
		fscanf(fp, "%s %d %d %d %d\n", s, &standard_joystick, &bios_joystick, &analog_rudder, &analog_throttle);
		fscanf(fp, "%s %d %d\n", s, &thrustmaster_stick, &thrustmaster_throttle);
		fscanf(fp, "%s %d\n", s, &flightstick_pro);
		fclose(fp);
	}
}


/* Scalling table for mapping 0-128 raw data to
 * full range 0 - 540
 */
#define MAX_CONTROL 128
static int control_table[MAX_CONTROL] = {
    0,0,0,1,1,2,2,3,4,5,
    6,7,8,9,10,12,13,14,16,18,
    19,21,23,25,27,29,31,33,36,38,
    40,43,45,48,50,53,56,59,62,64,
    67,71,74,77,80,83,87,90,94,97,
    101,105,108,112,116,120,124,128,132,136,
    140,144,149,153,157,162,166,171,175,180,
    185,190,194,199,204,209,214,219,225,230,
    235,240,246,251,257,262,268,273,279,285,
    291,296,302,308,314,320,326,332,339,345,
    351,358,364,370,377,383,390,397,403,410,
    417,424,431,438,445,452,459,466,473,480,
    488,495,502,510,517,525,532,540
};

static int scale_control(int control)
{
    if (control >= 0) {
        if (control >= MAX_CONTROL)
            control = MAX_CONTROL - 1;
        control = control_table[control];
    }
    else {
        if (control <= -MAX_CONTROL)
            control = -MAX_CONTROL + 1;
        control = -control_table[-control];
    }
    return control;
}


/* Stick control */
/* ============= */

/* setup control[], from analog channels */

/* --------------------
 * LOCAL analog_control
 *
 * Sets control value array depending upon
 * setup of analog_chan and XXX_joystick options
 *
 * Implements stick hat events (changing view direction)
 */
static void analog_control(void)
{
	static int last_dir = 0;
	int c4, dir;

	/* work out how many channels need reading */
	if ((standard_joystick || bios_joystick) &&
			!(analog_rudder || analog_throttle || thrustmaster_stick)) {
		buttons |= read_analog2(analog);
	} else {
		buttons |= read_analog(analog_modes, analog);
	}

	if (standard_joystick || bios_joystick || thrustmaster_stick) {
    	if (analog_chan[0] > 0)
        	control[0] = scale_control(analog[analog_chan[0] - 1] * analog_polarity[0]);
    	if (analog_chan[1] > 0)
        	control[1] = scale_control(analog[analog_chan[1] - 1] * analog_polarity[1]);
	}

	if (analog_rudder) {
    	if (analog_chan[2] > 0)
        	control[2] = -scale_control(analog[analog_chan[2] - 1] * analog_polarity[2]);
	}

	/* control[3] : 0 - 256 */
	if (analog_throttle) {
		if (analog_chan[3] > 0) {
			if (analog_polarity[3] >= 0)
        		control[3] = (256 - analog[analog_chan[3] - 1]) + 1;
			else
        		control[3] = (analog[analog_chan[3] - 1]) + 1;
		}
	}

	dir = 0;
	/* Hat on FLIGHTSTICK PRO */
	if (flightstick_pro && (buttons != 0)) {
		if (buttons == 0x0f)
			dir = 1;
		else if (buttons == 0x0b)
			dir = 2;
		else if (buttons == 0x07)
			dir = 3;
		else if (buttons == 0x03)
			dir = 4;
	}
	if (thrustmaster_stick) {
		c4 = analog[3];
		if (c4 > 224)
			dir = 1;
		else if (c4 < 32)
			dir = 2;
		else if ((c4 > 32) && (c4 < 96))
			dir = 3;
		else if ((c4 > 96) && (c4 < 160))
			dir = 4;
		else if ((c4 > 160) && (c4 < 224))
			dir = 0;
	}
	if (dir && (dir != last_dir)) {
		switch (dir) {
			/* Push up - Front and centre */
        	case 1: if (look_direction == 0) {
						draw_full_cockpit = !draw_full_cockpit;
		   		 		message(viewer, become_viewer);
					} else {
						set_view_direction(TRUE, DEG(0));
					}
					break;

			/* Push right */
        	case 2: set_view_direction(FALSE, DEG(-60)); break;

			/* Push left */
        	case 4: set_view_direction(FALSE, DEG(60));  break;

			/* Push down - Look back */
        	case 3:
				set_view_direction(TRUE, DEG(180));
				break;

			default:
				break;
		}
	}
	last_dir = dir;
}

/* Read from ADC card */
static void adc_control(void)
{
	buttons |= read_adc(analog_modes, analog) & 0xff;

    if (analog_chan[0] > 0)
        control[0] = scale_control(analog[analog_chan[0] - 1] * analog_polarity[0]);
    if (analog_chan[1] > 0)
        control[1] = scale_control(analog[analog_chan[1] - 1] * analog_polarity[1]);

	if (analog_rudder) {
    	if (analog_chan[2] > 0)
        	control[2] = -scale_control(analog[analog_chan[2] - 1] * analog_polarity[2]);
	}

	/* control[3] : 0 - 256 */
	if (analog_throttle) {
		if (analog_chan[3] > 0) {
			if (analog_polarity[3] >= 0)
        		control[3] = (256 - analog[analog_chan[3] - 1]) + 1;
			else
        		control[3] = (analog[analog_chan[3] - 1]) + 1;
		}
	}
}


static void mouse_control(void)
{
	int mx, my;

	buttons |= read_mouse(&mx, &my);
	control[0] = scale_control(SR(mx , 1));
	control[1] = scale_control(SR(my , 1));
}

static int get_scan_control(int val, int step, int restore, int increase, int decrease)
{
	if (val < 0) {
		if (key_down(increase)) {
			val = step;
		} else if (key_down(decrease)) {
			if (val > -MAX_CONTROL)
				val -= step;
		} else {
			val += restore;
			if (val > 0)
				val = 0;
		}
	} else {
		if (key_down(decrease)) {
			val = -step;
		} else if (key_down(increase)) {
			if (val < MAX_CONTROL)
				val += step;
		} else {
			val -= restore;
			if (val < 0)
				val = 0;
		}
	}
	return val;
}


/* -------------------
 * LOCAL scan_control
 *
 * Uses key state table for continuous controls
 */
static void scan_control(int code)
{
	static int val1 = 0, val2 = 0, val3 = 0;
	int step, restore;

	step = frame_time;
	restore = step * 3;

	/* Rudder control
	 * -------------- */
	if (!key_down(RIGHT_SHIFT_KEY)) {
		val3 = get_scan_control(val3, step, restore, RUDDER_LEFT, RUDDER_RIGHT);
    	control[2] = scale_control(val3);
	}
	if (key_down(SPACE))
        buttons |= 1;
    if (key_down(ENTER))
        buttons |= 2;
	/* Throttle control may also be from normal key input */
	if (key_down(EQUALS)) {
		if (!(key_down(LEFT_SHIFT_KEY)) &&
			!(key_down(RIGHT_SHIFT_KEY)))
	        throttlein += 1;
	}
	if (key_down(MINUS)) {
		if (!(key_down(LEFT_SHIFT_KEY)) &&
			!(key_down(RIGHT_SHIFT_KEY)))
    	    throttlein -= 1;
	}
	key_events = 0;

	/* Stick control
	 * -------------- */
	if (keyboard_on) {
		if (key_down(CENTRE_CONTROL))
			val1 = val2 = 0;
		val1 = get_scan_control(val1, step, restore, ROLL_RIGHT, ROLL_LEFT);
		val2 = get_scan_control(val2, step, restore, PITCH_UP, PITCH_DOWN);
	    control[0] = scale_control(val1);
    	control[1] = scale_control(val2);
	}
}




/* --------------------
 * LOCAL keyboard_control
 *
 * Keyboard input event dispatcher
 */
static void keyboard_control(int code)
{
	sSymbol *s;
    sObject *o;

	switch (code) {
		case ALT(KEY_K): enable_keyboard(); break;

		/* Throttle and Afterburner keys */
		case KEY_1: throttlein = 0;  break;
        case KEY_2: throttlein = 28;  break;
        case KEY_3: throttlein = 56; break;
        case KEY_4: throttlein = 84; break;
        case KEY_5: throttlein = 112; break;
        case KEY_6: throttlein = 140; break;
        case KEY_7: throttlein = 180; break;
        case KEY_8: throttlein = 224; break;
        case KEY_9: throttlein = 240; break;
        case KEY_0: throttlein = 256; break;

		case UNDERSCORE: throttlein = 0; break;
        case PLUS:       throttlein = 256;    break;

/*		case EQUALS: throttlein += 2; break;
		case MINUS:  throttlein -= 2; break;*/

		case SHIFT(COMMA): ((World *)Player->data)->r.y++; break;
		case SHIFT(DOT): ((World *)Player->data)->r.y--; break;

		/* Flight control keys */
		case PAD_PLUS: if (pitch_trim < MAX_PITCH_TRIM) pitch_trim += PITCH_TRIM_STEP; break;
		case PAD_MINUS: if (pitch_trim > MIN_PITCH_TRIM) pitch_trim -= PITCH_TRIM_STEP; break;
		case PAD_TIMES: pitch_trim = 0; break;

        case KEY_A: message(Player, autopilot_control); break;
        case KEY_B: message(Player, airbrake_control); break;

        case KEY_S: message(Player, flaps_control); break;
        case KEY_G: message(Player, gear_control); break;
        case KEY_E: message(Player, engine_control); break;
        case KEY_W: message(Player, wheelbrake_control); break;

		/* Thrustmaster alternative */
        case KEY_M:       message(Player, airbrake_control); break;
        case RIGHTSQUARE: message(Player, flaps_control); break;

		/* Simulaion controls */
        case ALT(KEY_P): paused = TRUE;  break;
        case KEY_Q: engine_noise_on = !engine_noise_on; break;
        case CTRL(KEY_Q): noise_on = !noise_on; break;

		case ALT(KEY_S):
			distance_factor += 1;
			distance_factor = MIN(distance_factor, 3);
			break;
		case CTRL(KEY_S):
			distance_factor -= 1;
			distance_factor = MAX(distance_factor, -3);
			break;

		case CTRL(KEY_G): ground_grid_on = !ground_grid_on; break;
		case CTRL(KEY_1): ground_grid_size = 1; break;
		case CTRL(KEY_2): ground_grid_size = 2; break;
		case CTRL(KEY_3): ground_grid_size = 3; break;
		case CTRL(KEY_4): ground_grid_size = 4; break;

		case ALT(KEY_G): save_fst_screen(); break;
		case ALT(KEY_T): timer_on = !timer_on; break;

		/* Navigation controls */

		case SLASH:
		  scan_NDBs(TRUE);
		  break;
		case SHIFT(SLASH):
		  scan_NDBs(FALSE);
		  break;

		case LEFTSQUARE:
		  select_obi_course(0, +2);
		  break;
		case SHIFT(LEFTSQUARE):
		  select_obi_course(0, -2);
		  break;

		case HASH:
		  select_obi_course(1, +2);
		  break;
		case SHIFT(HASH):
		  select_obi_course(1, -2);
		  break;

		case SEMICOLON:
		  scan_VORs(0, FALSE);
		  break;
		case SHIFT(SEMICOLON):
		  scan_VORs(0, TRUE);
		  break;

		case QUOTE:
		  scan_VORs(1, FALSE);
		  break;
		case SHIFT(QUOTE):
		  scan_VORs(1, TRUE);
		  break;

		case KEY_I:
		  select_nearest_ils();
		  break;

		/* Weapon selection and release */
		/* case TAB: */
		case BACKSPACE:
			message(Player, weapon_cycle); break;

        case KEY_F: message(Player, flare); break;
	    case KEY_C: message(Player, drop_chaff); break;

        case CTRL(KEY_R): message(Player, jettison_stores); break;
        case CTRL(KEY_E): message(Player, eject); break;

		/* View controls */

		case ALT(KEY_M):
			if (missile_symbol->value) {
				control_set_viewer(missile_symbol->value);
			}
			break;

		case KEY_O:
			if (ejector_symbol->value)
				set_outside_view(ejector_symbol->value);
			else
				set_outside_view(Player);
			break;

		case KEY_P:
			if (padlock_view_on) {
				padlock_view_on = FALSE;
				show_message("PADLOCK VIEW OFF");
			} else {
				padlock_view_on = TRUE;
				show_message("PADLOCK VIEW ON");
			}
			break;

		case KEY_D:
			if (target_track_on) {
				target_track_on = FALSE;
				show_message("TARGET TRACK OFF");
			} else {
				target_track_on = TRUE;
				show_message("TARGET TRACK ON");
			}
			break;

		case KEY_V:
			if (ejector_symbol->value)
				set_track_view(ejector_symbol->value);
			else
				set_track_view(Player);
			break;

		case ALT(KEY_V):
			if (ejector_symbol->value) {
				remove_object(ejector_symbol->value);
				ejector_symbol->value = NULL;
			}
			set_view_orientation(TRUE, DEG(0), DEG(0));
			message(Player, become_viewer);
			break;

		case CTRL(KEY_V):
			s = find_symbol(object_symbol_table, name);
			if (s && s->value) {
				message(s->value, become_viewer);
			}
			break;

        case DELETE:
			s = find_symbol(object_symbol_table, name);
			if (s) {
				o = s->value;
				if (o) {
				  if (o == viewer)
					set_viewer(NULL);
                  SafeMethod(o, World, kill)(o);
				  s->value = NULL;
				}
			}
            break;


		/* View controls
		 * ------------- */
        case F1: set_view_direction(TRUE, DEG(0));   break;
        case F2: set_view_direction(TRUE, DEG(60)); break;
        case F3: set_view_direction(TRUE, DEG(-60));  break;

        case SHIFT(F1): set_view_direction(TRUE, DEG(180));   break;

		case F4: draw_full_cockpit = !draw_full_cockpit;
		   		 message(viewer, become_viewer);
				 break;

        case F5: set_view_direction(TRUE, DEG(120)); break;
        case F6: set_view_direction(TRUE, DEG(-120));  break;

		case F7:
			set_view_direction(FALSE, DEG(10));
			break;

		case F8:
			set_view_direction(FALSE, DEG(-10));
			break;

		case F9:
			set_view_elevation(FALSE, DEG(-10));
			break;

		case F10:
			set_view_elevation(FALSE, DEG(10));
			break;

		case ALT(F1):
			look_direction = 0;
			look_elevation = 0;
			zoom_factor = default_zoom;
			message(viewer, become_viewer);
			break;

		case PAGE_UP:   change_zoom(1); break;
		case PAGE_DOWN: change_zoom(-1); break;

		case CTRL(KEY_M): draw_map(); break;

		/* Controler selection */
		case KEY_Z: reset_mouse(MOUSE_RANGE, MOUSE_RANGE); reset_analog(); break;

		case ESCAPE: process_menus(); break;

		case ALT(KEY_Z):
		   fast_time = (fast_time) ? 0 : 3;
		   break;

		case ALT(KEY_X):
			merge_screen();
			alert_box(160, 20, "EXIT SIMULATION? (Y/N)");
			code = wait_ascii();
			if ((code == 'y') || (code == 'Y')) {
				escape_flag = TRUE;
			}
			split_screen();
			break;

		case ALT(KEY_C):
			calibrate_joystick();
			break;
	}
}



/* --------------------
 * LOCAL button_control
 *
 * Dispatches events from PC joystick discreetes
 */
static void button_control(void)
{
    if (buttons == 0x01) {
		message(Player, primary_fire);
	}
    if ((buttons == 0x02) && !(last_buttons == 2))  {
		message(Player, weapon_cycle);
	}
    if ((buttons == 0x04) && !(last_buttons == 4)) {
        message(Player, gear_control);
    }
    if ((buttons == 0x08) && !(last_buttons == 8)) {
        message(Player, flaps_control);
    }
    if ((buttons == 0x10) && !(last_buttons == 0x10)) {
        message(Player, engine_control);
    }
    if ((buttons == 0x20) && !(last_buttons == 0x20)) {
        message(Player, wheelbrake_control);
    }
    if ((buttons == 0x40) && !(last_buttons == 0x40)) {
        message(Player, airbrake_control);
    }
}


/* -------------------
 * GLOBAL get_control
 *
 * Entry point for control.c module
 *
 */
extern void get_control(void)
{
    sSymbol *s;
    sObject *o;
    int code, i;
	static symbol_i = 0;
	static int enter_symbol = FALSE;

	/* Reset intermediate control mapping array */
    control[0] = control[1] = control[2] = control[3] = 0;
	buttons = 0;

	/* Keyboard event code */
	code = check_keycode();
	if (enter_symbol) {
	  if (code == ENTER) {
		  name[symbol_i] = '\0';
		  enter_symbol = FALSE;
	  } else {
		  code = (code < 0x40) ? ascii_table[code] : 0;
		  if (code != 0) {
		      name[symbol_i++] = (char)code;
		  	  name[symbol_i] = '\0';
		  }
	  }
	} else {
		keyboard_control(code);
    }

	if (enter_symbol)
    	draw_string(20, 20, name, WHITE);
	else
		draw_string(20, 20, name, GREEN);

	scan_control(code);
    if (mouse_on) {
        mouse_control();
    } else if (analog_on) {
        analog_control();
    } else if (adc_on) {
		adc_control();
    }
    if (buttons != 0)
        button_control();

    rollin     = control[0];
    pitchin    = control[1] + pitch_trim;
    yawin      = control[2];
	if (analog_throttle)
	    throttlein = control[3];

    last_buttons = buttons;
}


void setup_control(void)
{
  setup_menus();

  /* Setup message symbols used in control */

  SetupMessage(autopilot_control);
  SetupMessage(airbrake_control);

  SetupMessage(flaps_control);
  SetupMessage(gear_control);
  SetupMessage(engine_control);
  SetupMessage(wheelbrake_control);

  SetupMessage(flare);
  SetupMessage(drop_chaff);
  SetupMessage(weapon_cycle);
  SetupMessage(jettison_stores);
  SetupMessage(eject);
  SetupMessage(primary_fire);

  SetupMessage(become_viewer);

  /* Initialise the controller state */
  reset_mouse(MOUSE_RANGE, MOUSE_RANGE);
  if (check_analog() == FALSE) {
	  mouse_on  = TRUE;
	  analog_on = FALSE;
  } else {
  	setup_analog();
  }
  load_joystick_defaults();
  setup_analog_useage();
  use_bios_gameport(bios_joystick);

  if (screen_width > 320) {
	default_zoom = 2;
	max_zoom     = 16;
  	zoom_view(default_zoom);
	zoom_factor = default_zoom;
  }
  load_menu_defaults();
}

void tidyup_control(void)
{
}

