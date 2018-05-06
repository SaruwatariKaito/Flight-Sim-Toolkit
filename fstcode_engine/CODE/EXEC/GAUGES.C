/* --------------------------------------------------------------------------
 * EXEC 		Copyright (c) Simis Ltd 1993,994,1995
 * ---			All Rights Reserved
 *
 * File:		gauges.c
 * Ver:			2.00
 * Desc:		Player aircraft cockpit module
 *
 * Head down and head up gauges for FSF
 * 
 * Implements 16 different types of dial and fixed bitmap cockpit
 * background.
 * 
 * Dial parameters are setup in windows Cockpit editor and loaded by
 * read_cockpit.
 * 
 * Background is version 5 PCX file with same name as gauge file by .PCX
 * extension. Colour 0 is transparant.
 * 
 * 320x200 and 640x480 resolutions supported.
 * 
 * Values shown on dials are selected from globals by number
 * and pointed to in gauge data structure.
 * 
 * Cockpits can be displayed as full cockpit (where .PCX is overlayed on 
 * whole screen) or part cockpit (where bottom of cockpit bitmap - defined
 * by lines with no transparacy - is shifted off bottom of screen).
 * -------------------------------------------------------------------------- */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "mem.h"
#include "lists.h"
#include "pic.h"
#include "bitmap.h"
#include "symbol.h"
#include "text.h"
#include "defs.h"
#include "mobile.h"
#include "ground.h"
#include "sound.h"

static Font *cockpit_font = NULL;


/* ----------------------
 * Gauge global variables
 * ----------------------
 * */

/* Aircraft flight data */
int aircraft_alt = 0;
int aircraft_asi = 0;
int aircraft_vsi = 0;
int aircraft_rpm = 0;
int aircraft_heading = 0;
int	aircraft_slip = 0;
int aircraft_roll = 0;
int aircraft_pitch = 0;
int aircraft_g = 0;
int aircraft_aoa = 0;
int aircraft_fuel = 0;

int aircraft_grounded = FALSE;
int	gear_down = TRUE;
int brakes_on = FALSE;
int airbrake_on = FALSE;
int aircraft_stalled = FALSE;
int low_fuel = FALSE;
int engine_fail = FALSE;
int hyd_fail = FALSE;
int electric_fail = FALSE;
int hud_fail = FALSE;
int radar_fail = FALSE;
int nav_fail = FALSE;

int aircraft_flap_angle = 0;
int aircraft_damage     = 0;

/* Navigation globals
 * ------------------
 */
int nav1 = 0;
int nav2 = 0;
int adf  = 0;
int ils  = 0;

int adf_bearing  = 0;
int nav1_bearing = 0;
int nav2_bearing = 0;
int nav1_distance = 0;
int nav2_distance = 0;
int ils_radial   = 0;
int ils_bearing  = 0;
int ils_elev     = 0;

/* Course selector in compass co-ords */
int obi1_course = 0;
int obi2_course = 0;


static int dummy = 0;

typedef struct {
	int draw_hud, draw_radar;
	int hud_width, hud_up, hud_down;
	int draw_full_panel;
	char panel_file[212];
	int panel_height, panel_top, ycentre;
	sBitmap *panel_bitmap;
	Sprite *panel_top_sprite;
	Sprite *attitude_sprite;
	sList *base_gauges, *top_gauges;
    int base_panel_drawn[2];
    int top_panel_drawn[2];
} Cockpit;

static Symbol_Table *cockpits;

static sBitmap *cockpit_pic = NULL;

static char error_message[128];
static void draw_hud_displays(Cockpit *cp);

/* Cockpit tool gauge types -
 * --------------------------
"LFD",
"Simple dial",
"Altimeter dial",
"Double dial",
"Vertical bar",
"Horizontal bar",
"Small digits",
"Large digits",
"Square lamp",
"Round lamp",
"ADF"
"OBI",
"DME",
"ILS",
"EFIS",
"Radar",
"HUD",
"Ordnance"
*/

#define GT_LFD 		0
#define GT_HUD		1
#define GT_DIAL 	2
#define GT_ALT 		3
#define GT_VBAR 	4
#define GT_HBAR 	5
#define GT_SMLDIGIT	6
#define GT_LRGDIGIT	7
#define GT_SQLMP	8
#define GT_RNDLMP	9
#define GT_ADF		10
#define GT_OBI		11
#define GT_DME		12
#define GT_ILS		13
#define GT_RADAR	14
#define GT_ORDNANCE	15
#define GT_ATTITUDE	16

#define IN_TOP 	0x01
#define IN_BASE 0x02

typedef struct Gauge {
    int *monitor1, *monitor2, delta, factor, divisor;
    int last_value1[2], last_value2[2];
    int drawn[2];
    int x, y, width, height, flags, min_val, start_angle;
    void (*update)(Cockpit *cp, struct Gauge *self);
    void (*draw)(Cockpit *cp, struct Gauge *self, int val1, int val2, int rub);
} Gauge;


#define DIAL_BACK 0

static Cockpit *alloc_cockpit(void)
{
	Cockpit *cp;

	cp = alloc_bytes(sizeof(Cockpit));
	if (cp) {
		cp->draw_hud = FALSE;
		cp->draw_radar = FALSE;
		cp->draw_full_panel = TRUE;
		cp->panel_height = 0;
		cp->panel_top    = 199;
		cp->panel_bitmap = NULL;
		cp->panel_top_sprite = NULL;
    	cp->base_gauges  = new_list();
    	cp->top_gauges   = new_list();
		cp->base_panel_drawn[0] = cp->base_panel_drawn[1] = FALSE;
		cp->top_panel_drawn[0]  = cp->top_panel_drawn[1]  = FALSE;
	}
	return cp;
}


/* --------------------
 * LOCAL new_gauge
 * 
 * Create new gauge data structure and initialise
 */ 
static Gauge *new_gauge(Cockpit *cp, int x, int y, int width, int height, int *mon1,
    int *mon2, int delta, int factor, int divisor,
    void (*update)(Cockpit *cp, Gauge *g), void (*draw)(Cockpit *cp, Gauge *g, int val1, int val2, int rub))
{
    Gauge *g, *gd;

	g = (Gauge *)alloc_bytes((unsigned)sizeof(Gauge));
	if (g == NULL)
    	stop("No memory for new gauge");
	g->drawn[1] = g->drawn[2] = FALSE;
	g->monitor1 = mon1;
	g->monitor2 = mon2;
	g->delta = delta;
	if (factor == 0)
    	factor = 1;
	g->factor = factor;
	g->divisor = divisor;
	g->x = x;
	g->y = y;
	g->width = width;
	g->height = height;
	g->update = update;
	g->draw = draw;
	g->flags = 0;
	g->min_val = 0;
	g->start_angle = 0;
	if (((g->y - height) < cp->panel_top) && ((g->y + height) > cp->panel_top)) {
		gd = (Gauge *)alloc_bytes((unsigned)sizeof(Gauge));
		if (gd == NULL)
    		stop("No memory for new gauge");
		*gd = *g;
		list_add(g, cp->top_gauges);
		g->flags |= IN_TOP;
		list_add(gd, cp->base_gauges);
		gd->flags |= IN_BASE;
	} else if ((g->y - height) < cp->panel_top) {
		list_add(g, cp->top_gauges);
		g->flags |= IN_TOP;
	} else {
		list_add(g, cp->base_gauges);
		g->flags |= IN_BASE;
	}
	return g;
}

