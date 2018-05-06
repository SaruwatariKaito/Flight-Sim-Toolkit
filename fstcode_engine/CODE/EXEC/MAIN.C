/* --------------------------------------------------------------------------
 * EXEC 		Copyright (c) Simis Ltd 1993,994,1995
 * ---			All Rights Reserved
 *
 * File:		main.c
 * Ver:			2.00
 * Desc:        Main loop and supporting function
 *
 * Entry point and main simulation loop
 *
 * -------------------------------------------------------------------------- */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <dos.h>

#include "shape.h"
#include "world.h"

#include "keys.h"
#include "io.h"
#include "text.h"
#include "pic.h"
#include "colour.h"

#include "hash.h"
#include "symbol.h"
#include "class.h"

#include "sound.h"

#include "defs.h"
#include "ground.h"

extern void update_adf(void);
extern void update_obi(void);

extern int g_effects_on;
extern int disable_sounds;

static char version[16] = "V2.00";

/* Global symbols */
sSymbol *player_symbol  = NULL;
sSymbol *missile_symbol = NULL;
sSymbol *ejector_symbol = NULL;

static int32 game_time = 0;

/* Weather data */
sVector wind = {0, 0, 0};
int wind_speed = 0;
int wind_dir   = 0;

/* Misc externs */
int use_save_path = FALSE;
char save_path[80] = "";

extern char terrain_name[32];
extern void get_control(void);
extern void print_class_tables(void);
extern void shapes_loaded(void);
extern sObject *new_explorer(int32 x, int32 y, int32 z, sShape_def *s);
extern void setup_control(void);
extern void tidyup_control(void);

/* externs for debugging */
extern long int heap_allocated;
extern int sort_count;
extern int copied_poly_count;
extern int heap_available(void);
static int initial_heap_size = 0;

/* Include application class headers */
#include "cultural.h"
#include "ndb.h"
#include "vor.h"
#include "ils.h"
#include "aagun.h"
#include "sam.h"

#include "brick.h"
#include "plane.h"
#include "military.h"

#include "depot.h"
#include "hanger.h"

#include "weapons.h"
#include "rocket.h"
#include "missile.h"
#include "track.h"
#include "truck.h"
#include "aircraft.h"
#include "flare.h"
#include "eject.h"


int escape_flag = FALSE;

int program_version = 1;
int world_file_version = 1;
int32 simulation_time = 0;
int32 frame_number = 0;
int   frame_time = 10;
int   slow_cycle = 0, fast_cycle = 0;
int   fast_time  = 0;
int   paused     = 0;
int timer_on = FALSE;
int player_die = FALSE;

int hires_graphics = FALSE;
int view_base = 199;

static int view_direction = 0;
int look_direction = 0, look_elevation = 0;
int look_roll = 0;

static int min_frame_time = 4;
static int setup_done = FALSE;
static int sound_started = FALSE;


/* -------------------
 * GLOBAL stop
 *
 * Stop program and print reason to DOS screen
 */
void stop(char *format, ...)
{
    va_list argptr;

    if (setup_done) {
		if (sound_started)
			exit_sound();
        tidyup_graphics();
        tidyup_world();
		tidyup_colours();
		tidyup_pic();
		tidyup_gauges();
		tidyup_control();
    }
	tidyup_keyboard();
    tidyup_program();

	if (scoring_on) {
		write_score();
	}
    va_start(argptr, format);
    vprintf(format, argptr);
    va_end(argptr);

    exit(0);
}


static int ram_log = FALSE;

/* -------------------
 * GLOBAL debug
 *
 * Debugging function - writes to RAM disk D:
 */
void debug(char *format, ...)
{
    va_list argptr;
    FILE *fp;

    if (ram_log == FALSE)
        return;

    fp = fopen("D:log", "a");
    if (fp) {
        va_start(argptr, format);
        vfprintf(fp, format, argptr);
        va_end(argptr);
        fclose(fp);
    }
}

void save_fst_screen(void)
{
	static int save_count = 1;
	char s[40];
    Screen_clip sc;

    sc = clip_region;
	clipping(0, screen_width - 1, 0, view_base);
	sprintf(s, "GRAB%d.PCX", save_count++);
	merge_screen();
	save_screen(s);
	split_screen();
    clipping(sc.left, sc.right, sc.top, sc.bottom);
}

