/* Graphics test */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <dos.h>

#include "graph.h"

volatile long total_time = 0;
static long old_tick_count = 0;

static int screen_cleared = FALSE;
static long test_var = 600;
static int time1 = 0, time2 = 0;
static int iters = 500;
static int counter = 0;

extern void load_texture(int n, Texture *tex)
{
}

#define BLACK 0
#define WHITE 0xff
#define RED 0x17
#define GREEN 0x63
#define BROWN 0x30
#define ORANGE 0x57
#define YELLOW 0x77
#define BLUE 0x8b

#define GREY0 0
#define GREY1 1
#define GREY2 2
#define GREY3 3
#define GREY4 0x2c
#define GREY5 0x2d
#define GREY6 0x2e
#define GREY7 0x2f
#define GREY8 0xd0
#define GREY9 0xd1
#define GREY10 0xd2
#define GREY11 0xd3
#define GREY12 0xfc
#define GREY13 0xfd
#define GREY14 0xfe
#define GREY15 0xff

#define C_MIG1 GREY3
#define C_MIG2 GREY6
#define C_MIG3 GREY7
#define C_MIG4 GREY9
#define C_MIG5 GREY9
#define C_MIG6 GREY11
#define C_MIG7 GREY12
#define C_MIG8 RED

static Vertex migpts[69];
static sVector migp[69] = {
    {35,0,-100},{35,0,45},{10,0,150},{-10,0,150},{-35,0,45},{-35,0,-100},
/* 6 */
    {0,0,-75},{10,0,75},{10,20,75},{10,17,110},{10,15,150},{0,0,200},
/* 12 */
    {0,18,150},{0,32,110},{0,30,75},
/* 15 */
    {-10,0,75},{-10,20,75},{-10,17,110},{-10,15,150},
/* 19 */
    {0,-7,150},{0,0,70},
/* 21 */
    {-22,-15,-115},{-10,-15,-115},{-22,-3,-115},{-10,-3,-115},
    {-25,-18,-100},{-7,-18,-100},{-25,0,-100},{-7,0,-100},
    {-21,-23,40},{-7,-23,40},{-20,0,75},{-7,0,75},
/* 33 */
    {22,-15,-115},{10,-15,-115},{22,-3,-115},{10,-3,-115},
    {25,-18,-100},{7,-18,-100},{25,0,-100},{7,0,-100},
    {21,-23,40},{7,-23,40},{20,0,75},{7,0,75},
/* 45 */
    {35,0,-50},{120,-10,-65},{120,-10,-40},
    {-35,0,-50},{-120,-10,-65},{-120,-10,-40},
/* 51 */
    {35,0,-120},{75,0,-140},{80,0,-125},{35,0,-70},
    {-35,0,-120},{-75,0,-140},{-80,0,-125},{-35,0,-70},
/* 59 */
    {35,55,-115},{35,60,-100},{35,0,-30},
    {35,20,-55},{35,0,-10},
/* 64 */
    {-35,55,-115},{-35,60,-100},{-35,0,-30},
    {-35,20,-55},{-35,0,-10},
/* 69 */
};

