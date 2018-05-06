/* PC specific functions for Watcom compiler */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <time.h>

#include <direct.h>
#include <io.h>
#include <sys\stat.h>
#include <dos.h>

#include "graph.h"
#include "io.h"
#include "keys.h"

extern int error_state;

volatile long total_time = 0;

unsigned _stack = 0x10000;

static int sound_status;
static volatile int sound_on = FALSE;
static int sound_index = 0, *sound_buffer = NULL;
static long old_tick_count = 0;

extern int draw_string(int x, int y, char *s, int col);

static int bios_gameport_on = FALSE;

extern int get_gameport(int *values, int timeout);
extern int get_gameport2(int *values, int timeout);

static int (*get_analog)(int *values, int timeout);
static int (*get_analog2)(int *values, int timeout);

int music_on = FALSE;

static union REGS r;

static int pointer_x = 160, pointer_y = 100;

static int analog_modes[4] = {2, 2, 0, 0};
static int analog_values[4] = {0, 0, 0, 0};
static int analog_orig[4] = {300, 300, 300, 300};
static int analog_min[4] = {0, 0, 0, 0};
static int analog_max[4] = {600, 600, 600, 600};
static int analog_timeout = 1200;
static int using_adc = FALSE;

static int mouse_ok = FALSE;
static int mouse_xorig = 160, mouse_yorig = 100;

static int pointer_analog_on = TRUE;

/*void debug(char *message)
{
	FILE *fp;

	fp = fopen("error", "a");
	if (fp != NULL) {
		fprintf(fp, "%s\n", message);
		fclose(fp);
	}
}*/

/* keyboard functions */

volatile int key_events = 0, key_code = 0, key_pressed = 0;
unsigned char key_table[128];

/* Cuts out BIOS */
void _interrupt _far keyboard_int(void)
{
	int sk;

	key_events++;
	sk = inp(0x60);
	if ((sk & 0x80) == 0x80) {
		key_table[sk & 0x7f] &= ~KEY_DOWN;
		key_pressed = 0;
	} else {
		key_table[sk & 0x7f] |= (KEY_DOWN | KEY_PRESSED);
		key_code = sk & 0x7f;
		key_pressed = sk & 0x7f;
	}
/*	if ((key_table[CTRL_KEY] & KEY_DOWN) && (key_code == 1)) {
		stop("Crash out");
	}*/
	sk = inp(0x61);
	outp(0x61, sk | 0x80);
	outp(0x61, sk);
	outp(0x20, 0x20);
	_enable();
}

#define MAX_PENDING 4
static int pending_keys[MAX_PENDING] = {0, 0, 0, 0};
static int keys_pending_in = 0;
static int keys_pending_out = 0;

/* called once per frame to set up key_table in non-interrupt case */
void update_keyboard(void)
{
}

void (_interrupt _far *prev_int_09)(void);

void setup_keyboard(void)
{
	int i;

	key_events = 0;
	for (i = 0; i < 128; i++)
		key_table[i] = 0;
	key_code = 0;
	key_pressed = 0;
	prev_int_09 = _dos_getvect(0x09);
	_dos_setvect(0x09, keyboard_int);
}

void tidyup_keyboard(void)
{
	_dos_setvect(0x09, prev_int_09);
}

static int bios_getchar(void)
{
	r.x.eax = 0x0000;
	int386(0x16, &r, &r);
	return r.h.al;
}

void enable_keyboard(void)
{
	rectangle(20, 10, screen_width - 20, 80, 0);
	draw_string(30, 30, "Standard key input enabled", 15);
	draw_string(30, 40, "Press Enter to finish", 15);
	swap_screen();
	tidyup_keyboard();
	while (bios_getchar() != 13)
		;
	setup_keyboard();
	/* restore tick to 100 hz */
	outp(0x43, 0x36);
	outp(0x40, 0x14);
	outp(0x40, 0x2e);
}

int key_down(int code)
{
	return(key_table[code] & KEY_DOWN);
}

void flush_keyboard(void)
{
	key_code = 0;
}

