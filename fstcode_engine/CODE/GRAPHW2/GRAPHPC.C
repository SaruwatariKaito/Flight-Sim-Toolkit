/* graphpc.c */

#include <stdlib.h>
#include <dos.h>

#include "graph.h"

int screen_width = 320, screen_height = 200;
int screen_colours = 256;
int double_buffered = FALSE;

int draw_screen = 1, view_screen = 0;

static int num_colours = 0;
unsigned int *cols = NULL;

int screen_type = VGA_320x200_256;
int screen_mode = 3;
int overide_mode = 0;

static int start_screen_mode = 3;

/* Primitives are set up for each screen type */
void (*use_screen)(void);
void (*use_buffer)(void);
void (*update_screen)(int y1, int y2);
void (*vsync)(void);
void (*swap_hardware)(int draw_screen, int view_screen);

void (*palette_prim)(int n, int r, int g, int b);
void (*clip_prim)(int left, int right, int top, int bottom);
void (*clear_prim)(int y1, int y2, int col);
void (*pixel_prim)(int x, int y, int col);
int (*getpixel_prim)(int x, int y);
void (*block_prim)(int x, int y, int colour);
void (*hline_prim)(int x1, int x2, int y, int colour);
void (*vline_prim)(int y1, int y2, int x, int colour);
void (*line_prim)(int x1, int y1, int x2, int y2, int col);
void (*poly_prim)(int npts, sVector2 pts[], int colour);
void (*bitmap_prim)(int x, int y, int w, uchar *bitmap);
void (*getbitmap_prim)(int x, int y, int w, uchar *bitmap);
void (*char_prim)(int x, int y, int width, int pattern, int colour);
void (*remap_prim)(int x, int y, int w, uchar *map);
void (*raw_hline_prim)(int x1, int x2, int y, int colour);
void (*tex_hline_prim)(int x1, int x2, int y, int32 btx1, int32 btx2, int32 bty1, int32 bty2, int tw, uchar *tex);
void (*pctex_hline_prim)(int x1, int x2, float z1, float z2, int y, float btx1, float btx2, float bty1, float bty2, int tw, uchar *tex);
void (*tex_line_prim)(int x1, int y1, int x2, int y2, uchar *pixels, int length);


int32 divtab[768] = {
	65536,32768,21845,16384,13107,10923,9362,8192,7282,6554,5958,5461,5041,4681,4369,
	4096,3855,3641,3449,3277,3121,2979,2849,2731,2621,2521,2427,2341,2260,2185,2114,
	2048,1986,1928,1872,1820,1771,1725,1680,1638,1598,1560,1524,1489,1456,1425,1394,
	1365,1337,1311,1285,1260,1237,1214,1192,1170,1150,1130,1111,1092,1074,1057,1040,
	1024,1008,993,978,964,950,936,923,910,898,886,874,862,851,840,830,
	819,809,799,790,780,771,762,753,745,736,728,720,712,705,697,690,
	683,676,669,662,655,649,643,636,630,624,618,612,607,601,596,590,
	585,580,575,570,565,560,555,551,546,542,537,533,529,524,520,516,
	512,508,504,500,496,493,489,485,482,478,475,471,468,465,462,458,
	455,452,449,446,443,440,437,434,431,428,426,423,420,417,415,412,
	410,407,405,402,400,397,395,392,390,388,386,383,381,379,377,374,
	372,370,368,366,364,362,360,358,356,354,352,350,349,347,345,343,
	341,340,338,336,334,333,331,329,328,326,324,323,321,320,318,317,
	315,314,312,311,309,308,306,305,303,302,301,299,298,297,295,294,
	293,291,290,289,287,286,285,284,282,281,280,279,278,277,275,274,
	273,272,271,270,269,267,266,265,264,263,262,261,260,259,258,257,
	256,255,254,253,252,251,250,249,248,247,246,245,245,244,243,242,
	241,240,239,238,237,237,236,235,234,233,232,232,231,230,229,228,
	228,227,226,225,224,224,223,222,221,221,220,219,218,218,217,216,
	216,215,214,213,213,212,211,211,210,209,209,208,207,207,206,205,
	205,204,204,203,202,202,201,200,200,199,199,198,197,197,196,196,
	195,194,194,193,193,192,192,191,191,190,189,189,188,188,187,187,
	186,186,185,185,184,184,183,183,182,182,181,181,180,180,179,179,
	178,178,177,177,176,176,175,175,174,174,173,173,172,172,172,171,
	171,170,170,169,169,168,168,168,167,167,166,166,165,165,165,164,
	164,163,163,163,162,162,161,161,161,160,160,159,159,159,158,158,
	158,157,157,156,156,156,155,155,155,154,154,153,153,153,152,152,
	152,151,151,151,150,150,150,149,149,149,148,148,148,147,147,147,
	146,146,146,145,145,145,144,144,144,143,143,143,142,142,142,142,
	141,141,141,140,140,140,139,139,139,139,138,138,138,137,137,137,
	137,136,136,136,135,135,135,135,134,134,134,133,133,133,133,132,
	132,132,132,131,131,131,131,130,130,130,130,129,129,129,129,128,
	128,128,128,127,127,127,127,126,126,126,126,125,125,125,125,124,
	124,124,124,123,123,123,123,122,122,122,122,122,121,121,121,121,
	120,120,120,120,120,119,119,119,119,119,118,118,118,118,117,117,
	117,117,117,116,116,116,116,116,115,115,115,115,115,114,114,114,
	114,114,113,113,113,113,113,112,112,112,112,112,111,111,111,111,
	111,111,110,110,110,110,110,109,109,109,109,109,109,108,108,108,
	108,108,107,107,107,107,107,107,106,106,106,106,106,106,105,105,
	105,105,105,105,104,104,104,104,104,104,103,103,103,103,103,103,
	102,102,102,102,102,102,101,101,101,101,101,101,101,100,100,100,
	100,100,100,99,99,99,99,99,99,99,98,98,98,98,98,98,
	98,97,97,97,97,97,97,97,96,96,96,96,96,96,96,95,
	95,95,95,95,95,95,94,94,94,94,94,94,94,93,93,93,
	93,93,93,93,93,92,92,92,92,92,92,92,92,91,91,91,
	91,91,91,91,91,90,90,90,90,90,90,90,90,89,89,89,
	89,89,89,89,89,88,88,88,88,88,88,88,88,87,87,87,
	87,87,87,87,87,87,86,86,86,86,86,86,86,86,86,85,
	85,
};