/* body flat */
static Pindex migpoly1[6] = {0,5,4,3,2,1};
static Pindex migpoly2[6] = {5,0,1,2,3,4};
/* body back top */
static Pindex migpoly3[3] = {8,7,6};
static Pindex migpoly4[3] = {14,8,6};
static Pindex migpoly5[3] = {16,14,6};
static Pindex migpoly6[3] = {15,16,6};
/* body cockpit */
static Pindex migpoly7[4] = {7,8,10,2};
static Pindex migpoly8[4] = {3,18,16,15};
static Pindex migpoly9[4] = {8,16,18,10};
static Pindex migpoly10[2] = {14,13};
static Pindex migpoly11[2] = {13,12};
static Pindex migpoly12[2] = {9,13};
static Pindex migpoly13[4] = {17,13};
/* nose */
static Pindex migpoly14[3] = {2,10,11};
static Pindex migpoly15[3] = {10,12,11};
static Pindex migpoly16[3] = {12,18,11};
static Pindex migpoly17[3] = {18,3,11};
static Pindex migpoly18[3] = {19,2,11};
static Pindex migpoly19[3] = {3,19,11};
static Pindex migpoly20[3] = {2,19,20};
static Pindex migpoly21[3] = {19,3,20};
/* left engine nozzle */
static Pindex migpoly22[4] = {21,22,26,25};
static Pindex migpoly23[4] = {23,21,25,27};
static Pindex migpoly24[4] = {22,24,28,26};
static Pindex migpoly25[4] = {24,23,27,28};
static Pindex migpoly26[4] = {21,23,24,22};
/* right engine nozzle */
static Pindex migpoly27[4] = {34,33,37,38};
static Pindex migpoly28[4] = {36,34,38,40};
static Pindex migpoly29[4] = {33,35,39,37};
static Pindex migpoly30[4] = {35,36,40,39};
static Pindex migpoly31[4] = {34,36,35,33};
/* left engine */
static Pindex migpoly32[4] = {25,26,30,29};
static Pindex migpoly33[4] = {27,25,29,31};
static Pindex migpoly34[4] = {26,28,32,30};
static Pindex migpoly35[4] = {29,30,32,31};
/* right engine */
static Pindex migpoly36[4] = {38,37,41,42};
static Pindex migpoly37[4] = {40,38,42,44};
static Pindex migpoly38[4] = {37,39,43,41};
static Pindex migpoly39[4] = {42,41,43,44};
/* right wing */
static Pindex migpoly40[4] = {46,45,1,47};
static Pindex migpoly41[4] = {1,45,46,47};
/* left wing */
static Pindex migpoly42[4] = {4,48,49,50};
static Pindex migpoly43[4] = {49,48,4,50};
/* right tailplane */
static Pindex migpoly44[4] = {52,51,54,53};
static Pindex migpoly45[4] = {54,51,52,53};
/* left tailplane */
static Pindex migpoly46[4] = {58,55,56,57};
static Pindex migpoly47[4] = {56,55,58,57};
/* right fin */
static Pindex migpoly48[4] = {61,0,59,60};
static Pindex migpoly49[3] = {63,61,62};
static Pindex migpoly50[4] = {59,0,61,60};
static Pindex migpoly51[3] = {62,61,63};
/* left fin */
static Pindex migpoly52[4] = {66,5,64,65};
static Pindex migpoly53[3] = {68,66,67};
static Pindex migpoly54[4] = {64,5,66,65};
static Pindex migpoly55[3] = {67,66,68};
/* cockpit back, front */
static Pindex migpoly56[3] = {8,14,16};
static Pindex migpoly57[3] = {18,12,10};