static char ascii_table1[0x40] = {
	0,27,'1','2', '3','4','5','6', '7','8','9','0', '-','=',8,9,
	'q','w','e','r', 't','y','u','i', 'o','p','[',']' ,13,'0','a','s',
	'd','f','g','h', 'j','k','l',';' ,39,'#',0,'\\', 'z','x','c','v',
	'b','n','m',',', '.','/',0,'*', 0,' ',0,0, 0,0,0,0,
};
static char ascii_table2[0x40] = {
	0,27,'!','"', 'œ','$','%','^', '&','*','(',')', '_','+',8,9,
	'Q','W','E','R', 'T','Y','U','I', 'O','P','{','}' ,13,'0','A','S',
	'D','F','G','H', 'J','K','L',':' ,'@','~',0,'|', 'Z','X','C','V',
	'B','N','M','<', '>','?',0,'*', 0,' ',0,0, 0,0,0,0,
};

int wait_ascii(void)
{
	int c;

	while (key_code == 0)
		;
	if (key_code >= 0x40) {
		key_code = 0;
		return 0;
	}
	if ((key_table[LEFT_SHIFT_KEY] & KEY_DOWN) || (key_table[RIGHT_SHIFT_KEY] & KEY_DOWN))
		c = ascii_table2[key_code];
	else
		c = ascii_table1[key_code];
	key_code = 0;
	return c;
}

/* must call update_keyboard() in non-interrupt case */
/* returns keyboard scan code */
/* May add SHIFT_OFFSET, CTRL_OFFSET or ALT_OFFSET  */
int check_keycode(void)
{
    int sk;
	unsigned int status;

	sk = 0;
	if (key_code != 0) {
		sk = key_code;
		key_code = 0;
		if (sk == 1)
			escape_flag = TRUE;
	}
	if (sk > 0) {
		if (key_table[LEFT_SHIFT_KEY] & KEY_DOWN)
			sk += SHIFT_OFFSET;
		if (key_table[RIGHT_SHIFT_KEY] & KEY_DOWN)
			sk += SHIFT_OFFSET;
		if (key_table[CTRL_KEY] & KEY_DOWN)
			sk += CTRL_OFFSET;
		if (key_table[ALT_KEY] & KEY_DOWN)
			sk += ALT_OFFSET;
		return sk;
	}
	return 0;
}

int wait_keypress(void)
{
    int waiting, sk, mb, ab, dummy;

	mb = 0;
	ab = 0;
	waiting = TRUE;
	while (waiting) {
/*		mb = read_mouse(&dummy, &dummy);
		ab = read_analog(analog_modes, analog_values);*/
		if ((key_pressed == 0) && (mb == 0) && (ab == 0)) {
			waiting = FALSE;
		}
	}
	waiting = TRUE;
	while (waiting) {
		if ((sk = check_keycode()) != 0) {
			waiting = FALSE;
		}
/*		mb = read_mouse(&dummy, &dummy);
		ab = read_analog(analog_modes, analog_values);*/
		if ((mb != 0) || (ab != 0)) {
			sk = ENTER;
			waiting = FALSE;
		}
	}
	return sk;
}

/* Pointer functions use mouse, joystick and keyboard */

void set_pointer(int x, int y)
{
	pointer_x = x;
	pointer_y = y;
	set_mouse(x, y);
}