static void force_draw(Cockpit *cp, Gauge *g)
{
    int val1, val2;

    val1 = *(g->monitor1);
    val2 = *(g->monitor2);
    if (g->factor != 1) {
        val1 *= g->factor;
        val2 *= g->factor;
	}
    if (g->divisor > 0) {
        val1 /= g->divisor;
        val2 /= g->divisor;
	}
    (*g->draw)(cp, g, val1, val2, FALSE);
    g->drawn[draw_screen] = TRUE;
    g->last_value1[draw_screen] = val1;
    g->last_value2[draw_screen] = val2;
}

static void rub_gauge(Cockpit *cp, Gauge *g)
{
    int val, last_val, d;

    val = *(g->monitor1);
    if (g->factor != 1)
        val *= g->factor;
    if (g->divisor > 1)
        val /= g->divisor;
    if (g->drawn[draw_screen]) {
        last_val = g->last_value1[draw_screen];
        d = val - last_val;
        if ((ABS(d) >= g->delta) || (last_val != g->last_value1[view_screen])) {
            (*g->draw)(cp, g, last_val, 0, TRUE);
            g->drawn[draw_screen] = FALSE;
            g->last_value1[draw_screen] = val;
        }
    } else {
        g->last_value1[draw_screen] = val;
    }
}

static void draw_gauge(Cockpit *cp, Gauge *g)
{
	int val;

	if (!g->drawn[draw_screen]) {
		val = g->last_value1[draw_screen];
		(*g->draw)(cp, g, val, 0, FALSE);
		g->drawn[draw_screen] = TRUE;
	}
}

static void update_dial(Cockpit *cp, Gauge *g)
{
    int val, last_val, d;

    val = *(g->monitor1);
    if (g->factor != 1)
        val *= g->factor;
    if (g->divisor > 1)
        val /= g->divisor;
    if (g->drawn[draw_screen]) {
        last_val = g->last_value1[draw_screen];
        d = val - last_val;
        if ((ABS(d) >= g->delta) || (last_val != g->last_value1[view_screen])) {
            (*g->draw)(cp, g, last_val, 0, TRUE);
            (*g->draw)(cp, g, val, 0, FALSE);
            g->drawn[draw_screen] = TRUE;
            g->last_value1[draw_screen] = val;
        }
    } else {
        (*g->draw)(cp, g, val, 0, FALSE);
        g->drawn[draw_screen] = TRUE;
        g->last_value1[draw_screen] = val;
    }
}

static Gauge *new_dial(Cockpit *cp, int x, int y, int width, int height, int *monitor, int delta, int factor, int divisor,
    void (*draw)(Cockpit *cp, Gauge *g, int val1, int val2, int rub))
{
    Gauge *g;

    g = new_gauge(cp, x, y, width, height, monitor, &dummy, delta, factor, divisor, update_dial, draw);
    return g;
}

static void update_dial2(Cockpit *cp, Gauge *g)
{
    int val1, last_val1, val2, last_val2, d1, d2;

    val1 = *(g->monitor1);
    val2 = *(g->monitor2);
    if (g->factor != 1) {
        val1 *= g->factor;
        val2 *= g->factor;
    }
    if (g->divisor > 1) {
        val1 /= g->divisor;
        val2 /= g->divisor;
    }
    if (g->drawn[draw_screen]) {
        last_val1 = g->last_value1[draw_screen];
        d1 = val1 - last_val1;
        last_val2 = g->last_value2[draw_screen];
        d2 = val2 - last_val2;
        if ((ABS(d1) >= g->delta) || (ABS(d2) >= g->delta) ||
                (last_val1 != g->last_value1[view_screen]) ||
                (last_val2 != g->last_value2[view_screen])) {
            (*g->draw)(cp, g, last_val1, last_val2, TRUE);
            (*g->draw)(cp, g, val1, val2, FALSE);
            g->drawn[draw_screen] = TRUE;
            g->last_value1[draw_screen] = val1;
            g->last_value2[draw_screen] = val2;
        }
    } else {
        (*g->draw)(cp, g, val1, val2, FALSE);
        g->drawn[draw_screen] = TRUE;
        g->last_value1[draw_screen] = val1;
        g->last_value2[draw_screen] = val2;
    }
}

static Gauge *new_dial2(Cockpit *cp, int x, int y, int width, int height, int *mon1, int *mon2, int delta, int factor, int divisor,
    void (*draw)(Cockpit *cp, Gauge *g, int val1, int val2, int rub))
{
    Gauge *g;

    g = new_gauge(cp, x, y, width, height, mon1, mon2, delta, factor, divisor, update_dial2, draw);
    return g;
}

static void update_indicator(Cockpit *cp, Gauge *g)
{
    int val, last_val;

    val = *(g->monitor1);
    if (g->drawn[draw_screen]) {
        last_val = g->last_value1[draw_screen];
        if (val != last_val) {
            (*g->draw)(cp, g, val, 0, FALSE);
            g->drawn[draw_screen] = TRUE;
            g->last_value1[draw_screen] = val;
        }
    } else {
        (*g->draw)(cp, g, val, 0, FALSE);
        g->drawn[draw_screen] = TRUE;
        g->last_value1[draw_screen] = val;
    }
}

static Gauge *new_indicator(Cockpit *cp, int x, int y, int width, int height, int *monitor,
    void (*draw)(Cockpit *cp, Gauge *g, int val1, int val2, int rub))
{
    Gauge *g;

    g = new_gauge(cp, x, y, width, height, monitor, &dummy, 1, 1, 0, update_indicator, draw);
    return g;
}

static void draw_dial_back(Cockpit *cockpit, Gauge *g)
{
	int x, y, width, height;

	x = g->x - g->width;
	y = g->y - g->height;
	width = (g->width << 1) + 1;
	height = (g->height << 1) + 1;
	write_bitmap(cockpit->panel_bitmap, screen_width, x, y - cockpit->panel_top, x, y, width, height);
}

static void draw_lamp_back(Cockpit *cockpit, Gauge *g)
{
	int x, y;

	x = g->x - g->width;
	y = g->y - g->height;
	write_bitmap(cockpit->panel_bitmap, screen_width, x, y - cockpit->panel_top, x, y, g->width*2+1, g->height*2 + 1);
}

/* Val1 is in degrees (0 - 360) */
static void draw_dial(Cockpit *cp, Gauge *g, int val1, int val2, int rub)
{
    int sa, ca, x, y, size;

	if (rub) {
		if (g->flags & IN_BASE)
			draw_dial_back(cp, g);
	} else {
		val1 -= g->min_val;
		val1 += g->start_angle;
		val1 = DEG(val1);
		sa = SIN(val1);
		ca = COS(val1);
		size = g->width;
		x = g->x + (int)TRIG_SHIFT(MUL(size, sa));
		y = g->y - (int)TRIG_SHIFT(MUL(size, ca));
		line(g->x, g->y, x, y, WHITE);
	}
}