/* seen from above */
static sPolygon migpolysa[57] = {
/* body bottom */
    {C_MIG7,C_MIG2,6,migpoly2},
    {C_MIG7,C_MIG3,3,migpoly20},{C_MIG7,C_MIG3,3,migpoly21},
/* wing bottoms */
    {C_MIG7,C_MIG2,4,migpoly41},{C_MIG7,C_MIG2,4,migpoly43},
/* tailplane bottoms */
    {C_MIG7,C_MIG2,4,migpoly45},{C_MIG7,C_MIG2,4,migpoly47},
/* engines right sides */
    {C_MIG7,C_MIG1,4,migpoly24},{C_MIG7,C_MIG4,4,migpoly34},
    {C_MIG7,C_MIG1,4,migpoly29},{C_MIG7,C_MIG4,4,migpoly38},
/* engines left sides */
    {C_MIG7,C_MIG1,4,migpoly28},{C_MIG7,C_MIG4,4,migpoly37},
    {C_MIG7,C_MIG1,4,migpoly23},{C_MIG7,C_MIG4,4,migpoly33},
/* engines tops */
    {C_MIG7,C_MIG1,4,migpoly30},{C_MIG7,C_MIG1,4,migpoly25},
/* engines bottoms */
    {C_MIG7,C_MIG1,4,migpoly22},{C_MIG7,C_MIG1,4,migpoly27},
    {C_MIG7,C_MIG3,4,migpoly32},{C_MIG7,C_MIG3,4,migpoly36},
    {C_MIG7,C_MIG1,4,migpoly35},{C_MIG7,C_MIG1,4,migpoly39},
/* engine backs */
    {C_MIG7,C_MIG8,4,migpoly26},{C_MIG7,C_MIG8,4,migpoly31},
/* body flat top */
    {C_MIG7,C_MIG6,6,migpoly1},
/* wing tops */
    {C_MIG7,C_MIG7,4,migpoly40},{C_MIG7,C_MIG7,4,migpoly42},
/* tailplane tops */
    {C_MIG7,C_MIG7,4,migpoly44},{C_MIG7,C_MIG7,4,migpoly46},
/* fin insides */
    {C_MIG7,C_MIG5,4,migpoly50},{C_MIG7,C_MIG5,3,migpoly51},
    {C_MIG7,C_MIG5,4,migpoly52},{C_MIG7,C_MIG5,3,migpoly53},
/* cockpit */
    {C_MIG7,C_MIG1,4,migpoly9},{C_MIG7,C_MIG1,3,migpoly56},{C_MIG7,C_MIG1,3,migpoly57},
    {C_MIG7,C_MIG4,2,migpoly10},{C_MIG7,C_MIG4,2,migpoly11},
    {C_MIG7,C_MIG4,2,migpoly12},{C_MIG7,C_MIG4,2,migpoly13},
    {C_MIG7,C_MIG3,4,migpoly7},{C_MIG7,C_MIG3,4,migpoly8},
/* back body sides */
    {C_MIG7,C_MIG3,3,migpoly3},{C_MIG7,C_MIG3,3,migpoly6},
    {C_MIG7,C_MIG4,3,migpoly4},{C_MIG7,C_MIG4,3,migpoly5},
/* nose */
    {C_MIG7,C_MIG2,3,migpoly14},{C_MIG7,C_MIG2,3,migpoly17},
    {C_MIG7,C_MIG4,3,migpoly15},{C_MIG7,C_MIG4,3,migpoly16},
    {C_MIG7,C_MIG2,3,migpoly18},{C_MIG7,C_MIG2,3,migpoly19},
/* fin outsides */
    {C_MIG7,C_MIG5,4,migpoly48},{C_MIG7,C_MIG5,3,migpoly49},
    {C_MIG7,C_MIG5,4,migpoly54},{C_MIG7,C_MIG5,3,migpoly55},
};
sShape mig29_s1a = {69,migpts, 57,NULL,{0,0,0},{0,0,0},8,0};