int get_pointer(int *key, int *px, int *py)
{
	static int kx = 0, ky = 0, first_time = TRUE;
	int non_mouse, buttons, ax, ay, sk, analog[4];

	non_mouse = FALSE;
	if (first_time) {
		first_time = FALSE;
		if (check_analog())
			pointer_analog_on = TRUE;
		else
			pointer_analog_on = FALSE;
	}
	buttons = 0;
	if (mouse_ok) {
		r.x.eax = 0x0003;
		int386(0x33, &r, &r);
		pointer_x = r.x.ecx;
		pointer_y = r.x.edx;
		buttons |= r.x.ebx;
	}
	if (pointer_analog_on) {
		buttons |= read_analog2(analog);
		ax = analog[0];
		ay = analog[1];
		if ((ABS(ax) > 16) || (ABS(ay) > 16)) {
			pointer_x += SR(ax, 4);
			pointer_y += SR(ay, 4);
			non_mouse = TRUE;
		}
	}
	update_keyboard();
	if (key_down(RIGHTARROW)) {
		if (kx < 8)
			kx += 1;
	} else if (key_down(LEFTARROW)) {
		if (kx > -8)
			kx -= 1;
	} else {
		kx = 0;
	}
	if (key_down(DOWNARROW)) {
		if (ky < 8)
			ky += 1;
	} else if (key_down(UPARROW)) {
		if (ky > -8)
			ky -= 1;
	} else {
		ky = 0;
	}
	if ((sk = check_keycode()) != 0) {
        if (sk == SPACE) {
			buttons |= 1;
        } else if (sk == ENTER) {
            buttons |= 2;
		} else if (sk == ESCAPE) {
			escape_flag = TRUE;
		} else if ((sk == CTRL_K) || (sk == CTRL_M)) {
			pointer_analog_on = FALSE;
		} else if (sk == CTRL_J) {
			pointer_analog_on = TRUE;
		} else if ((sk == CTRL_Z) || (sk == KEY_Z)) {
			reset_analog();
		}
	}
	if ((kx != 0) || (ky != 0)) {
		pointer_x += SR(kx, 1);
		pointer_y += SR(ky, 1);
		non_mouse = TRUE;
	}
	if (pointer_x < 0)
		pointer_x = 0;
	if (pointer_x >= screen_width)
		pointer_x = screen_width - 1;
	if (pointer_y < 0)
		pointer_y = 0;
	if (pointer_y >= screen_height)
		pointer_y = screen_height - 1;
	if (non_mouse) {
		set_mouse(pointer_x, pointer_y);
	}
	*px = pointer_x;
	*py = pointer_y;
	*key = sk;
	return buttons;
}

/* control functions */

void stop_mouse(void)
{
	if (mouse_ok) {
		r.x.eax = 0x0021;
		int386(0x33, &r, &r);
		mouse_ok = FALSE;
	}
}

void show_mouse(void)
{
	if (mouse_ok) {
		r.x.eax = 0x0001;
		int386(0x33, &r, &r);
	}
}

void hide_mouse(void)
{
	if (mouse_ok) {
		r.x.eax = 0x0002;
		int386(0x33, &r, &r);
	}
}

void set_mouse(int x, int y)
{
	if (mouse_ok) {
		r.x.eax = 0x0004;
		r.x.ecx = x;
		r.x.edx = y;
		int386(0x33, &r, &r);
	}
}

void limit_mouse(int x1, int y1, int x2, int y2)
{
	if (mouse_ok) {
		r.x.eax = 0x0007;
		r.x.ecx = x1;
		r.x.edx = x2;
		int386(0x33, &r, &r);
		r.x.eax = 0x0008;
		r.x.ecx = y1;
		r.x.edx = y2;
		int386(0x33, &r, &r);
	}
}

/* Needs to be called whenever screen size or user interface is changed */
void reset_mouse(int x_range, int y_range)
{
	if (mouse_ok) {
		r.x.eax = 0x0021;
		int386(0x33, &r, &r);
		r.x.eax = 0x000f;
		r.x.ecx = 16;
		r.x.edx = 16;
		int386(0x33, &r, &r);
	}
	limit_mouse(0, 0, x_range, y_range);
	mouse_xorig = x_range / 2;
	mouse_yorig = y_range / 2;
	set_mouse(mouse_xorig, mouse_yorig);
}

int read_mouse(int *mx, int *my)
{
	if (mouse_ok) {
		r.x.eax = 0x0003;
		int386(0x33, &r, &r);
		*mx = r.x.ecx - mouse_xorig;
		*my = r.x.edx - mouse_yorig;
		return r.x.ebx;
	} else {
		*mx = 0;
		*my = 0;
		return 0;
	}
}

void setup_mouse(int x_range, int y_range)
{
	r.x.eax = 0;
	int386(0x33, &r, &r);
	if (r.x.eax == 0xffff) {
		mouse_ok = TRUE;
	}
	reset_mouse(x_range, y_range);
}

void tidyup_mouse(void)
{
}

