/* --------------------------------------------------------------------------
 * EXEC 		Copyright (c) Simis Ltd 1993,994,1995
 * ---			All Rights Reserved
 *
 * File:		menu.c
 * Ver:			1.00
 * Desc:		In game menu
 *
 * This file implements both the menu system and the in game menu
 * used.
 *
 * -------------------------------------------------------------------------- */

/* FSF Simulation menus
 * ====================
 */
#include <string.h>
#include "defs.h"
#include "io.h"
#include "text.h"
#include "keys.h"

extern void setup_analog_useage(void);
extern void calibrate_joystick(void);
extern void save_joystick_defaults(void);
extern void set_control_type(int n);

extern int analog_on;
extern int mouse_on;
extern int keyboard_on;

extern int standard_joystick;
extern int bios_joystick;
extern int analog_rudder;
extern int analog_throttle;
extern int thrustmaster_stick;
extern int thrustmaster_throttle;
extern int flightstick;
extern int flightstick_pro;

static int crash_on = 0;

static Font *menu_font = NULL;

#define M_RADIO  0x02
#define M_TOGGLE 0x01
typedef struct {
	int flags;
	int *value;
	char *s;
} Menu_item;

typedef struct Menu {
	char *title;
	int x, y, width, height, n_items;
	int (*action)(struct Menu *m, int sel);
	Menu_item *items;
} Menu;

#define MENU1_SIZE 6
#define M_EXIT (MENU1_SIZE - 1)
static Menu_item menu1_d[MENU1_SIZE] = {
	{0x00, NULL, "Controls"},
	{0x00, NULL, "Display"},
	{0x00, NULL, "Simulation"},
    {0x00, NULL, "Sounds"},
	{0x00, NULL, "Save as default"},
	{0x00, NULL, "Exit Simulation"},
};
static Menu *top_menu = NULL;

extern int use_save_path;
extern char save_path[80];

int g_effects_on = TRUE;
extern int noise_on, engine_noise_on;

#define MENU2_SIZE 3
static Menu_item menu2_d[MENU2_SIZE] = {
	{0x02, &analog_on,   "Joystick"},
	{0x02, &mouse_on,    "Mouse"},
	{0x02, &keyboard_on, "Keyboard"},
};
static Menu *control_menu = NULL;

#define MENU21_SIZE 8
static Menu_item menu21_d[MENU21_SIZE] = {
	{0x00, NULL, "Calibrate"},
	{0x00, &standard_joystick, "Standard Stick"},
	{0x00, &analog_rudder, "Analog Rudder"},
	{0x00, &analog_throttle, "Analog Throttle"},
	{0x00, &flightstick_pro, "Flightstick Pro"},
	{0x00, &thrustmaster_stick, "Thrustmaster Stick"},
	{0x00, &thrustmaster_throttle, "Thrustmaster Throttle"},
	{0x00, &bios_joystick, "BIOS Gameport"},
};
static Menu *joystick_menu = NULL;
static int calibrate_flag = FALSE;

#define MENU3_SIZE 3
static Menu_item menu3_d[MENU3_SIZE] = {
	{0x01, &shading_on,     "Horizon shading"},
	{0x00, NULL,            "Increase haze"},
	{0x00, NULL,            "Decrease haze"},
};

static Menu *display_menu = NULL;


#define MENU4_SIZE 2
static Menu_item menu4_d[MENU4_SIZE] = {
	{0x01, &crash_on,          "Ground Crash"},
	{0x01, &g_effects_on,      "G-lock"},
};
static Menu *simulation_menu = NULL;

#define MENU5_SIZE 2
static Menu_item menu5_d[MENU5_SIZE] = {
    {0x01, &engine_noise_on, "Engine noise"},
    {0x01, &noise_on,        "All sounds"},
};
static Menu *sound_menu = NULL;