static void setup_wind(void)
{
	int spd, dir, sa, ca;
    sSymbol *s;

	s = find_symbol(value_symbol_table, "WINDSPEED");
	if (s) {
		wind_speed = (int)s->value;
	}
	s = find_symbol(value_symbol_table, "WINDDIR");
	if (s) {
		wind_dir = (int)s->value;
	}
	dir = DEG(wind_dir);
	sa = SIN(dir);
	ca = COS(dir);
	/* wind_speed is in Km/h */
	spd = FIXSPD(wind_speed);
	wind.y = 0;
	wind.x = (int)(MUL(spd, -sa) >> SSHIFT);
	wind.z = (int)(MUL(spd, ca) >> SSHIFT);
}

static int automatic_colours = FALSE;
static int32 next_recolour_time = 0L;
static int32 next_time_of_day = 0L;
static int last_night_n = 0;
void time_of_day(int32 time)
{
	int mins, min15s, hours, n, sun_rx, sun_ry;

	if (time >= next_time_of_day) {
		next_time_of_day = time + 6000L;
		mins = DIV(time, 100 * 60) % (24 * 60);
		min15s = (mins + 7) / 15;
		hours = (mins + 30) / 60;
		if ((hours <= 6) || (hours >= 18)) {
			if (hours <= 6)
				n = ((6 * 60 + 10) - mins) / 4;
			else if (hours >= 18)
				n = (mins - (17 * 60 + 50)) / 4;
			if (n < 0)
				n = 0;
			if (n > 15)
				n = 15;
			if (n != last_night_n) {
				last_night_n = n;
				if (automatic_colours) {
					night_colourmap(0, 15, n, 32);
					night_colourmap(16, 31, n, 48);
					night_colourmap(32, 255, n, 16);
				}
			}
		} else if (last_night_n != 0) {
			last_night_n = 0;
			if (automatic_colours)
				night_colourmap(0, 255, 0, 16);
		}
		n = (int)(MUL(mins, DEG(6)) / 24L);
		sun_rx = -(int)SR(MUL(DEG(80), COS(n)), SSHIFT);
		sun_ry = ADDANGLE(DEG(180), n);
		set_sun_angle(sun_rx, sun_ry);
		if (hours == 6) {
			if (min15s == (6 * 4))
				sun_colour = ORANGE;
			else
				sun_colour = AMBER;
			sun_size = 45 - (ABS((6 * 60) - mins) / 3);
		} else if (hours == 18) {
			if (min15s == (18 * 4))
				sun_colour = ORANGE;
			else
				sun_colour = AMBER;
			sun_size = 45 - (ABS((18 * 60) - mins) / 3);
		} else if ((hours < 6) || (hours > 18)) {
			sun_colour = 0;
			sun_size = 35;
		} else {
			sun_colour = YELLOW;
			sun_size = 35;
		}
		if (time >= next_recolour_time) {
			next_recolour_time = time + 60000L;
			recolour_world();
		}
	}
}

void g_effects(void)
{
	static int32 blackout_start = 0, blackout_end = 0;
	static int blackout = FALSE;
	static int greyout = FALSE;
	static int redout = FALSE;
	int g;

	if (aircraft_g >= (80)) {
		redout = FALSE;
		greyout = TRUE;
		if (aircraft_g >= (120)) {
			blackout_start += frame_time;
			if (blackout_start > 50) {
				blackout_start = 50;
				blackout = TRUE;
				blackout_end = total_time + 200;
			}
		} else {
			if (blackout_start > 0)
				blackout_start -= frame_time;
		}
	} else if (aircraft_g <= (-50)) {
		greyout = FALSE;
		redout = TRUE;
	} else {
		greyout = FALSE;
		redout = FALSE;
	}
	if (blackout && (total_time < blackout_end)) {
		g_colourmap(32);
	} else if (greyout) {
		g = aircraft_g - (80);
		if (g > 31)
			g = 31;
		g_colourmap(g);
		blackout = FALSE;
	} else if (redout) {
		g = aircraft_g + (50);
		if (g < -31)
			g = -31;
		g_colourmap(g);
		blackout = FALSE;
	} else {
		g_colourmap(0);
		blackout = FALSE;
	}
}