/* PC games port analog */

/* BIOS functions - modified to work with NOTEBOOK GAMEPORT device */

static int gameport_bx = 0;
static int32 last_gameport_time = 0L;
static int last_gameport_values[4] = {128, 128, 128, 128};
static int last_gameport_buttons = 0;

static int setup_bios_gameport(void)
{
	last_gameport_time = total_time;
	r.h.ah = 0x84;
	r.h.al = 0x00;
	r.x.ebx = gameport_bx;
	r.x.edx = 0xf0;
	int386(0x15, &r, &r);
	gameport_bx = r.x.ebx;
	if (r.x.eax == 'SG')
		return TRUE;
	return FALSE;
}

static int get_bios_gameport(int *values, int timeout)
{
	int buttons;

	if ((total_time - last_gameport_time) >= 10) {
		last_gameport_time = total_time;
		r.h.ah = 0x84;
		r.h.al = 0x04;
		r.x.ebx = gameport_bx;
		r.x.edx = 1;
		int386(0x15, &r, &r);
		last_gameport_values[0] = values[0] = r.x.eax;
		last_gameport_values[1] = values[1] = r.x.ebx;
		last_gameport_values[2] = values[2] = r.x.ecx;
		last_gameport_values[3] = values[3] = r.x.edx;
		r.h.ah = 0x84;
		r.h.al = 0x00;
		r.x.ebx = gameport_bx;
		r.x.edx = 0;
		int386(0x15, &r, &r);
		last_gameport_buttons = buttons = (~r.h.al >> 4) & 0x000f;
		return buttons;
	}
	values[0] = last_gameport_values[0];
	values[1] = last_gameport_values[1];
	values[2] = last_gameport_values[2];
	values[3] = last_gameport_values[3];
	return last_gameport_buttons;
}

static int get_bios_gameport2(int *values, int timeout)
{
	int buttons;

	if ((total_time - last_gameport_time) >= 10) {
		last_gameport_time = total_time;
		r.h.ah = 0x84;
		r.h.al = 0x01;
		r.x.ebx = gameport_bx;
		r.x.edx = 1;
		int386(0x15, &r, &r);
		last_gameport_values[0] = values[0] = r.x.eax;
		last_gameport_values[1] = values[1] = r.x.ebx;
		last_gameport_values[2] = values[2] = 0;
		last_gameport_values[3] = values[3] = 0;
		r.h.ah = 0x84;
		r.h.al = 0x00;
		r.x.ebx = gameport_bx;
		r.x.edx = 0;
		int386(0x15, &r, &r);
		last_gameport_buttons = buttons = (~r.h.al >> 4) & 0x000f;
		return buttons;
	}
	values[0] = last_gameport_values[0];
	values[1] = last_gameport_values[1];
	values[2] = last_gameport_values[2];
	values[3] = last_gameport_values[3];
	return last_gameport_buttons;
}

static int c_gameport(int *values, int timeout);
static int c_gameport2(int *values, int timeout);

void use_bios_gameport(int state)
{
	static int first_time = TRUE;

	bios_gameport_on = state;
	if (bios_gameport_on) {
		if (first_time) {
			first_time = FALSE;
			setup_bios_gameport();
		}
		get_analog = get_bios_gameport;
		get_analog2 = get_bios_gameport2;
	} else {
/*		get_analog = get_gameport;
		get_analog2 = get_gameport2;*/
		get_analog = c_gameport;
		get_analog2 = c_gameport2;
	}
}

static int c_gameport(int *values, int timeout)
{
	register int i, bits, last_bits, buttons;
	int v1, v2, v3, v4;

	outp(0x201, 0);
	last_bits = bits = 0x000f;
	buttons = 0;
	v1 = v2 = v3 = v4 = 0;
	_disable();
	for (i = 0; (i < timeout) && (bits != 0); i++) {
		bits = inp(0x201);
		buttons |= bits;
		bits &= 0x000f;
		if (((bits & 1) == 0) && ((last_bits & 1) == 1))
			v1 = i;
		if (((bits & 2) == 0) && ((last_bits & 2) == 2))
			v2 = i;
		if (((bits & 4) == 0) && ((last_bits & 4) == 4))
			v3 = i;
		if (((bits & 8) == 0) && ((last_bits & 8) == 8))
			v4 = i;
		last_bits = bits;
	}
	_enable();
	values[0] = v1;
	values[1] = v2;
	values[2] = v3;
	values[3] = v4;
	return((~buttons >> 4) & 0x000f);
}