/* seen from below */
static sPolygon migpolysb[57] = {
/* body flat top */
    {C_MIG7,C_MIG6,6,migpoly1},
/* tailplane tops */
    {C_MIG7,C_MIG7,4,migpoly44},{C_MIG7,C_MIG7,4,migpoly46},
/* cockpit */
    {C_MIG7,C_MIG1,4,migpoly9},{C_MIG7,C_MIG1,3,migpoly56},{C_MIG7,C_MIG1,3,migpoly57},
    {C_MIG7,C_MIG4,2,migpoly10},{C_MIG7,C_MIG4,2,migpoly11},
    {C_MIG7,C_MIG4,2,migpoly12},{C_MIG7,C_MIG4,2,migpoly13},
    {C_MIG7,C_MIG3,4,migpoly7},{C_MIG7,C_MIG3,4,migpoly8},
/* back body sides */
    {C_MIG7,C_MIG3,3,migpoly3},{C_MIG7,C_MIG3,3,migpoly6},
    {C_MIG7,C_MIG4,3,migpoly4},{C_MIG7,C_MIG4,3,migpoly5},
/* nose */
    {C_MIG7,C_MIG2,3,migpoly14},{C_MIG7,C_MIG2,3,migpoly17},
    {C_MIG7,C_MIG4,3,migpoly15},{C_MIG7,C_MIG4,3,migpoly16},
    {C_MIG7,C_MIG2,3,migpoly18},{C_MIG7,C_MIG2,3,migpoly19},
/* fin insides */
    {C_MIG7,C_MIG5,4,migpoly50},{C_MIG7,C_MIG5,3,migpoly51},
    {C_MIG7,C_MIG5,4,migpoly52},{C_MIG7,C_MIG5,3,migpoly53},
/* fin outsides */
    {C_MIG7,C_MIG5,4,migpoly48},{C_MIG7,C_MIG5,3,migpoly49},
    {C_MIG7,C_MIG5,4,migpoly54},{C_MIG7,C_MIG5,3,migpoly55},
/* body bottom */
    {C_MIG7,C_MIG2,6,migpoly2},
    {C_MIG7,C_MIG3,3,migpoly20},{C_MIG7,C_MIG3,3,migpoly21},
/* wing bottoms */
    {C_MIG7,C_MIG2,4,migpoly41},{C_MIG7,C_MIG2,4,migpoly43},
/* tailplane bottoms */
    {C_MIG7,C_MIG2,4,migpoly45},{C_MIG7,C_MIG2,4,migpoly47},
/* engines right sides */
    {C_MIG7,C_MIG1,4,migpoly24},{C_MIG7,C_MIG4,4,migpoly34},
    {C_MIG7,C_MIG1,4,migpoly29},{C_MIG7,C_MIG4,4,migpoly38},
/* engines left sides */
    {C_MIG7,C_MIG1,4,migpoly28},{C_MIG7,C_MIG4,4,migpoly37},
    {C_MIG7,C_MIG1,4,migpoly23},{C_MIG7,C_MIG4,4,migpoly33},
/* wing tops */
    {C_MIG7,C_MIG7,4,migpoly40},{C_MIG7,C_MIG7,4,migpoly42},
/* engines tops */
    {C_MIG7,C_MIG1,4,migpoly30},{C_MIG7,C_MIG1,4,migpoly25},
/* engines bottoms */
    {C_MIG7,C_MIG1,4,migpoly22},{C_MIG7,C_MIG1,4,migpoly27},
    {C_MIG7,C_MIG3,4,migpoly32},{C_MIG7,C_MIG3,4,migpoly36},
    {C_MIG7,C_MIG1,4,migpoly35},{C_MIG7,C_MIG1,4,migpoly39},
/* engine backs */
    {C_MIG7,C_MIG8,4,migpoly26},{C_MIG7,C_MIG8,4,migpoly31},
};
sShape mig29_s1b = {69,migpts, 57,NULL,{0,0,0},{0,0,0},8,0};

static void setup_polygons(sShape *s, sPolygon *polys)
{
    int i, pt1, pt2, pt3;
	sPolygon *poly;

	s->polygons = (sPolygon **)malloc(s->npolygons * sizeof(sPolygon *));
	for (i = 0; i < s->npolygons; i++) {
		poly = &polys[i];
		if (poly->npoints > 2) {
        	pt1 = poly->points[0];
        	pt2 = poly->points[1];
        	pt3 = poly->points[2];
        	poly_normal(&s->pts[pt2], &s->pts[pt1], &s->pts[pt3], &poly->norm);
		} else {
			poly->norm = zero;
		}
		s->polygons[i] = poly;
	}
}

static void setup_shapes(void)
{
	int i;

	for (i = 0; i < 69; i++) {
		migpts[i].x = migp[i].x;
		migpts[i].y = migp[i].y;
		migpts[i].z = migp[i].z;
		migpts[i].norm = zero;
		migpts[i].colour = 0;
		migpts[i].shade = 0;
	}
	setup_polygons(&mig29_s1a, migpolysa);
	setup_polygons(&mig29_s1b, migpolysb);
}