#define MENU6_SIZE 7
static Menu_item menu6_d[MENU6_SIZE] = {
	{0x00,  NULL,    "NDB Scan up"},
	{0x00,  NULL,    "NDB Scan down"},
	{0x00,  NULL,    "VOR 1 Scan up"},
	{0x00,  NULL,    "VOR 1 Scan down"},
	{0x00,  NULL,    "VOR 2 Scan up"},
	{0x00,  NULL,    "VOR 2 Scan down"},
	{0x00,  NULL,    "ILS Scan"},
};

static Menu *nav_menu = NULL;

#define C_MENU1  GREEN /* Menu frame colour */
#define C_MENU2  GREY10
#define C_MENU3  GREY12
#define C_MTEXT1 GREEN
#define C_MTEXT2 RED

static unsigned check_char_d[4] = {
	0x0800, 0x1000, 0xa000, 0x4000,
};
static sCharacter check_char = {5, 4, check_char_d};


Menu *create_menu(char *title, Menu_item *items, int n_items, int (*action)(Menu *m, int sel))
{
	int i, width, max_width;
	Menu *m;

	m = alloc_bytes(sizeof(Menu));
	if (m == NULL)
		stop("No memory for menu");
	m->title = title;
	m->items = items;
	m->n_items = n_items;
	m->action = action;
	m->x = 10;
	m->y = 10;
	for (i = 0, max_width = 0; i < n_items; i++, items++) {
		width = draw_string_width(items->s);
		if (width > max_width)
			max_width = width;
	}
	if (title) {
		width = draw_string_width(m->title);
		if (width > max_width)
			max_width = width;
	}
	m->height += menu_font->height+2;
	m->width = max_width + 6;
	m->height = n_items * (menu_font->height + 1);
	if (title)
		m->height += menu_font->height+1;
	return m;
}

void draw_menu_back(int x1, int y1, int width, int height)
{
	int x2, y2;

	x2 = x1 + width;
	y2 = y1 + height;
	rectangle(x1, y1, x2, y2, SKY);
	hline_prim(x1, x2 + 1, y1 - 1,     COL(C_MENU1));
	hline_prim(x1, x2 + 1, y2 + 1,     COL(C_MENU1));
	vline_prim(y1 - 1, y2 + 1, x1 - 1, COL(C_MENU1));
	vline_prim(y1, y2, x2 + 1,         COL(C_MENU1));
}

void show_menu_item(int x, int y, Menu_item *item, int col)
{
	if (item->value != NULL) {
		if (*item->value)
			character(x, y + menu_font->height - 5, &check_char, RED);
		else
			rectangle(x, y, x + 5, y + menu_font->height - 1, SKY);
	}
	draw_string(x + 6, y, item->s, col);
}

void show_menu(int x, int y, Menu *m)
{
	int i, y1, ystep;

	draw_menu_back(x, y, m->width, m->height);
	if (m->title) {
		draw_string(x + 6, y+1, m->title, C_MTEXT1);
		y1 = y + 1 + menu_font->height + 2;
		hline_prim(x, x + m->width, y1-2, COL(C_MENU1));
	} else {
		y1 = y + 1;
	}
	ystep = menu_font->height + 1;
	for (i = 0; i < m->n_items; i++, y1 += ystep) {
		show_menu_item(x, y1, &m->items[i], C_MTEXT1);
	}
}