static int c_gameport2(int *values, int timeout)
{
	register int i, bits, last_bits, buttons;
	int v1, v2, v3, v4;

	outp(0x201, 0);
	last_bits = bits = 0x000f;
	buttons = 0;
	v1 = v2 = 0;
	_disable();
	for (i = 0; (i < timeout) && (bits != 0); i++) {
		bits = inp(0x201);
		buttons |= bits;
		bits &= 0x000f;
		if (((bits & 1) == 0) && ((last_bits & 1) == 1))
			v1 = i;
		if (((bits & 2) == 0) && ((last_bits & 2) == 2))
			v2 = i;
		last_bits = bits;
	}
	_enable();
	values[0] = v1;
	values[1] = v2;
	values[2] = 0;
	values[3] = 0;
	return((~buttons >> 4) & 0x000f);
}

/* TRUE if joystick plugged in */
int check_analog(void)
{
	int i, analog[4];

	for (i = 0; i < 10; i++) {
		get_analog(analog, 3000);
		if ((analog[0] <= 0) || (analog[1] <= 0)) {
			return FALSE;
		}
		if ((analog[0] > 3000) || (analog[1] > 3000)) {
			return FALSE;
		}
	}
	return TRUE;
}

void reset_analog(void)
{
	using_adc = FALSE;
	get_analog(analog_orig, analog_timeout);
}

int read_analog2(int values[4])
{
	register int i, d, av, buttons;

	buttons = get_analog2(values, analog_timeout);
	for (i = 0; i < 2; i++) {
		av = values[i];
		av -= analog_orig[i];
		if (av < 0) {
			d = analog_orig[i] - analog_min[i];
			if (d > 0)
				av = MULDIV(av, 128, d);
		} else {
			d = analog_max[i] - analog_orig[i];
			if (d > 0)
				av = MULDIV(av, 128, d);
		}
		values[i] = av;
	}
	values[2] = 0;
	values[3] = 0;
	return buttons;
}

int read_analog(int modes[4], int values[4])
{
	int i, d, av, buttons;

	buttons = get_analog(values, analog_timeout);
	for (i = 0; i < 4; i++) {
		if (modes[i] > 0) {
			av = values[i];
			if (modes[i] == 1) {
				av -= analog_min[i];
				if (av < 0)
					av = 0;
				d = analog_max[i] - analog_min[i];
				if (d > 0)
					av = MULDIV(av, 256, d);
			} else if (modes[i] == 2) {
				av -= analog_orig[i];
				if (av < 0) {
					d = analog_orig[i] - analog_min[i];
					if (d > 0)
						av = MULDIV(av, 128, d);
				} else {
					d = analog_max[i] - analog_orig[i];
					if (d > 0)
						av = MULDIV(av, 128, d);
				}
			}
			values[i] = av;
		}
	}
	return buttons;
}

void setup_analog(void)
{
	int i, max;

	analog_timeout = 3000;
	reset_analog();
	max = 0;
	for (i = 0; i < 4; i++) {
		analog_min[i] = 0;
		analog_max[i] = analog_orig[i] * 2;
		if (analog_max[i] > max)
			max = analog_max[i];
	}
	if (max < 1000)
		analog_timeout = 1300;
	else if (max < 3000)
		analog_timeout = max + 300;
	else
		analog_timeout = 3000;
}

void set_analog_limits(int min[4], int max[4])
{
	int i, mx;

	mx = 0;
	for (i = 0; i < 4; i++) {
		analog_min[i] = min[i];
		analog_max[i] = max[i];
		if (analog_max[i] > mx)
			mx = analog_max[i];
	}
	if (mx < 1000)
		analog_timeout = 1200;
	else if (mx < 3000)
		analog_timeout = mx + 200;
	else
		analog_timeout = 3000;
}