static void draw_attitude(Cockpit *cp, Gauge *g, int val1, int val2, int rub)
{
    int x, y, size, sa, ca, cy, dx, dy, top, bottom;
    Screen_clip s;
	sVector2 pts1[3], pts2[3];

    if (!rub) {
    	x = g->x;
    	y = g->y;
    	s = clip_region;
		size = g->width + (g->width >> 1);
		if (val2 > DEG(80)) {
			rectangle(x - size, y - size, x + size, y + size, D_BLUE);
		} else if (val2 < DEG(-80)) {
			rectangle(x - size, y - size, x + size, y + size, BROWN);
		} else {
			top = y - size;
			if (top < s.top)
				top = s.top;
			bottom = y + size;
			if (bottom > s.bottom)
				bottom = s.bottom;
			clipping(x - size, x + size, top, bottom);
        	sa = SIN(val1); ca = COS(val1);
        	cy = -SR(val2, 10);
    		dx = (int)SR((MUL(-42,ca) - MUL(cy,sa)), SSHIFT);
    		dy = (int)SR((MUL(-42,sa) + MUL(cy,ca)), SSHIFT);
			pts1[0].x = x + dx;
			pts1[0].y = y - dy;
			pts1[1].x = x + dy;
			pts1[1].y = y + dx;
			pts2[1].x = x - dy;
			pts2[1].y = y - dx;
    		dx = (int)SR((MUL(42,ca) - MUL(cy,sa)), SSHIFT);
    		dy = (int)SR((MUL(42,sa) + MUL(cy,ca)), SSHIFT);
			pts1[2].x = x + dx;
			pts1[2].y = y - dy;
			polygon(3, pts1, D_BLUE);
			pts2[0] = pts1[2];
			pts2[2] = pts1[0];
        	polygon(3, pts2, BROWN);
			cy += 4;
    		dx = (int)SR((MUL(-6,ca) - MUL(cy,sa)), SSHIFT);
    		dy = (int)SR((MUL(-6,sa) + MUL(cy,ca)), SSHIFT);
			pts1[0].x = x + dx;
			pts1[0].y = y - dy;
    		dx = (int)SR((MUL(6,ca) - MUL(cy,sa)), SSHIFT);
    		dy = (int)SR((MUL(6,sa) + MUL(cy,ca)), SSHIFT);
			line(pts1[0].x, pts1[0].y, x + dx, y - dy, 0x03);
			cy -= 8;
    		dx = (int)SR((MUL(-6,ca) - MUL(cy,sa)), SSHIFT);
    		dy = (int)SR((MUL(-6,sa) + MUL(cy,ca)), SSHIFT);
			pts1[0].x = x + dx;
			pts1[0].y = y - dy;
    		dx = (int)SR((MUL(6,ca) - MUL(cy,sa)), SSHIFT);
    		dy = (int)SR((MUL(6,sa) + MUL(cy,ca)), SSHIFT);
			line(pts1[0].x, pts1[0].y, x + dx, y - dy, 0x0a);
        	clipping(s.left, s.right, s.top, s.bottom);
		}
		draw_sprite(cp->attitude_sprite, x - size, y - size, 2 * size + 1);
    }
}

static int angle(int a)
{
  while (a < 0)
	a += 360;
  while (a >= 360)
	a -= 360;
  return a;
}

static void draw_gauge_number(int val, int x, int y, int col)
{
	int width, height;
	char s[16];

	if (screen_width <= 640) {
		vector_number(val, x, y, col);
	} else {
		sprintf(s, "%d", val);
		draw_string_size(s, &width, &height);
		draw_string(x - width, y - (height >> 1), s, col);
	}
}

static void draw_flag(int x1, int y1, int to)
{
	int y;


	if (to) {
		y = y1;
		pixel_prim(x1+2,       y++, COL(RED));
    	hline_prim(x1+1, x1+3, y++, COL(RED));
    	hline_prim(x1,   x1+4, y++, COL(RED));
	} else {
		y = y1+8;
		pixel_prim(x1+2,       y--, COL(RED));
    	hline_prim(x1+1, x1+3, y--, COL(RED));
    	hline_prim(x1,   x1+4, y--, COL(RED));
	}
}


static void draw_obi(Cockpit *cp, Gauge *g, int val1, int val2, int rub)
{
    int sa, ca, x, y, size, hsize;
	int csel, radial, cdi, to;

	if (rub) {
		if (g->flags & IN_BASE)
			x = g->x - g->width;
			y = g->y - g->height;
			write_bitmap(cp->panel_bitmap, screen_width, x, y - cp->panel_top, x, y, g->width * 2 + 1, g->height * 2 + 1);
	} else {
		csel = val2; /* Course selector in compass coords */
        draw_gauge_number(csel, g->x+5, g->y-g->height+5, GREEN);

		/* reciprocal course */
		draw_gauge_number(angle(180+csel), g->x+5, g->y+g->height-5, GREEN);

		pixel_prim(g->x+1, g->y, COL(WHITE));
		pixel_prim(g->x-1, g->y, COL(WHITE));
		pixel_prim(g->x, g->y+1, COL(WHITE));
		pixel_prim(g->x, g->y-1, COL(WHITE));
		for (x = -5; x <= +5; x++) {
			pixel_prim(g->x+x*3, g->y, COL(WHITE));
		}

		if (val1) {

			radial = angle(360 + 180 - val1/DEG(1)); /* Radial on which a/c is in compass coords */

			to = FALSE;
			cdi = angle(radial - csel); /* course deviation */

			/* Get cdi in range -180 to +180 */
			if (cdi > 180)
				cdi = cdi-360;

			if (cdi > 90) {
				to = TRUE;
				cdi = cdi - 180;
			}
			if (cdi < -90) {
				to = TRUE;
				cdi = 180 + cdi;
			}

			size = SR(g->width, 1);
			size = (size + size>>2) * 2;

			/* Scale cdi to size -180 == - size, +180 == size */
			x = (cdi * size) / 20;

			if (x > size) x = size;
		  	else if (x < -size) x = -size;

			draw_flag(g->x+4, g->y-4, to);
		} else {
			x = 0;
		}
		hsize = SR(g->height, 1);
        line(g->x+x, g->y-hsize, g->x+x, g->y+hsize, WHITE);
	}
}