/* total_time       - setup by interupt
 * simulation_time  - current time the simulation has run
 */
#define DT_RATE 4


/* -----------------------------------------
 * LOCAL wait_count
 * Determins if more than or equal to "by" csec have passed since
 * last_time by chacking against total_time (set by interupt)
 */
static int wait_count(int by, int last_time)
{
	if ((total_time - last_time) >= by)
		return TRUE;
    return FALSE;
}

/* --------------------
 * LOCAL main_loop
 *
 * Main simulation loop
 *
 */
static void main_loop(void)
{
    int c;
	int32 dt, last_time, max_frame_time, last_dt_time;

	next_time_of_day = 0L;
	next_recolour_time = 0L;
	time_of_day(game_time);

	frame_time   = 5;
	max_frame_time = (3 * frame_time) / 2;
	last_time    = total_time - frame_time;
	last_night_n = 0;
    dt = frame_time * DT_RATE;
	last_dt_time = total_time - dt;
    while (!escape_flag) {
        /* Pause Simulation from keyboard */
        while (paused) {
            c = wait_ascii();
            if ((c == 'p') || (c == 'P'))
                paused = FALSE;
            if ((c == 's') || (c == 'S'))
                break;
            if ((c == 'g') || (c == 'G'))
				save_fst_screen();
                break;
			last_time = total_time - frame_time;
        }

        /* Lock Simulation to a maximum rate */
		if ((total_time - last_time) < min_frame_time)
			while (!wait_count(min_frame_time, last_time))
				;


		last_time = total_time;
        /* Calculate frame time */
        if (DT_RATE == 16) {
        	if (slow_cycle == 0) {
				dt = total_time - last_dt_time;
            	last_dt_time = total_time;
        	}
        } else {
        	if (fast_cycle == 0) {
				dt = total_time - last_dt_time;
            	last_dt_time = total_time;
        	}
        }
		frame_time = dt / DT_RATE;

        /* Lock Simulation to a minimum rate */
        if (frame_time > max_frame_time)
            frame_time = max_frame_time;
        if (frame_time < min_frame_time)
            frame_time = min_frame_time;

		if (fast_time)
            frame_time *= fast_time;

        /* Single step mode
         * Set frame rate to 25Hz */
        if (paused)
            frame_time = 4;

        simulation_time += frame_time;
		game_time += frame_time;
		max_frame_time = ((3 * frame_time) / 2) + 1;
		if (max_frame_time > 20)
			max_frame_time = 20;

        /* Set level of continuous noises (engines etc) */

        /* Get control inputs
         * -----------------
         */
		get_control();

		update_sounds();

        /* Simulation
         * ----------
         */
        update_world();

		update_adf();
		update_obi();

        /* Graphics
         * --------
         */

    	clipping(0, screen_width - 1, 0, view_base);
		swap_screen();
		view_direction = draw_world(viewer, look_direction, look_elevation, look_roll, draw_ground);

		if (view_direction == -1)
			draw_string(screen_width/2 - draw_string_width("No viewer")/2, screen_height/2, "No viewer", GREEN);

		show_messages();
		remove_objects();
		time_of_day(game_time);
		if (g_effects_on && (viewer == Player))
			g_effects();

		if (slow_cycle == 11)
			check_targets();

		if (timer_on) {
			draw_string(4, 1, version, WHITE);
			if (dt > 0 && (dt/DT_RATE) > 0)
        		vector_number(100 / (dt / DT_RATE), 30, 12, GREEN);
	      	vector_number(objects_allocated, 30, 20, GREEN);
        	vector_number(heap_allocated, 30, 26, GREEN);
        	vector_number(sort_count, 30, 32, GREEN);
        	vector_number(copied_poly_count, 30, 38, GREEN);
	        vector_number(dt, 30, 44, GREEN);
	        vector_number(total_time, 30, 54, GREEN);
	        vector_number(last_dt_time, 30, 64, GREEN);
	        vector_number(frame_time, 30, 74, GREEN);
	        vector_number(total_time/100, 30, 84, GREEN);
		}
		frame_number++;
		if (++fast_cycle >= 4)
            fast_cycle = 0;
        if (++slow_cycle >= 16)
            slow_cycle = 0;
    }
}