void get_analog_limits(int min[4], int max[4])
{
	int i;

	for (i = 0; i < 4; i++) {
		min[i] = analog_min[i];
		max[i] = analog_max[i];
	}
}

static int get_scaled(int val, int orig, int max)
{
	if (orig > 0)
		val = MULDIV(val - orig, max, orig);
	if (val < -max)
		return -max;
	else if (val > max)
		return max;
	return val;
}

void calibrate_analog(int modes[4], int col1, int col2, int col3, int col4)
{
	int i, x, y, cx, cy, last_c3, last_c4, buttons, max, analog[4];

	for (i = 0; i < 4; i++) {
		analog_min[i] = 32000;
		analog_max[i] = -32000;
	}
	max = -32000;
	cx = screen_width / 2;
	cy = 90;
	rectangle(cx - 70, cy - 84, cx + 76, cy + 82, col2);
	hline_prim(cx - 71, cx + 77, cy - 85, COL(col3));
	hline_prim(cx - 71, cx + 77, cy + 83, COL(col4));
	vline_prim(cy - 84, cy + 82, cx - 71, COL(col3));
	vline_prim(cy - 84, cy + 82, cx + 77, COL(col4));
	rectangle(cx - 64, cy - 64, cx + 64, cy + 64, 0);
	rectangle(cx - 64, cy + 68, cx + 64, cy + 72, 0);
	rectangle(cx + 68, cy - 64, cx + 72, cy + 64, 0);
	while (check_keycode() != 0)
		;
	if ((modes[2] == 0) && (modes[3] == 0))
		draw_string(cx - 34, cy - 82, "CENTRE STICK", col1);
	else
		draw_string(cx - 44, cy - 82, "CENTRE CONTROLS", col1);
	draw_string(cx - 40, cy - 73, "THEN PRESS FIRE", col1);
	buttons = 0;
	while (buttons == 0) {
		if (using_adc)
			buttons = (read_raw_adc(4, analog) | check_keycode());
		else
			buttons = (get_analog(analog, 3000) | check_keycode());
	}
	for (i = 0; i < 4; i++) {
		analog_orig[i] = analog[i];
	}
	if (modes[3] == 1) {
		if (analog_orig[3] < 60)
			analog_orig[3] = 60;
	}
	while (buttons != 0) {
		if (using_adc)
			buttons = (read_raw_adc(4, analog) | check_keycode());
		else
			buttons = (get_analog(analog, 3000) | check_keycode());
	}
	pixel_prim(cx, cy, COL(col1));
	rectangle(cx - 70, cy - 84, cx + 76, cy - 65, col2);
	if ((modes[2] == 0) && (modes[3] == 0))
		draw_string(cx - 55, cy - 82, "MOVE STICK TO LIMITS", col1);
	else
		draw_string(cx - 60, cy - 82, "MOVE CONTROLS TO LIMITS", col1);
	draw_string(cx - 40, cy - 73, "THEN PRESS FIRE", col1);
	last_c3 = 0;
	last_c4 = 0;
	buttons = 0;
	while (buttons == 0) {
		if (using_adc)
			buttons = (read_raw_adc(4, analog) | check_keycode());
		else
			buttons = (get_analog(analog, 3000) | check_keycode());
		x = get_scaled(analog[0], analog_orig[0], 63);
		y = get_scaled(analog[1], analog_orig[1], 63);
		pixel_prim(cx + x, cy + y, COL(col1));
		rectangle(cx - 56, cy + 75, cx - 40, cy + 81, 0);
		vector_number(analog[0], cx - 42, cy + 78, col1);
		rectangle(cx - 26, cy + 75, cx - 10, cy + 81, 0);
		vector_number(analog[1], cx - 12, cy + 78, col1);
		if (modes[2] > 0) {
			x = get_scaled(analog[2], analog_orig[2], 63);
			vline_prim(cy + 68, cy + 72, cx + last_c3, 0);
			vline_prim(cy + 68, cy + 72, cx + x, COL(col1));
			rectangle(cx + 10, cy + 75, cx + 26, cy + 81, 0);
			vector_number(analog[2], cx + 24, cy + 78, col1);
			last_c3 = x;
		}
		if (modes[3] > 0) {
			y = get_scaled(analog[3], analog_orig[3], 63);
			hline_prim(cx + 68, cx + 72, cy + last_c4, 0);
			hline_prim(cx + 68, cx + 72, cy + y, COL(col1));
			rectangle(cx + 40, cy + 75, cx + 56, cy + 81, 0);
			vector_number(analog[3], cx + 54, cy + 78, col1);
			last_c4 = y;
		}
		for (i = 0; i < 4; i++) {
			if (analog[i] < analog_min[i])
				analog_min[i] = analog[i];
			if (analog[i] > analog_max[i])
				analog_max[i] = analog[i];
			if (analog_max[i] > max)
				max = analog_max[i];
		}
		if (max < 1000)
			analog_timeout = 1200;
		else if (max < 3000)
			analog_timeout = max + 200;
		else
			analog_timeout = 3000;
	}
	while (buttons != 0) {
		if (using_adc)
			buttons = (read_raw_adc(4, analog) | check_keycode());
		else
			buttons = (get_analog(analog, 3000) | check_keycode());
	}
}