#define GLIDESLOPE DEG(6)
#define ILSDEV     10
static void draw_ils(Cockpit *cp, Gauge *g, int val1, int val2, int rub)
{
    int sa, ca, size, hsize, xtmp, ytmp;
	int csel, radial, cdi, ils_dirn, elev;
	static int x = 0, y = 0;
	static int32 glidewarn_timer = 0L;

	if (rub) {
		if (g->flags & IN_BASE)
			xtmp = g->x - g->width;
			ytmp = g->y - g->height;
			write_bitmap(cp->panel_bitmap, screen_width, xtmp, ytmp - cp->panel_top, xtmp, ytmp, g->width * 2 + 1, g->height * 2 + 1);
	} else {
		elev  = val2; /* Elev */

		size  = SR(g->width, 1);
		hsize = SR(g->height, 1);
		if ((ils != 0) && (val1 > DEG(-10) && val1 < DEG(10))) {
			/* Draw lamp showing ils active */
    		hline_prim(g->x+size/2+1, g->x+size/2+4-1, g->y-4, COL(GREEN));
    		hline_prim(g->x+size/2, g->x+size/2+4, g->y-3, COL(GREEN));
    		hline_prim(g->x+size/2, g->x+size/2+4, g->y-2, COL(GREEN));
    		hline_prim(g->x+size/2+1, g->x+size/2+4-1, g->y-1, COL(GREEN));

			/*
			 * vector_number(elev/DEG(1), g->x+5, g->y-g->height+5, GREEN);
        	vector_number(val1/DEG(1), g->x+5, g->y+g->height-5, GREEN);
			*/

			ils_dirn = angle(360 - ils_radial/DEG(1)); /* ILS Radial in compass coords */
			radial   = angle(360 + 180 - val1/DEG(1)); /* A/C Radial in compass coords */

			cdi = angle(radial - ils_dirn); /* course deviation */

			/* Get cdi in range -180 to +180 */
			if (cdi > 180)
				cdi = cdi-360;

			if (cdi > 90) {
				cdi = cdi - 180;
			}
			if (cdi < -90) {
				cdi = 180 + cdi;
			}

			/* Scale cdi to size -180 == - size, +180 == size */
			x = (cdi * size) / ILSDEV;

			if (x > size) x = size;
		  	else if (x < -size) x = -size;


			/* Draw glideslope line */
			y = SUBANGLE(elev, GLIDESLOPE);
			if (y > 2*GLIDESLOPE) {
				if (glidewarn_timer < simulation_time) {
					glidewarn_sound();
					glidewarn_timer = simulation_time + SECS(5);
				}
				y = 2*GLIDESLOPE;
			}
			y = y * size / GLIDESLOPE;

		}
		for (xtmp = -6; xtmp <= +6; xtmp += 2) {
			pixel_prim(g->x+xtmp, g->y, COL(GREY9));
			pixel_prim(g->x, g->y+xtmp, COL(GREY9));
		}
        line(g->x+x,    g->y-hsize, g->x+x,    g->y+hsize, WHITE);
        line(g->x-size, g->y+y,     g->x+size, g->y+y, WHITE);
	}
}

static void draw_alt_dial(Cockpit *cp, Gauge *g, int val1, int val2, int rub)
{
    int sa, ca, x1, y1, x2, y2, v, size;

    if (rub) {
		if (g->flags & IN_BASE)
			draw_dial_back(cp, g);
    } else {
		size = g->width;
		v = (int)(MUL(val1, DEG(9)) / 25L);
    	sa = SIN(v) + 20;
    	ca = COS(v);
    	x1 = g->x + (int)TRIG_SHIFT(MUL(size, sa));
    	y1 = g->y - (int)TRIG_SHIFT(MUL(size, ca));
		v = (int)(MUL(val1, DEG(9)) / 250L);
    	sa = SIN(v) + 20;
    	ca = COS(v);
		v = size - 3;
    	x2 = g->x + (int)TRIG_SHIFT(MUL(v, sa));
    	y2 = g->y - (int)TRIG_SHIFT(MUL(v, ca));
        line(g->x, g->y, x1, y1, WHITE);
        line(g->x, g->y, x2, y2, WHITE);
		pixel_prim(g->x, g->y, COL(BLACK));
    }
}

static char *weapon_names[] = {"GUN", "R", "AA", "AG", "BOMB", "CLSTR", "TORP"};
static void draw_weapon(Cockpit *cp, Gauge *g, int val1, int val2, int rub)
{
    int x1,x2,y;
	char s[32];

    if (rub) {
        x1 = g->x-g->width-1;
        x2 = g->x+2;
        y = g->y-2;
        hline_prim(x1, x2, y++, DIAL_BACK);
        hline_prim(x1, x2, y++, DIAL_BACK);
        hline_prim(x1, x2, y++, DIAL_BACK);
        hline_prim(x1, x2, y++, DIAL_BACK);
        hline_prim(x1, x2, y++, DIAL_BACK);
    } else {
		x1 = g->x-g->width+2;
		sprintf(s, "%s %d", weapon_names[val1], weapon_n);
		x1 = draw_string(x1, g->y - cockpit_font->height/2, s, GREEN);
    }
}

static void draw_vbar(Cockpit *cp, Gauge *g, int val1, int val2, int rub)
{
    int base, top, bottom;
    Screen_clip s;

    if (rub) {
		if (g->flags & IN_BASE) {
			draw_lamp_back(cp, g);
		}
    } else {
		s = clip_region;
		base = g->y + g->height;
		top = g->y - g->height;
		if (top < s.top)
			top = s.top;
		bottom = base;
		if (bottom > s.bottom)
			bottom = s.bottom;
		clipping(g->x - g->width, g->x + g->width, top, bottom);
		rectangle(g->x - g->width, base - val1, g->x + g->width, base, GREEN);
		clipping(s.left, s.right, s.top, s.bottom);
    }
}

static void draw_hbar(Cockpit *cp, Gauge *g, int val1, int val2, int rub)
{
    int top, bottom;
    Screen_clip s;

    if (rub) {
		if (g->flags & IN_BASE) {
			draw_lamp_back(cp, g);
		}
    } else {
		s = clip_region;
		top = g->y - g->height;
		if (top < s.top)
			top = s.top;
		bottom = g->y + g->height;
		if (bottom > s.bottom)
			bottom = s.bottom;
		clipping(g->x - g->width, g->x + g->width, top, bottom);
		vline_prim(g->y - g->height, g->y + g->height, g->x + val1, WHITE);
		clipping(s.left, s.right, s.top, s.bottom);
    }
}

static void draw_number(Cockpit *cp, Gauge *g, int val1, int val2, int rub)
{
    if (rub) {
		if (g->flags & IN_BASE) {
			draw_lamp_back(cp, g);
		}
    } else {
        vector_number(val1, g->x+g->width-2, g->y, GREEN);
    }
}

static void draw_large_number(Cockpit *cp, Gauge *g, int val1, int val2, int rub)
{
	char s[64];

    if (rub) {
		if (g->flags & IN_BASE) {
			draw_lamp_back(cp, g);
		}
    } else {
		sprintf(s, "%d", val1);
		draw_string((g->x - g->width) + 1, (g->y - g->height + 1), s, GREEN);
    }
}

extern void draw_radar_display(int cx, int cy, int width, int height);

static void draw_radar(Cockpit *cp, Gauge *g, int val1, int val2, int rub)
{
    if (rub) {
		if (g->flags & IN_BASE) {
			rectangle(g->x - g->width, g->y - g->height, g->x + g->width, g->y + g->height, BLACK);
		}
    } else {
		draw_radar_display(g->x, g->y, g->width, g->height);
	}
}

static void draw_lamp(int x1, int x2, int y, int h, int col)
{
    int c;

    c = COL(col);
    while (h-- > 0)
        hline_prim(x1, x2, y++, c);
}

static void draw_state_lamp(Cockpit *cp, Gauge *g, int val1, int val2, int rub)
{
	if (rub) {
		if (g->flags & IN_BASE)
			draw_lamp_back(cp, g);
    } else if (val1) {
        draw_lamp(g->x-g->width, g->x + g->width, g->y-g->height, g->height*2, GREEN);
    }
}