void draw_mig29(void)
{
    sVector v;

    v.x = v.y = 0;
    v.z = 256;
    invrotate(&v, &v);
    if (v.y >= 0) {
        if (test_var < 500)
            shape3(&mig29_s1b, 0);
        else
            /*shape3(&mig29_s1b, 0);*/
            far_shape3(&mig29_s1b, 0);
    } else {
        if (test_var < 500)
            shape3(&mig29_s1a, 0);
        else
            /*shape3(&mig29_s1a, 0);*/
            far_shape3(&mig29_s1a, 0);
    }
}

int escape_flag = FALSE;

void stop(char *message, ...)
{
	FILE *fp;

	if (message != NULL) {
		fp = fopen("log", "a");
		if (fp != NULL) {
			fprintf(fp, "%s\n", message);
			fclose(fp);
		}
	}
	tidyup_graphics();
	if (message != NULL) {
    	printf("%s\n", message);
	}
    exit(0);
}

void (_interrupt _far *prev_int_08)(void);

void _interrupt _far clock_tick(void)
{
	static int count = 0;

	total_time++;
	if (count++ >= 4) {
		count = 0;
		_chain_intr(prev_int_08);
	}
	outp(0x20, 0x20);
	_enable();
}

static void setup_timer(void)
{
	union REGS r;

	total_time = 0;
	r.x.eax = 0x0000;
	int386(0x1a, &r, &r);
	old_tick_count = r.x.ecx;
	old_tick_count = (old_tick_count << 16) | r.x.edx;
	prev_int_08 = _dos_getvect(0x08);
	_dos_setvect(0x08, clock_tick);
	outp(0x43, 0x36);
	outp(0x40, 0x14);
	outp(0x40, 0x2e);
}

static void tidyup_timer(void)
{
	union REGS r;

	outp(0x43, 0x36);
	outp(0x40, 0xff);
	outp(0x40, 0xff);
	_dos_setvect(0x08, prev_int_08);
	old_tick_count += (total_time * 18) / 100;
	r.x.eax = 0x0100;
	r.x.ecx = (int)(old_tick_count >> 16);
	r.x.edx = (int)old_tick_count;
	int386(0x1a, &r, &r);
}

static sVector2 s1[4] = {{20, 20}, {79, 24}, {71, 40}, {13, 51}};
static sVector2 s1a[4] = {{120, 20}, {179, 24}, {171, 40}, {113, 51}};
static sVector2 s1b[4] = {{20, 120}, {79, 124}, {71, 140}, {13, 151}};
static sVector2 s1c[4] = {{120, 120}, {179, 124}, {171, 140}, {113, 151}};
static sVector2 s2[4] = {{17, 10}, {79, 14}, {79, 24}, {20, 20}};
static sVector2 s3[4] = {{13, 51}, {71, 40}, {77, 47}, {22, 59}};
static sVector2 s4[4] = {{140, 20}, {163, 23}, {167, 40}, {142, 46}};
static uchar bm1[32];
static uchar bm2[8] = {RED,GREEN,RED,GREEN,YELLOW,BLUE,YELLOW,BLUE};
static void gtest(void)
{
	int i;

	for (i = 0; i < 200; i++) {
		if ((i & 3) == 0)
			pixel_prim(i, i, COL(WHITE));
		else
			pixel_prim(i, i, COL(RED));
		pixel_prim(i + 2, i, COL(GREEN));
	}
	for (i = 0; i < 200; i++) {
		pixel_prim(i + 4, i, getpixel_prim(i, i));
	}
	for (i = 0; i < 8; i++) {
		vline_prim(10 - i, 90 + i, 50 + i, COL(RED));
	}
	pixel_prim(10, 49, COL(RED));
	pixel_prim(200, 49, COL(RED));
	for (i = 0; i < 8; i++) {
		hline_prim(11, 200 + i, 50 + i, COL(WHITE));
	}
	for (i = 0; i < 8; i++) {
		hline_prim(10 + i, 200, 60 + i, COL(GREEN));
	}
	for (i = 0; i < 8; i++) {
		hline_prim(10 - i, 200 + i, 70 + i, COL(WHITE));
	}
	for (i = 0; i < 32; i++)
		bm1[i] = i + 8;
	bitmap_prim(90, 15, 32, bm1);
	vector_number(1234, 250, 20, WHITE);
	vector_number(5678, 251, 30, WHITE);
	vector_number(90, 252, 40, WHITE);
	vector_number(900, 253, 50, WHITE);
	line_prim(20, 10, 120, 110, COL(YELLOW));
	line_prim(30, 10, 120, 40, COL(GREEN));
	line_prim(100, 10, 120, 130, COL(GREEN));
	line_prim(11, 191, 201, 120, COL(GREEN));
	line_prim(201, 185, 227, 120, COL(RED));
	line_prim(11, 150, 93, 150, COL(WHITE));
	poly_prim(4, s1, COL(GREEN));
	getbitmap_prim(90, 15, 32, bm1);
	bitmap_prim(100, 17, 32, bm1);
}