/* ADC card I/O */

#define ADC_PORT 0x280

static int read_adc_chan(int chan)
{
	int v;

	outp(ADC_PORT + 10, chan);
	outp(ADC_PORT + 12, 0);
	v = inp(ADC_PORT + 5);
	while ((v & 0x10) == 0x10) {
		v = inp(ADC_PORT + 5);
	}
	v = (v & 0x0f) << 8;
	v |= inp(ADC_PORT + 4);
	return v;
}

/* Values must have space for channels */
unsigned read_raw_adc(int num_channels, int *values)
{
	unsigned i, disc;

	for (i = 0; i < num_channels; i++) {
		values[i] = read_adc_chan(i);
	}
	disc = inp(ADC_PORT + 6);
	return disc;
}

int read_adc(int modes[4], int values[4])
{
	int i, d, av, buttons;

	buttons = read_raw_adc(4, values);
	for (i = 0; i < 4; i++) {
		if (modes[i] > 0) {
			av = values[i];
			if (modes[i] == 1) {
				av -= analog_min[i];
				if (av < 0)
					av = 0;
				d = analog_max[i] - analog_min[i];
				if (d > 0)
					av = MULDIV(av, 256, d);
			} else if (modes[i] == 2) {
				av -= analog_orig[i];
				if (av < 0) {
					d = analog_orig[i] - analog_min[i];
					if (d > 0)
						av = MULDIV(av, 128, d);
				} else {
					d = analog_max[i] - analog_orig[i];
					if (d > 0)
						av = MULDIV(av, 128, d);
				}
			}
			values[i] = av;
		}
	}
	return buttons;
}

void reset_adc(void)
{
	int i;

	read_raw_adc(4, analog_orig);
	for (i = 0; i < 4; i++) {
		analog_min[i] = 0;
		analog_max[i] = analog_orig[i] * 2;
	}
	using_adc = TRUE;
}

/* Original 8 bit card */
/*#define ADC_PORT 0x170

void setup_adc(void)
{
	outp(ADC_PORT + 15, 0x9b);
}

static int read_adc_chan(int chan)
{
	int i, v1, v2;

	outp(ADC_PORT, chan + 8);
	outp(ADC_PORT + 1, 0);
	v1 = inp(ADC_PORT + 1);
	for (i = 0; (i < 200); i++) {
		v2 = inp(ADC_PORT + 1);
	}
	return v2;
}

void read_adc(int *enables, int *values, unsigned *discretes)
{
	unsigned i, disc;

	for (i = 0; i < 5; i++) {
		if (enables[i])
			values[i] = read_adc_chan(i);
	}
	disc = inp(ADC_PORT + 12);
	disc |= (inp(ADC_PORT + 14) << 2);
	*discretes = disc;
}*/

/* special purpose functions */

void cursor(int n)
{
}

void pc_sound(int *buffer, int length)
{
	sound_buffer = buffer;
	sound_index = length;
	sound_on = TRUE;
}