static void draw_warn_lamp(Cockpit *cp, Gauge *g, int val1, int val2, int rub)
{
	if (rub) {
		if (g->flags & IN_BASE)
			draw_lamp_back(cp, g);
    } else if (val1) {
        draw_lamp(g->x-g->width, g->x + g->width, g->y-g->height, g->height*2, RED);
    }
}

static void draw_state_round(Cockpit *cp, Gauge *g, int val1, int val2, int rub)
{
	if (rub) {
		if (g->flags & IN_BASE)
			draw_lamp_back(cp, g);
    } else if (val1) {
        circle_f(g->x, g->y, g->width, GREEN);
    }
}

static void draw_warn_round(Cockpit *cp, Gauge *g, int val1, int val2, int rub)
{
	if (rub) {
		if (g->flags & IN_BASE)
			draw_lamp_back(cp, g);
    } else if (val1) {
        circle_f(g->x, g->y, g->width, RED);
    }
}

void rub_gauges(Cockpit *cockpit)
{
    sList *l;
    Gauge *g;

	if (cockpit == NULL)
		return;
    cockpit->base_panel_drawn[draw_screen] = FALSE;
    cockpit->base_panel_drawn[view_screen] = FALSE;
    cockpit->top_panel_drawn[draw_screen]  = FALSE;
    cockpit->top_panel_drawn[view_screen]  = FALSE;
    for (l = cockpit->base_gauges->next; l != cockpit->base_gauges; l = l->next) {
        g = (Gauge *)l->item;
        g->drawn[draw_screen] = FALSE;
        g->drawn[view_screen] = FALSE;
    }
    for (l = cockpit->top_gauges->next; l != cockpit->top_gauges; l = l->next) {
        g = (Gauge *)l->item;
        g->drawn[draw_screen] = FALSE;
        g->drawn[view_screen] = FALSE;
    }
}


/* -------------------
 * GLOBAL set_cockpit_state
 * 
 * Moves cockpit from displaying full cockpit to part cockpit
 * and back.
 */
void set_cockpit_state(sSymbol *cs, int draw_full_panel)
{
  Cockpit *cp;
  sList *l;
  Gauge *g;

  if (cs && cs != nil_symbol) {
  	cp = (Cockpit*)cs->value;
	if (cp->draw_full_panel != draw_full_panel) {
		cp->draw_full_panel = draw_full_panel;
		if (cp->draw_full_panel) {
			/* Was top only - add to y */
    		for (l = cp->top_gauges->next; l != cp->top_gauges; l = l->next) {
        		g = (Gauge *)l->item;
				g->y -= cp->panel_height;
    		}
		} else {
			/* Was total only - sub from y */
    		for (l = cp->top_gauges->next; l != cp->top_gauges; l = l->next) {
        		g = (Gauge *)l->item;
				g->y += cp->panel_height;
    		}
		}
	}
  }
}

static sSymbol *last_cockpit = NULL;

/* -------------------
 * GLOBAL change_cockpit
 * 
 * Change to new cockpit:
 * 
 * rubs old cocpit gauges
 * resets new cockpit data structures
 */
void change_cockpit(sSymbol *cs)
{
  Cockpit *cp;

  /* Old cockpit */
  if (last_cockpit && last_cockpit != nil_symbol)
  	rub_gauges((Cockpit*)last_cockpit->value);

  /* New cockpit */
  if (cs && cs != nil_symbol) {
  	cp = (Cockpit*)cs->value;
	if (cp->draw_full_panel) {
		view_base = cp->panel_top-1;
  		ycentre   = cp->ycentre;
	} else {
  		view_base = screen_height-1;
  		ycentre   = cp->ycentre + cp->panel_height;
	}
  } else {
  	view_base = screen_height-1;
  	ycentre   = view_base / 2;
  }
  last_cockpit = cs;
}


/* -------------------
 * GLOBAL draw_gauges
 * 
 * Draws cockpit background and gauges
 * 
 */
void draw_gauges(sSymbol *cs)
{
	int col;
	Cockpit *cp;
    sList *l;
    Gauge *g;
    Screen_clip s;

	s = clip_region;
	/* If change cockpit has not been called */
	if (last_cockpit != cs) {
		change_cockpit(cs);
	}

	if (cockpit_font == NULL) {
		cockpit_font = use_font("FONTG");
		if (cockpit_font == NULL) {
			cockpit_font = use_font("FONT57");
		}
	} else {
		current_font = cockpit_font;
	}

	cp = (cs) ? cs->value : NULL;
	if (cp == NULL) {
		col = COL(GREEN);
		pixel_prim(xcentre-2, ycentre, col);
		pixel_prim(xcentre+2, ycentre, col);
		pixel_prim(xcentre, ycentre-2, col);
		pixel_prim(xcentre, ycentre+2, col);
		return;
	}

	/* Top panel */
    clipping(0, screen_width - 1, 0, view_base);
	draw_sprite(cp->panel_top_sprite, 0, (cp->draw_full_panel) ? 0 : cp->panel_height, 0);
    for (l = cp->top_gauges->next; l != cp->top_gauges; l = l->next) {
        g = (Gauge *)l->item;
        force_draw(cp, g);
    }

	if (cp->draw_hud) {
		clipping(xcentre - cp->hud_width, xcentre + cp->hud_width, ycentre - cp->hud_up, ycentre + cp->hud_down);
		draw_hud_displays(cp);
	} else {
		clipping(0, screen_width - 1, 0, view_base);
		col = COL(GREEN);
		pixel_prim(xcentre-2, ycentre, col);
		pixel_prim(xcentre+2, ycentre, col);
		pixel_prim(xcentre, ycentre-2, col);
		pixel_prim(xcentre, ycentre+2, col);
	}

	if (cp->draw_full_panel) {
    	clipping(0, screen_width - 1, view_base + 1, screen_height - 1);
		/* Bottom panel */
		use_screen();
    	if (!cp->base_panel_drawn[draw_screen]) {
    		if (cp->panel_bitmap != NULL) {
        		write_bitmap(cp->panel_bitmap, screen_width, 0, 0, 0, view_base + 1, screen_width, cp->panel_height);
			} else {
				clear_prim(view_base + 1, screen_height - 1, COL(0));
			}
    		cp->base_panel_drawn[draw_screen] = TRUE;
    	}
    	for (l = cp->base_gauges->next; l != cp->base_gauges; l = l->next) {
        	g = (Gauge *)l->item;
        	(*g->update)(cp, g);
    	}
		use_buffer();
	}
	clipping(s.left, s.right, s.top, s.bottom);
}

#define NAV1 20
#define NAV2 21

#define MAX_VARPTR 27
static int *var_ptrs[MAX_VARPTR] = {
	&ycentre, 			&aircraft_alt, 	&aircraft_asi, 		&aircraft_vsi, 	&aircraft_slip,
	&aircraft_heading, 	&aircraft_roll,	&aircraft_pitch, 	&aircraft_g, 	&aircraft_aoa,
	&aircraft_rpm, 		&aircraft_fuel, &gear_down,			&brakes_on, 	&airbrake_on,
	&aircraft_stalled, 	&low_fuel, 		&engine_fail, 		&hyd_fail, 		&electric_fail,
	&nav1, 				&nav2, 			&adf, 				&ils, 			&aircraft_flap_angle,
	&aircraft_damage, 	&dummy
};