static void clear_screen(void)
{
    clipping(0, screen_width - 1, 0, screen_height - 1);
    clear_prim(0, screen_height - 1, COL(0x00));
    swap_screen();
    clear_prim(0, screen_height - 1, COL(0x00));
}

static int load_project_colours(void)
{
	FILE *fp;
	char proj_name[32];
	int  x, z;			/* size of project world in 50Km tiles */
	char cols_file[32] = "cols.fcd";

	fp = fopen("FST.FPJ", "r");
	if (fp) {
		fscanf(fp, "%s %d %d %s", proj_name, &x, &z, cols_file);
		fclose(fp);
		load_colourmap(cols_file, 0, 255);
    	save_as_base_palette(0, 255);
		return TRUE;
	} else {
		return FALSE;
	}
}

static void show_title_pic(void)
{
	FILE *fp;

	if (screen_width >= 640)
		fp = fopen("SFST.PCX", "r");
	else
		fp = fopen("FST.PCX", "r");
	if (fp) {
		fclose(fp);
		if (screen_width >= 640)
    		show_pic("SFST.PCX", screen_height, TRUE, 256);
		else
    		show_pic("FST.PCX", screen_height, TRUE, 256);
		zero_palette(0, 255);
    	swap_screen();
    	fade_in(32, 0, 255);
	}
}

static void hide_title_pic(void)
{
	FILE *fp;

	if (screen_width >= 640)
		fp = fopen("SFST.PCX", "r");
	else
		fp = fopen("FST.PCX", "r");
	if (fp) {
		fclose(fp);
    	fade_out(32, 0, 255);
	}
}

/* args: -d exit on player die
 *       -l load state file
 *       -s save state file at end
 *       -w world name max 256 chars
 *       -n no sampled sounds
 */
static char world_name[32] = "WORLD.FST";
static char player_name[32] = "Player";
static int load, save, load_world;
static int graphics_mode;
static void process_args(int n, char ** argv)
{
    int a, i, type;
	char *argchars;

    a = 0;
    while (a < n) {
        if (argv[a][0] != '-')
            stop("Error reading command args");
        switch (argv[a][1]){
            case 'n': case 'N':
				/* Check for modifier */
				argchars = &(argv[a][2]);
				switch ((*argchars)) {
					case 'z': case 'Z':
						draw_sea_on = FALSE;
						shaded_ground = TRUE;
						break;
					case 'p': case 'P':
						presort_on = FALSE;
						break;
					case 'f': case 'F':
						fast_presort_on = FALSE;
						break;
					case 's': case 'S':
						disable_sounds = TRUE;
						break;
					case 'h': case 'H':
						horizon_polygons = FALSE;
						break;
					default:
						disable_sounds = TRUE;
						break;
				}
                break;
            case 'o': case 'O':
				/* Check for modifier */
				argchars = &(argv[a][2]);
				switch ((*argchars)) {
					case 't': case 'T':
						target_track_on = TRUE;
						break;
					default:
						break;
				}
                break;
            case 'd': case 'D':
				player_die = TRUE;
                break;
			case 'm': case 'M':
				/* Check for min frame time */
				min_frame_time = atoi(&argv[a][2]);
				if (min_frame_time < 1)
					min_frame_time = 1;
				if (min_frame_time > 20)
					min_frame_time = 20;
				break;
            case 'w': case 'W':
				argchars = &(argv[a][2]);
				i = 0;
				while (*argchars && (i < 32))
					world_name[i++] = *argchars++;
				world_name[i] = '\0';
                if (i < 5)
                    stop("-w no file name specified ; format is -wFILENAME");
                load_world = TRUE;
                break;
            case 'f': case 'F':
				argchars = &(argv[a][2]);
				i = 0;
				while (*argchars && (i < 32))
					save_path[i++] = *argchars++;
				save_path[i] = '\0';
                if (i < 2)
                    stop("-f probable bad path; format is -fPATH");
				if (save_path[i-1] != '\\') {
					save_path[i] = '\\';
					save_path[i+1] = '\0';
				}
				use_save_path = TRUE;
                break;
			case 'p': case 'P':
				argchars = &(argv[a][2]);
				i = 0;
				while (*argchars && (i < 32))
					player_name[i++] = *argchars++;
				player_name[i] = '\0';
                if (i < 1)
                    stop("-p no player name specified ; format is -pNAME");
                load_world = TRUE;
				break;
			case 'v': case 'V':
				graphics_mode = VESA_640x480_256;
				hires_graphics = TRUE;
				screen_height = 480;
				view_base = screen_height - 1;
				break;
            default:
                stop("Error reading command args");
                break;
        }
		a += 1;
    }
    if (a != n)
        stop("Error reading command args");
}