void swap_screen(void)
{
	draw_screen = 1 - draw_screen;
    view_screen = 1 - view_screen;
	swap_hardware(draw_screen, view_screen);
}

void merge_screen(void)
{
	swap_hardware(draw_screen, draw_screen);
}

void split_screen(void)
{
	swap_hardware(draw_screen, view_screen);
}

extern void setup_vga13(void);
extern void tidyup_vga13(void);
extern void reset_vga13(void);

extern void setup_vesa(void);
extern void tidyup_vesa(void);
extern void reset_vesa(void);

/* Called to initially setup screen */
static void setup_screen(int type)
{
	int i, a;

	screen_type = type;
	switch (screen_type) {
    	case VGA_320x200_256:
			setup_vga13();
			break;
    	case VESA_640x480_256:
			setup_vesa();
			break;
		default:
			setup_vga13();
			break;
	}
	split_screen();
}

/* Called to change screen */
static void reset_screen(int type)
{
	int i, a;

	screen_type = type;
	switch (screen_type) {
    	case VGA_320x200_256:
			reset_vga13();
			break;
    	case VESA_640x480_256:
			reset_vesa();
			break;
		default:
			setup_vga13();
			break;
	}
	split_screen();
}

void tidyup_screen(void)
{
	merge_screen();
	switch (screen_type) {
    	case VGA_320x200_256:
			tidyup_vga13();
			break;
    	case VESA_640x480_256:
			tidyup_vesa();
			break;
	}
}

void load_palette(sColour *pal, int first, int last)
{
	int i;

	palette_prim(first, pal->red, pal->green, pal->blue);
	for (i = first + 1, pal++; i <= last; i++, pal++) {
		palette_prim(-1, pal->red, pal->green, pal->blue);
	}
}

void set_mode(int n)
{
	union REGS r;

	if (n > 0xff) {
		r.x.eax = 0x4f02;
		r.x.ebx = n;
		int386(0x10, &r, &r);
		if (r.h.al != 0x4f)
			stop("VESA bios is not responding");
		if (r.h.ah != 0x00)
			stop("Failed to set video mode");
	} else {
		r.h.ah = 0x00;
		r.h.al = n;
		int386(0x10, &r, &r);
	}
}

int get_mode(void)
{
	union REGS r;

	r.h.ah = 0x0f;
	int386(0x10, &r, &r);
	return r.h.al;
}

static void set_arc_colour(int index, int col)
{
	int r, g, b, w;

	w = (col & 0x03);
	r = (((col & 0x10) >> 1) | (col & 0x04) | w) * 255 / 15;
	g = (((col & 0x40) >> 3) | ((col & 0x20) >> 3) | w) * 255 / 15;
	b = (((col & 0x80) >> 4) | ((col & 0x08) >> 1) | w) * 255 / 15;
	palette_prim(index, r, g, b);
}

/* Duplicates the Archimedes palette */
static void setup_colours_256(void)
{
	int n;

	for (n = 0; n < 256; n++) {
		cols[n] = n | (n << 8) | (n << 16) | (n << 24);
		set_arc_colour(n, n);
	}
}

static int vgacols[16] = {
	0x00, 0x2c, 0x2e, 0x2f, 0xd0, 0xd2, 0xfc, 0xff,
	0x63, 0x14, 0x77, 0x37, 0x10, 0xa1, 0xa3, 0xc9,
};

static void setup_colours_16(void)
{
	int n, n1;

	for (n = 0; n < 16; n++) {
		cols[n] = n | (n << 8) | (n << 16) | (n << 24);
		set_arc_colour(n, vgacols[n]);
	}
	for (n = 16; n < 256; n++) {
		n1 = n % 16;
		cols[n] = n1 | (n1 << 8);
	}
}

void reset_colours(int num_cols)
{
	int n;

	if (num_cols > num_colours) {
		num_colours = num_cols;
		if (cols != NULL)
			free(cols);
		cols = malloc(num_colours * sizeof(int));
		if (cols == NULL)
			stop("No memory for colour table");
	}
	for (n = 0; n < num_colours; n++) {
		cols[n] = 0;
	}
	switch (screen_type) {
		case VGA_640x480_16: setup_colours_16(); break;
		default: setup_colours_256(); break;
	}
}

extern void setup_trig(void);

void setup_graphics(int type)
{
	setup_trig();
	start_screen_mode = get_mode();
    setup_screen(type);
	reset_colours(256);
	clipping(0, screen_width - 1, 0, screen_height - 1);
}

void reset_graphics(int type)
{
    reset_screen(type);
	reset_colours(256);
	clipping(0, screen_width - 1, 0, screen_height - 1);
}

void tidyup_graphics(void)
{
    tidyup_screen();
	set_mode(start_screen_mode);
}