static int warn_lamp_type(int var)
{
	if (var >= 15 && var <= 19)
		return TRUE;
	return FALSE;
}

/* new_dial(x, y, width, height, monitor, delta, factor, divisor, draw) */
/* led - width = 0 : 1 digit
 *       width = 5 : 2 digit
 */
/* TBD:
#define GT_RNDLMP	9
 */
static void create_gauge(Cockpit *cp, int type, int var, int x, int y, int width, int height, int low, int high, int start_angle, int stop_angle)
{
    int delta, factor, divisor, trans_col;
	Gauge *g;

	trans_col = 0;
	delta = 1;
	factor = 1;
	divisor = 1;
	switch (type) {
		case GT_LFD:		/* LFD position */
			cp->ycentre = y;
            break;
		case GT_HUD:		/* LFD position also set bu HUD */
			cp->ycentre = y;
			cp->draw_hud = TRUE;
			cp->hud_width = width;
			cp->hud_up = low;
			cp->hud_down = high;
			break;
		case GT_DIAL:		/* Standard dial */
			if (start_angle > stop_angle)
				factor = stop_angle - (start_angle - 360);
			else
				factor = stop_angle - start_angle;
			if (factor == 0)
				factor = 360;
			divisor = high - low;
			if (divisor == 0)
				divisor = 1;
			delta = 2;
			g = new_dial(cp, x, y, width, height, var_ptrs[var], delta, factor, divisor, draw_dial);
			g->min_val = (low * factor) / divisor;
			g->start_angle = start_angle;
			break;
		case GT_ALT:		/* Altimeter dial */
            new_dial(cp, x, y, width, height, var_ptrs[var], delta, factor, divisor, draw_alt_dial);
            break;
		case GT_VBAR:		/* Vertical bar */
			factor = height * 2;
			divisor = high - low;
			if (divisor == 0)
				divisor = 1;
			delta = 1;
            new_dial(cp, x, y, width, height, var_ptrs[var], delta, factor, divisor, draw_vbar);
            break;
		case GT_HBAR:		/* Horizontal bar */
			factor = width * 2;
			divisor = high - low;
			if (divisor == 0)
				divisor = 1;
			delta = 1;
            new_dial(cp, x, y, width, height, var_ptrs[var], delta, factor, divisor, draw_hbar);
            break;
		case GT_SMLDIGIT:		/* Vector number */
            new_dial(cp, x, y, width, height, var_ptrs[var], delta, factor, divisor, draw_number);
            break;
		case GT_LRGDIGIT:		/* Vector number */
            new_dial(cp, x, y, width, height, var_ptrs[var], delta, factor, divisor, draw_large_number);
            break;
		case GT_SQLMP:		/* Square lamp */
            new_dial(cp, x, y, width, height, var_ptrs[var], delta, factor, divisor, warn_lamp_type(var) ? draw_warn_lamp : draw_state_lamp);
            break;
		case GT_RNDLMP:		/* Round lamp */
            new_dial(cp, x, y, width, height, var_ptrs[var], delta, factor, divisor, warn_lamp_type(var) ? draw_warn_round : draw_state_round);
            break;
		case GT_ORDNANCE:		/* Weapon string number */
            new_dial(cp, x, y, width, height, &weapon_type, 1, 1, 0, draw_weapon);
            break;
		case GT_ADF:		/* ADF nav dial */
            new_dial(cp, x, y, width, height, &adf_bearing, 1, 1, 0, draw_dial);
            break;
		case GT_OBI:		/* OBI nav dial */
			if (var == NAV1)
              new_dial2(cp, x, y, width, height, &nav1_bearing, &obi1_course, 1, 1, 0, draw_obi);
			else if (var == NAV2)
              new_dial2(cp, x, y, width, height, &nav2_bearing, &obi2_course, 1, 1, 0, draw_obi);
            break;
		case GT_DME:		/* DME nav dial */
			if (var == NAV1)
              new_dial(cp, x, y, width, height, &nav1_distance, 1, 1, 0, draw_number);
			else if (var == NAV2)
              new_dial(cp, x, y, width, height, &nav2_distance, 1, 1, 0, draw_number);
            break;
		case GT_ILS:		/* ILS nav dial */
            new_dial2(cp, x, y, width, height, &ils_bearing, &ils_elev, 1, 1, 0, draw_ils);
    		break;
		case GT_ATTITUDE:
			put_bitmap_circle(cockpit_pic, screen_width, x, y, width, 0);
			cp->attitude_sprite = create_sprite(cockpit_pic, screen_width, x - width - (width / 2), y - height - (height / 2), (width * 2) + width + 1, (height * 2) + height + 1, 0);
			new_dial2(cp, x, y, width, height, &aircraft_roll, &aircraft_pitch, DEG_1, 0, 0, draw_attitude);
    		break;
		case GT_RADAR:
            new_dial(cp, x, y, width, height, &aircraft_alt, delta, factor, divisor, draw_radar);
    		break;
    }
}

static void create_gauges(FILE *fp, Cockpit *cp)
{
    int c, type, var, x, y, width, height, low, high, start_angle, stop_angle;

    c = fscanf(fp, "%d %d %d %d %d %d %d %d %d %d\n", &type, &var, &x, &y, &width, &height, &low, &high, &start_angle, &stop_angle);
    while ((c > 0) && (c != EOF)) {
		create_gauge(cp, type, var, x, y, width, height, low, high, start_angle, stop_angle);
	    c = fscanf(fp, "%d %d %d %d %d %d %d %d %d %d\n", &type, &var, &x, &y, &width, &height, &low, &high, &start_angle, &stop_angle);
    }
	rub_gauges(cp);
}

/* Finds height of panel_bitmap as panel_height
   and puts rest of picture in panel_top_sprite */
static void load_panel(Cockpit *cockpit)
{
	int x, y, col;
	char s[80];

    if (cockpit_pic == NULL)
        stop("No memory for loading panel bitmap");
	load_pic(cockpit->panel_file, cockpit_pic, screen_width, screen_height, TRUE, 16);
	save_as_base_palette(0, 15);
	if (PCX_info.width != screen_width) {
		sprintf(s, "Cockpit PCX is %d pixels wide, but screen is %d pixels wide", PCX_info.width, screen_width);
		stop(s);
	}
	cockpit->panel_height = screen_height / 2;
	for (y = screen_height - 1, col = 1; (y > 0) && (col != 0); y--) {
		for (x = 0; (x < screen_width) && (col != 0); x++) {
			col = get_bitmap_pixel(cockpit_pic, screen_width, x, y);
			if (col == 0)
				cockpit->panel_height = screen_height - y - 1;
		}
	}
	if (cockpit->panel_height > 0) {
		cockpit->panel_bitmap = alloc_bitmap(screen_width, cockpit->panel_height);
    	if (cockpit->panel_bitmap == NULL)
        	stop("No memory for panel bitmap");
		copy_bitmap_lines(cockpit_pic, cockpit->panel_bitmap, screen_width, screen_height - cockpit->panel_height, 0, cockpit->panel_height);
		cockpit->panel_top = screen_height - cockpit->panel_height;
		cockpit->ycentre = (cockpit->panel_top-1)/2;
	}
	cockpit->panel_top_sprite = create_sprite(cockpit_pic, screen_width, 0, 0, screen_width, screen_height - cockpit->panel_height, 0);
}

