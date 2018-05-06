#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "shape.h"
#include "world.h"

#include "keys.h"
#include "io.h"
#include "colour.h"

#include "hash.h"
#include "symbol.h"
#include "class.h"

static char shape_name[16];
static sShape_def *view_shape;
static sVector view_pos = {0, 0, M(100)};
static Aspect view_rot = {0, 0, 0};

int disable_sounds = TRUE;

/* externs for debugging */
extern long int heap_allocated;

int escape_flag = FALSE;

int32 simulation_time = 0;
int32 frame_number = 0;
int   frame_time = 10;
int   slow_cycle = 0, fast_cycle = 0;
int   fast_time  = 0;
int   paused     = 0;

int   view_base = 199;

int view_direction = 0;
int look_direction = 0, look_elevation = 0;

static int setup_done = FALSE;
void stop(char *format, ...)
{
    va_list argptr;

    if (setup_done) {
        tidyup_graphics();
        tidyup_world();
        tidyup_keyboard();
        tidyup_program();
    }

    va_start(argptr, format);
    vprintf(format, argptr);
    va_end(argptr);

    exit(0);
}

static int ram_log = FALSE;
void debug(char *format, ...)
{
    va_list argptr;
    FILE *fp;

    if (ram_log == FALSE)
        return;

    fp = fopen("d:log", "a");
    if (fp) {
        va_start(argptr, format);
        vfprintf(fp, format, argptr);
        va_end(argptr);
        fclose(fp);
    }
}

static void mouse_control(void)
{
	int b, mx, my, speed;

	speed = M(1);
	if (key_down(LEFT_SHIFT_KEY) || key_down(RIGHT_SHIFT_KEY))
		speed = M(10);
	if (key_down(CTRL_KEY))
		speed = M(100);
	b = read_mouse(&mx, &my);
	view_rot.x = my * DEG_1;
	view_rot.y = mx * DEG_1;
	if (b & 1)
		view_pos.z -= speed;
	if (b & 2)
		view_pos.z += speed;
}

int control[4] = {0, 0, 0, 0};

void get_control(void)
{
    int code;

	mouse_control();
	code = check_keycode();
    switch (code) {
		case ALT(KEY_X): escape_flag = TRUE; break;

        case LEFTARROW:  view_rot.y += DEG(10); break;
        case RIGHTARROW:  view_rot.y -= DEG(10); break;
        case UPARROW:  view_rot.x -= DEG(10); break;
        case DOWNARROW:  view_rot.x += DEG(10); break;
        case PAD_PLUS:   view_pos.z += M(1); break;
        case PAD_MINUS:  view_pos.z -= M(1); break;

        case ENTER:
            break;

        case SPACE:
            break;

        default:
            break;
    }
}

static void clear_screen(void)
{
    clipping(0, screen_width - 1, 0, screen_height - 1);
    clear_prim(0, screen_height - 1, COL(0x00));
    swap_screen();
    clear_prim(0, screen_height - 1, COL(0x00));
}

static void show_normals(sShape_def *sd)
{
    int i;
	sPolygon *p, **pp;

	while (sd->down)
		sd = sd->down;
    for (i = sd->s->npolygons, pp = sd->s->polygons; i; i--) {
		p = *pp++;
		vector_number(p->norm.x, 220, 6 + (i * 6), 15);
		vector_number(p->norm.y, 250, 6 + (i * 6), 15);
		vector_number(p->norm.z, 280, 6 + (i * 6), 15);
    }
}

#define GREEN 0x1f

/* total_time       - setup by interupt
 * simulation_time  - current time the simulation has run
 */
static void main_loop(void)
{
    int c;
	int32 dt, last_time, last_dt_time;

    last_time    = total_time;
	last_dt_time = total_time;
    dt = 0;
    while (!escape_flag) {
        /* Calculate frame time */
		frame_time = (int)(total_time - last_time);
        dt += frame_time*10;
        if (fast_cycle == 0) {
            last_dt_time = dt;
            dt = 0;
        }

        /* Lock Simulation to a Maximum 50Hz
         *                      Minimum 5Hz
         */
        if (frame_time < 2) {
            while ((total_time - last_time) < 2)
				;
            frame_time = 2;
        }

		if (fast_time)
            frame_time *= fast_time;

        /* Single step mode
         * Set frame rate to 25Hz */
        if (paused)
            frame_time = 4;

        simulation_time += frame_time;
		last_time = total_time;

        /* Set level of continuous noises (engines etc) */

        /* Get control inputs
         * -----------------
         */
		get_control();

        /* Simulation */

		update_world();
		remove_objects();

        /* Graphics */

        swap_screen();
		clear_prim(0, screen_height - 1, COL(0x27));
		draw_shape_def(view_shape, &view_pos, &view_rot, TRUE);
/*		show_normals(view_shape);*/

		vector_number(view_pos.z / M(1), 30, 6, 0xf);
		vector_number(frame_time, 80, 6, 0xf);

		frame_number++;
		if (++fast_cycle >= 4)
            fast_cycle = 0;
        if (++slow_cycle >= 16)
            slow_cycle = 0;
    }
}

extern void print_class_tables(void);
extern void shapes_loaded(void);
extern sObject *new_explorer(int32 x, int32 y, int32 z, sShape_def *s);
extern void save_state(void);

int main(int argc, char **argv)
{
	char gmode;
	World *o;

    strcpy(shape_name, "");
    if (argc >= 2) {
		strcpy(shape_name, argv[1]);
    }
	gmode = 'l';
    if (argc >= 3) {
		gmode = *argv[2];
    }

    /* Initialise and Setup */
	setup_program(500000);
    setup_keyboard();
	setup_colours();

    init_world();

	switch (gmode) {
		case 'V': case 'v':
			setup_graphics(VESA_640x480_256);
    		setup_graphics3();
			zoom_view(2);
			break;
		default:
			setup_graphics(VGA_320x200_256);
    		setup_graphics3();
			zoom_view(1);
			break;
	}
	setup_mouse(512, 512);
    setup_world();
    setup_done = TRUE;

    /* Colours */
	default_colours();
   	load_colourmap("cols.fcd", 0, 255);
	use_palette(0, 255);
	save_as_base_palette(0, 255);

	set_sun_angle(DEG(60), DEG(0));

    clear_screen();

/*	presort_on = FALSE;*/

	viewer = new_object(World_class);
	o = viewer->data;
	o->p = zero;
	o->r = zero;

	view_shape = get_shape(shape_name);
	if (view_shape == NULL) {
		if (set_path("SHAPES")) {
			view_shape = get_shape(shape_name);
			set_path("..");
		}
	}

	use_base_palette(0, 255);
	if (view_shape != NULL) {
		view_pos.z = (view_shape->s->radius * 4) << view_shape->s->scale;
    	main_loop();
	} else {
		stop("Shape not found");
	}

    stop("");
	return 0;
}