void tab(int x, int y)
{
	r.h.ah = 0x02;
	r.h.bh = 0;
	r.h.dh = y;
	r.h.dl = x;
	int386(0x10, &r, &r);
}

void debug_value(int val, int x, int y)
{
	int i, v, x1;

	merge_screen();
	if (val < 0) {
		v = -val;
		i = 8;
	} else {
		v = val;
		i = 4;
	}
	while (v > 9) {
		v /= 10;
		i += 4;
	}
	x1 = x - i;
	for (i = y - 3; i < (y + 4); i++)
		hline_prim(x1, x + 2, i, 0);
	vector_number(val, x, y, 15);
	split_screen();
}

/* OS utility functions */

long disk_free_space(void)
{
	return 0L;
/*	return dos_getdiskfreespace(0);*/
}

int set_path(char *path)
{
	int val;

	val = chdir(path);
	if (val == 0)
		return TRUE;
	else
		return FALSE;
}

static void sigfn(int type)
{
 switch (type) {
        case SIGINT :  escape_flag = TRUE;
					   stop("crash out");
                       break;
        default     :  set_mode(3);
					   break;
        }
}

static void (_interrupt _far *prev_int_08)(void);

void _interrupt _far clock_tick(void)
{
	static int count = 0;
	register int freq;

	total_time++;
	if (sound_index > 0) {
		freq = sound_buffer[sound_index--];
		outp(0x43, 0xb6);
		outp(0x42, freq & 0xff);
		outp(0x42, (freq >> 8) & 0xff);
		outp(0x61, sound_status | 0x03);
	} else if (sound_on) {
		outp(0x61, sound_status);
		sound_on = FALSE;
	}
	if (count++ >= 4) {
		count = 0;
		_chain_intr(prev_int_08);
	}
	outp(0x20, 0x20);
	_enable();
}

static void setup_timer(void)
{
	total_time = 0;
	/* read initial tick count */
	r.x.eax = 0x0000;
	int386(0x1a, &r, &r);
	old_tick_count = r.x.ecx;
	old_tick_count = (old_tick_count << 16) | r.x.edx;
	/* intercept timer interrupt */
	prev_int_08 = _dos_getvect(0x08);
	_dos_setvect(0x08, clock_tick);
	/* increase tick to 100 hz */
	outp(0x43, 0x36);
	outp(0x40, 0x14);
	outp(0x40, 0x2e);
}

static void tidyup_timer(void)
{
	/* reduce tick to 18 Hz default */
	outp(0x43, 0x36);
	outp(0x40, 0xff);
	outp(0x40, 0xff);
	/* restore old timer interrupt */
	_dos_setvect(0x08, prev_int_08);
	/* restore correct tick count */
	old_tick_count += (total_time * 182L) / 1000L;
	r.x.eax = 0x0100;
	r.x.ecx = (int)(old_tick_count >> 16);
	r.x.edx = (int)old_tick_count;
	int386(0x1a, &r, &r);
}

/*int div_zero(struct INT_DATA *pd)
{
	stop("Trapped real divide by zero");
	outp(0x20, 0x20);
	int_on();
	return 1;
}

int protect_fault(struct INT_DATA *pd)
{
	stop("Protection fault");
	return 1;
}

int critical_int(struct INT_DATA *pd)
{
	pd->regs.x.ax = 0;
	return 1;
}*/

void setup_program(long memory_needed)
{
	int c;
	long free_mem;

/*	get_analog = get_gameport;
	get_analog2 = get_gameport2;*/
	get_analog = c_gameport;
	get_analog2 = c_gameport2;
/*	int_intercept(0x00, div_zero, 256);
	int_intercept(0x24, critical_int, 256);*/
	/* read state of speaker control port */
	sound_status = inp(0x61) & 0xfc;
	setup_timer();
    srand(time(NULL));
    signal(SIGINT, sigfn);              /* called when ESC pressed */
}

void tidyup_program(void)
{
	tidyup_timer();
	/* switch off speaker */
	outp(0x61, sound_status);
/*	int_restore(0x24);
	int_restore(0x00);*/
}