/* Returns TRUE if cockpit loaded OK */
static int read_cockpit(Cockpit *cp, char *file_name)
{
    int version, width;
    FILE *fp;
    char s[212];

	fp = fopen(file_name, "r");
    if (fp != NULL) {
        fscanf(fp, "%s %d\n", s, &version);
        if (strcmp(s, "FST_Gauges") != 0) {
            fclose(fp);
            stop("Not a valid FST gauge data file");
			return FALSE;
        }
        if (version > program_version) {
            fclose(fp);
            stop("Gauge data file version is too high");
			return FALSE;
        }
        fscanf(fp, "%s %s\n", s, cp->panel_file);
		load_pcx_info(cp->panel_file);
		if (PCX_info.width != screen_width) {
			sprintf(error_message, "Cockpit PCX is %d pixels wide, but screen is %d pixels wide", PCX_info.width, screen_width);
			return FALSE;
		}
		cockpit_pic = alloc_bitmap(screen_width, screen_height);
		load_panel(cp);
		create_gauges(fp, cp);
    	free_bitmap(cockpit_pic);
        fclose(fp);
		return TRUE;
    }
	return FALSE;
}

sSymbol *read_gauges(char *file_name)
{
    int status;
	sSymbol *sym;
	Cockpit *cp;
	char hires_file[32];

	sym = get_symbol(cockpits, file_name);
	if (sym == nil_symbol || sym->value != NULL) {
		return sym;
	}

	cp = sym->value = alloc_cockpit();
	if (sym->value == NULL) {
		stop("Failed to allocated Cockpit data structure");
	}

	status = read_cockpit(cp, file_name);
    if (!status && (screen_width > 320)) {
		/* If hires then check for S prefix */
		sprintf(hires_file, "S%s", file_name);
		status = read_cockpit(cp, hires_file);
		if (!status) {
			stop(error_message);
		}
	}
	return sym;
}

void reset_gauges(void)
{
}

void setup_gauges(void)
{
	cockpits = new_symbol_table();
}

void init_gauges(void)
{
}

static void free_cockpit_data(sSymbol *s, void *handle)
{
	Cockpit *cp;

	handle = handle;
	cp = s->value;
	if (cp)
		free_bitmap(cp->panel_bitmap);
}

void tidyup_gauges(void)
{
	apply_to_symbol_table(cockpits, free_cockpit_data, NULL);
}

/* ---------------------
 * Special HUD displays:
 * --------------------- */
static int c_hud = GREEN;

/* draw_hud_target
 * ---------------
 *  o 			- viewing object
 * target 		- target to be boxed
 * direction 	-
 * returns TRUE if target is still in f.o.v.
 */
static int draw_hud_target(sObject *from, sObject *target)
{
    int sx, sy, col;
	World *target_data;
	World *o;
    int32 range;
	sShape *ts;
    sVector p;
    sVector2 s;

	o 			= from->data;
	target_data = target->data;
	if (target_data == NULL || !(target_data->flags & TARGET_TAG))
		return FALSE;

	ts  = (target_data->shape) ? target_data->shape->s : NULL;
	if (ts == NULL)
		stop("draw_hud_target");
    p.x = (target_data->p.x - o->p.x) >> SPEED_SHIFT;
    p.y = (target_data->p.y - o->p.y) >> SPEED_SHIFT;
    p.z = (target_data->p.z - o->p.z) >> SPEED_SHIFT;
    range = perspect(&p, &s);
    sx = s.x - xcentre;
    sy = s.y - ycentre;
    if ((range > 0) && (ABS(sx) < 55) && (ABS(sy) < 55)) {
		if (weapon_track == W_HEAT) {
			circle(s.x, s.y, 7, c_hud);
		} else if (weapon_track == W_GROUND) {
			col = COL(c_hud);
			hline_prim(s.x - 6, s.x + 6, s.y - 5, col);
			hline_prim(s.x - 6, s.x + 6, s.y + 5, col);
			vline_prim(s.y - 4, s.y + 4, s.x - 6, col);
			vline_prim(s.y - 4, s.y + 4, s.x + 6, col);
		}
		return TRUE;
	}
	return FALSE;
}

void draw_hud_ccip(sObject *o, sVector *ccip)
{
    int col, dx, dy;
    int32 range;
    sVector p;
    sVector2 s;

    p.x = (ccip->x - Posn(o)->x) >> SPEED_SHIFT;
    p.y = (ccip->y - Posn(o)->y) >> SPEED_SHIFT;
    p.z = (ccip->z - Posn(o)->z) >> SPEED_SHIFT;
    range = perspect(&p, &s);
    if (range > 0) {
        col = COL(GREEN);
		if (weapon_type == WT_BOMB || weapon_type == WT_CLSTR) {
        	hline_prim(s.x - 6, s.x - 2, s.y, col);
        	hline_prim(s.x + 2, s.x + 6, s.y, col);
			if (TRUE/*weapon_mode == W_CCIP*/) {
        		vline_prim(s.y - 6, s.y - 2, s.x, col);
        		vline_prim(s.y + 2, s.y + 6, s.x, col);
			} else {
				dx = s.x - xcentre;
				dy = s.y - ycentre;
				s.x = xcentre + (dx * 2);
				s.y = ycentre + (dy * 2);
			}
			if ((look_elevation == 0) && (look_direction == 0))
				line(xcentre, ycentre, s.x, s.y, c_hud);
		} else if (weapon_type == WT_ROCKET) {
        	hline_prim(s.x - 5, s.x - 2, s.y, col);
        	hline_prim(s.x + 2, s.x + 5, s.y, col);
        	vline_prim(s.y + 2, s.y + 4, s.x, col);
		}
    }
}

static void draw_pbar(int pitch, int py, int sr, int cr)
{
    int x1, x2, x3, y1, y2, y3;
	int pbar_x1, pbar_x2, pshift;

	if (screen_width < 400) {
		pbar_x1 = 26;
		pbar_x2 = 16;
		pshift = SSHIFT - 2;
	} else {
		pbar_x1 = 30;
		pbar_x2 = 10;
		pshift = SSHIFT - 3;
	}
	if ((pitch == 9) || (pitch == -9)) {
        x1 = xcentre + ((6 * cr - py * sr) >> pshift);
        y1 = ycentre + ((6 * sr + py * cr) >> pshift);
        x2 = xcentre + ((-6 * cr - py * sr) >> pshift);
        y2 = ycentre + ((-6 * sr + py * cr) >> pshift);
        line(x1, y1, x2, y2, c_hud);
        x1 = xcentre + ((-(py + 6) * sr) >> pshift);
        y1 = ycentre + (((py + 6) * cr) >> pshift);
        x2 = xcentre + ((-(py - 6) * sr) >> pshift);
        y2 = ycentre + (((py - 6) * cr) >> pshift);
        line(x1, y1, x2, y2, c_hud);
    } else {
        x1 = xcentre + ((pbar_x1 * cr - py * sr) >> pshift);
        y1 = ycentre + ((pbar_x1 * sr + py * cr) >> pshift);
        x2 = xcentre + ((pbar_x2 * cr - py * sr) >> pshift);
        y2 = ycentre + ((pbar_x2 * sr + py * cr) >> pshift);
        line(x1, y1, x2, y2, c_hud);
        x3 = xcentre + ((-pbar_x1 * cr - py * sr) >> pshift);
        y3 = ycentre + ((-pbar_x1 * sr + py * cr) >> pshift);
        x2 = xcentre + ((-pbar_x2 * cr - py * sr) >> pshift);
        y2 = ycentre + ((-pbar_x2 * sr + py * cr) >> pshift);
        line(x3, y3, x2, y2, c_hud);
        if (x1 < xcentre) {
            x1 -= 4;
        } else {
            x1 = x3 - 4;
            y1 = y3;
        }
        if (pitch > 9)
            pitch = 18 - pitch;
        else if (pitch < -9)
            pitch = -18 - pitch;
        draw_gauge_number(pitch, x1, y1, c_hud);
    }
}