void mig_test(void)
{
    int i, c, d, a;

	time1 = total_time;
    for (i = 0, d = 16, a = 0; i < iters; i++, a += d) {
/*		vsync();*/
		if (screen_cleared) {
			swap_screen();
			clear_prim(0, screen_height - 1, COL(7));
		}
/*		vector_number(inpw(0xceee), 40, 10, 255);*/
/*		tab(0,0);
		printf("a = %4d\n", a);*/

		push_matrix();
		translate(0, 0, test_var);
        roty(a << 4);
        rotz(a << 5);
/*		scale_matrix(1);*/

        draw_mig29();
        pop_matrix();

/*		c = getch();
        if (c == '.')
            d = -4;
        if (c == ',')
            d = 4;
		if (c == 27)
			break;*/
	}
    time2 = total_time;
}

static sVector2 p1[4] = {{100, 20}, {120, 20}, {120, 20}, {100, 20}};
static sVector2 p2[4] = {{100, 40}, {120, 40}, {120, 41}, {100, 41}};
static sVector2 p3[4] = {{100, 60}, {120, 60}, {120, 62}, {100, 62}};

static void ptest(void)
{
	poly_prim(4, p1, COL(WHITE));
	poly_prim(4, p2, COL(WHITE));
	poly_prim(4, p3, COL(WHITE));
}

int main(int argc, char **argv)
{
	int i;

    test_var = 500;
    iters = 100;
    if (argc >= 2) {
        sscanf(argv[1], "%d", &test_var);
    }
    if (argc >= 3) {
        sscanf(argv[2], "%d", &iters);
    }
    if (argc == 4) {
        sscanf(argv[3], "%d", &screen_cleared);
    }


/*	setup_graphics(VGA_320x200_256);*/
	setup_graphics(VESA_640x480_256);

	setup_graphics3();
    clipping(0, screen_width - 1, 0, screen_height - 1);
	zoom_view(1);
	setup_timer();

	setup_shapes();

	if (!screen_cleared)
		merge_screen();

	for (i = 10; i < 100; i++)
		pixel_prim(i, i, i);
/*	hline_prim(20, 200, 40, 255);
	vline_prim(40, 100, 50, 255);
	line_prim(50, 50, 200, 30, 100);*/
/*	circle(150, 100, 50, 0x1f);
	ellipse(150, 100, 50, 1024, 450, 0x43);
	ellipse(150, 100, 50, 300, 1024, 0x43);*/

	mig_test();
/*	ptest();*/
/*	gtest();*/

	getch();

	tidyup_timer();
	tidyup_graphics();

	printf("centisecs = %d\n", time2 - time1);
	printf("counter = %d\n", counter);

	return(0);
}