void end_fst(void)
{
	die_sound();
	stop("");
}

int main(int argc, char **argv)
{
	int    mins, deg;
    sSymbol *s;
    sObject *o;

	/* Setup defaults
	 * -------------- */
	graphics_mode  = VGA_320x200_256;
	hires_graphics = FALSE;
	screen_height  = 200;
	view_base      = screen_height - 1;

    load = save = FALSE;
	load_world = TRUE;
    strcpy(world_name, "WORLD.FST");

    /* Process command line arguments
	 * ------------------------------ */
    if (argc > 1) {
        process_args(argc-1, &(argv[1]));
    }

    /* Initialise and Setup
	 * -------------------- */
	setup_sound();
	setup_program(500000);
    setup_keyboard();
	setup_pic();
	setup_colours();

    init_world();

	setup_graphics(graphics_mode);
    setup_graphics3();
	setup_mouse(screen_width, screen_height);
    setup_world();

	show_title_pic();

	init_ground();
	init_gauges();


	/* Colours - loaded from current directory - via project file */
	load_project_colours();

	setup_gauges();
	setup_control();

	setup_inter();

    setup_done = TRUE;

	if (use_font("FONT57") == NULL)
		stop("Failed to find font57");

    /* Register application specific classes
	 * ------------------------------------- */
    register_cultural();
    register_ndb();
    register_vor();
    register_ils();
    register_aagun();
    register_sam();
    register_brick();
    register_plane();
    register_military();
    register_depot();
    register_hanger();

    register_rocket();
    register_missile();
    register_weapons();
    register_track();
    register_truck();
    register_aircraft();
    register_flare();
    register_eject();

	/* Set a default position for lighting */
	set_sun_angle(DEG(60), DEG(170));

    /* Create a world */
    if (load_world) {
		setup_ground();
        load_worldfile(world_name);

		/* Player
		 * ------ */
		player_symbol = find_symbol(object_symbol_table, player_name);
		if (player_symbol == nil_symbol)
			stop("No player in world.fst");

		message(player_symbol->value, get_symbol(message_symbol_table, "become_viewer"));

        missile_symbol = get_symbol(object_symbol_table, "Missile");
        missile_symbol->value = NULL;

        ejector_symbol = get_symbol(object_symbol_table, "Ejector");
        ejector_symbol->value = NULL;
    }

	/* Set time of day */
	s = find_symbol(value_symbol_table, "TIMEOFDAY");
	if (s) {
		game_time = (long int)(s->value) * 60L * 100L;
	} else {
		game_time = 9L * 60L * 60L * 100L; /* 9 AM */
	}

	/* Set auto-colour */
	s = find_symbol(value_symbol_table, "AUTOCOLS");
	if (s && s->value) {
		automatic_colours = TRUE;
	}

	/* Set weather data */
	setup_wind();

	start_sound();
	sound_started = TRUE;
	startup_ground(viewer);
	hide_title_pic();
    clear_screen();

	use_base_palette(0, 255);
	c_smoke1 = match_colour(160, 160, 160, 0, 31);
	c_smoke2 = match_colour(140, 140, 140, 0, 31);
	c_smoke3 = match_colour(120, 120, 120, 0, 31);
	bullet_scoring_firer = Player;
	set_viewer(viewer);

	main_loop();
	clear_screen();

    stop("FST finished \n");
	return 0;
}