void draw_pitch_ladder(Cockpit *cp, sObject *view, int direction)
{
    int pitch, roll, p1, py, sr, cr;
	World *o;

	o = view->data;
    if (direction == 0) {
		roll = ADDANGLE(o->r.z, look_roll);
        sr = SIN(-roll) >> 2;
        cr = COS(-roll) >> 2;
        pitch = (short)o->r.x >> 4;
        p1 = pitch / 112;
        py = ((pitch % 112) * 42) / 112;
        draw_pbar(p1 + 1, py - 42, sr, cr);
        draw_pbar(p1, py, sr, cr);
        draw_pbar(p1 - 1, py + 42, sr, cr);
    }
}

static unsigned nav_vel_d[7] = {
	0x0400, 0x0400, 0x0400, 0x0a00, 0xf1e0, 0x1100, 0x0e00
};
static sCharacter nav_vel = {11, 7, nav_vel_d};

extern sVector wind;

static void draw_velocity_vector(sObject *view)
{
	sVector v;
	sVector2 s;
	Mobile *o;

	o = view->data;
	v.x = o->wv.x >> 1;
	v.y = o->wv.y >> 1;
	v.z = o->wv.z >> 1;
	fast_perspect(&v, &s);
	character(s.x - 5, s.y - 4, &nav_vel, GREEN);
}

static void draw_hdg_num(int x, int y, int n)
{
    static int d1[12] = {0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3};
    static int d2[12] = {0, 3, 6, 9, 2, 5, 8, 1, 4, 7, 0, 3};
	char s[16];

	if (screen_width <= 640) {
		vector_digit(d1[n], x-2, y, c_hud);
    	vector_digit(d2[n], x+2, y, c_hud);
	} else {
		sprintf(s, "%d%d", d1[n], d2[n]);
		draw_string(x - cockpit_font->max_width, y - (cockpit_font->height >> 1), s, c_hud);
	}
}

static void draw_heading_tape(Cockpit *cp, sObject *view)
{
    int a, d, y;
    Screen_clip s;

    s = clip_region;
	y = ycentre - cp->hud_up + 6;
	clipping(xcentre - cp->hud_width + 4, xcentre + cp->hud_width - 4, y - 6, y + 10);
    a = aircraft_heading / 30;
    d = aircraft_heading - (a*30);

    vline_prim(y + 5, y + 7, xcentre, COL(GREEN));
    draw_hdg_num(xcentre - d - 30, y, (a+11)%12);
    draw_hdg_num(xcentre - d, y, a%12);
    draw_hdg_num(xcentre - d + 30, y, (a+1)%12);
    draw_hdg_num(xcentre - d + 60, y, (a+2)%12);
    clipping(s.left, s.right, s.top, s.bottom);
}

static void draw_hud_displays(Cockpit *cp)
{
	int col;
	World *o1, *o2;

	col = COL(GREEN);
    pixel_prim(xcentre-2, ycentre, col);
    pixel_prim(xcentre+2, ycentre, col);
    pixel_prim(xcentre, ycentre-2, col);
    pixel_prim(xcentre, ycentre+2, col);
	if (!hud_fail) {
		draw_pitch_ladder(cp, viewer, 0);
		draw_velocity_vector(viewer);
		draw_heading_tape(cp, viewer);
		if (selected != NULL) {
			o1 = viewer->data;
			o2 = selected->data;
    		if (check_intervisibility(&o1->p, &o2->p)) {
    			if (!draw_hud_target(viewer, selected))
					weapon_lock = FALSE;
			} else {
				weapon_lock = FALSE;
			}
		}
		if ((weapon_type == WT_ROCKET) || (weapon_type == WT_BOMB) || (weapon_type == WT_CLSTR)) {
 			draw_hud_ccip(viewer, &ccip);
		}
	}
}

void draw_floating_hud(sSymbol *cs)
{
	Cockpit *cp;
	sVector v;
	sVector2 s;
	Mobile *o;
	Transdata trans;
	int oldx, oldy;
	int col;
	World *o1, *o2;

	cp = (cs) ? cs->value : NULL;

	o = viewer->data;
 	v.x = 0;
	v.y = 0;
	v.z = 1000;
	setup_trans(&o->r, &trans);
	rot3d(&v, &v, &trans);
	if (fast_perspect(&v, &s) > 0) {

		oldx = xcentre;
		oldy = ycentre;
		xcentre= s.x;
		ycentre= s.y;

		col = COL(GREEN);
    	hline_prim(xcentre-4, xcentre-2, ycentre, col);
    	hline_prim(xcentre+4, xcentre+2, ycentre, col);
    	vline_prim(ycentre-4, ycentre-2, xcentre, col);
    	vline_prim(ycentre+4, ycentre+2, xcentre, col);
		if (!hud_fail) {
			draw_pitch_ladder(cp, viewer, 0);
			draw_heading_tape(cp, viewer);
			if (selected != NULL) {
				o1 = viewer->data;
				o2 = selected->data;
    			if (check_intervisibility(&o1->p, &o2->p)) {
    				if (!draw_hud_target(viewer, selected))
						weapon_lock = FALSE;
				} else {
					weapon_lock = FALSE;
				}
			}
		}

		xcentre= oldx;
		ycentre= oldy;

		if (!hud_fail) {
			draw_velocity_vector(viewer);
			if ((weapon_type == WT_ROCKET) || (weapon_type == WT_BOMB) || (weapon_type == WT_CLSTR)) {
 				draw_hud_ccip(viewer, &ccip);
			}
		}
	}
}

void select_obi_course(int n, int dn)
{
	if (n == 0)
		obi1_course = angle(obi1_course + dn);
	else
		obi2_course = angle(obi1_course + dn);
}

static char message_string[64] = "";
static int32 message_timer = 0L;

void show_message(char *s)
{
	strcpy(message_string, s);
	message_timer = total_time + SECS(2);
}

void show_messages(void)
{
	if (message_timer > total_time) {
		draw_string(screen_width/2 - draw_string_width(message_string)/2, 2, message_string, WHITE);
	} else {
		if (fast_time) {
			draw_string(screen_width/2 - draw_string_width("FAST TIME")/2, 2, "FAST TIME", WHITE);
		}
	}
}