int process_menu(int x, int y, Menu *m, int sel)
{
	int i, c, choosing, last_sel, ystep;

	m->x = x;
	m->y = y;
	show_menu(x, y, m);
	last_sel = sel;
	ystep = menu_font->height + 1;
	y += (m->title) ? menu_font->height + 3 : 1;
	show_menu_item(x, y + (sel * ystep), &m->items[sel], C_MTEXT2);
	choosing = TRUE;
	while (choosing) {
		show_menu(m->x, m->y, m);
		show_menu_item(x, y + (sel * ystep), &m->items[sel], C_MTEXT2);
		/*if (sel != last_sel) {
		    show_menu_item(x, y + (last_sel * ystep), &m->items[last_sel], C_MTEXT1);
			show_menu_item(x, y + (sel * ystep), &m->items[sel], C_MTEXT2);
			last_sel = sel;
		}*/
		c = wait_keypress();
		if (c == ESCAPE) {
			sel = -1;
			choosing = FALSE;
		} else if (c == ENTER) {
			if (m->items[sel].flags & M_TOGGLE) {
				*m->items[sel].value = !*m->items[sel].value;
			} else if (m->items[sel].flags & M_RADIO) {
				for (i = 0; i < m->n_items; i++) {
					if (m->items[i].flags & M_RADIO) {
						*m->items[i].value = FALSE;
					}
				}
				*m->items[sel].value = TRUE;
			}
            if (m->action) {
                (*m->action)(m, sel);
			}
			choosing = FALSE;
		} else if (c == SPACE) {
			if (m->items[sel].flags & M_TOGGLE) {
				*m->items[sel].value = !*m->items[sel].value;
				show_menu_item(x, y + (sel * ystep), &m->items[sel], C_MTEXT2);
			} else if (m->items[sel].flags & M_RADIO) {
				for (i = 0; i < m->n_items; i++) {
					if (m->items[i].flags & M_RADIO) {
						*m->items[i].value = FALSE;
					}
					show_menu_item(x, y + (i * ystep), &m->items[i], C_MTEXT1);
				}
				*m->items[sel].value = TRUE;
				show_menu_item(x, y + (sel * ystep), &m->items[sel], C_MTEXT2);
			}
            if (m->action) {
                (*m->action)(m, sel);
			}
		} else if (((c == UPARROW) || (c == SHIFT(UPARROW))) && (sel > 0)) {
			sel--;
		} else if (((c == DOWNARROW) || (c == SHIFT(DOWNARROW))) && (sel < (m->n_items - 1))) {
			sel++;
		}
	}
	return sel;
}

void process_menus(void)
{
	int sel, code;

	escape_flag = FALSE;
	merge_screen();
	use_font("FONT58");
	sel = process_menu(30, 20, top_menu, 0);
	escape_flag = FALSE;
	if (sel == M_EXIT) {
		alert_box(160, 20, "CONFIRM EXIT? (Y/N)");
		code = wait_ascii();
		if ((code == 'y') || (code == 'Y')) {
			escape_flag = TRUE;
			/* dos_quit_flag = TRUE; */
		}
	}
	split_screen();
	if (calibrate_flag) {
		calibrate_joystick();
		calibrate_flag = FALSE;
		escape_flag = FALSE;
	}
}

static void save_menu_defaults(void)
{
	FILE *fp;
	char file_name[100];

	sprintf(file_name, "%sMENU.CFG", save_path);
	fp = fopen(file_name, "w");
	if (fp != NULL) {
		fprintf(fp, "%d %d %d %d\n", analog_on, mouse_on, keyboard_on, shading_on);
		fprintf(fp, "%d %d %d %d\n", crash_on, g_effects_on, engine_noise_on, noise_on);
		fclose(fp);
	}
}

void load_menu_defaults(void)
{
	FILE *fp;
	char file_name[100];

	sprintf(file_name, "%sMENU.CFG", save_path);
	fp = fopen(file_name, "r");
	if (fp != NULL) {
		fscanf(fp, "%d %d %d %d\n", &analog_on, &mouse_on, &keyboard_on, &shading_on);
		fscanf(fp, "%d %d %d %d\n", &crash_on, &g_effects_on, &engine_noise_on, &noise_on);
		fclose(fp);
	}
}

/* Menu action functions
 * ---------------------
 * */
static int menu1_action(Menu *m, int sel)
{
	int y;

	y = m->y + ((m->title) ? menu_font->height + 2 : 0);

	switch (sel) {
		case 0: process_menu(m->x + m->width + 2, y, control_menu, 0); break;
		case 1: process_menu(m->x + m->width + 2, y, display_menu, 0); break;
		case 2: process_menu(m->x + m->width + 2, y, simulation_menu, 0); break;
        case 3: process_menu(m->x + m->width + 2, y, sound_menu, 0); break;
		case 4: save_menu_defaults(); save_joystick_defaults(); break;
	}
	return sel;
}

static int menu2_action(Menu *m, int sel)
{
	set_control_type(sel);
	if (sel == 0)
		process_menu(m->x + m->width + 2, m->y, joystick_menu, 0);
	setup_analog_useage();

	return sel;
}

/* For stick selection switch()
 * sets mutually exclusive values to zero
 */
int menu21_action(Menu *m, int sel)
{
	switch (sel) {
		case 0: calibrate_flag = TRUE;
			break;
		case 1: standard_joystick 	= TRUE;
				bios_joystick 		= FALSE;
		        thrustmaster_stick 	= FALSE;
				flightstick_pro = FALSE;
				use_bios_gameport(FALSE);
				break;
		case 2: analog_rudder = !analog_rudder;
				break;
		case 3: analog_throttle = !analog_throttle;
		        if (analog_throttle) {
				  thrustmaster_throttle = FALSE;
				}
				break;
		case 4:
				standard_joystick = TRUE;
				flightstick_pro = TRUE;
				analog_throttle = TRUE;
				thrustmaster_stick 	= FALSE;
				break;
		case 5:
				if (thrustmaster_stick == FALSE) {
					thrustmaster_stick 	= TRUE;
					standard_joystick  	= FALSE;
					flightstick_pro = FALSE;
					bios_joystick 	   	= FALSE;
					analog_throttle    	= FALSE;
				} else {
					thrustmaster_stick 	= FALSE;
					standard_joystick 	= TRUE;
					bios_joystick 	   	= FALSE;
				}
				break;
		case 6:
				if (thrustmaster_throttle == FALSE) {
					thrustmaster_throttle = TRUE;
		        	analog_throttle = FALSE;
				} else {
					thrustmaster_throttle = FALSE;
				}
				break;
		case 7: bios_joystick 		= TRUE;
		        standard_joystick 	= FALSE;
		        thrustmaster_stick 	= FALSE;
				use_bios_gameport(TRUE);
				break;
		default:
		    break;
	}
	return sel;
}

extern int distance_factor;
static int menu3_action(Menu *m, int sel)
{
	switch (sel) {
		case 0: break;
		case 1:
			distance_factor -= 1;
			distance_factor = MAX(distance_factor, -3);
			break;
		case 2:
			distance_factor += 1;
			distance_factor = MIN(distance_factor, 3);
			break;
	}
	return sel;
}

/* Draw box in centre of screen */
extern void alert_box(int cx, int cy, char *message)
{
	int w, h;

	cx = screen_width/2;
	draw_string_size(message, &w, &h);
	w = (w / 2) + 2;
	h = (h / 2) + 1;
	cy += h;
	rectangle(cx - w, cy - h, cx + w, cy + h, SKY);
	hline_prim(cx - w - 1, cx + w + 1, cy - h - 1, COL(RED));
	hline_prim(cx - w - 1, cx + w + 1, cy + h + 1, COL(RED));
	vline_prim(cy - h, cy + h, cx - w - 1, COL(RED));
	vline_prim(cy - h, cy + h, cx + w + 1, COL(RED));
	draw_string(cx - w + 2, cy - h + 2, message, RED);
}

void init_menus(void)
{
}

void setup_menus(void)
{
	menu_font 		= use_font("FONT58");
	top_menu 		= create_menu("FSF Simulator", menu1_d,  MENU1_SIZE,  menu1_action);
	control_menu 	= create_menu(NULL, menu2_d,  MENU2_SIZE,  menu2_action);
	joystick_menu 	= create_menu(NULL, menu21_d, MENU21_SIZE, menu21_action);
	display_menu 	= create_menu(NULL, menu3_d,  MENU3_SIZE,  menu3_action);
    simulation_menu = create_menu(NULL, menu4_d,  MENU4_SIZE,  NULL);
    sound_menu 		= create_menu(NULL, menu5_d,  MENU5_SIZE,  NULL);
}

