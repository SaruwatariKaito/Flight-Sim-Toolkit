#define GOGOGO
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
static Vertex migpts[4];
static sVector migp[4] = {{-50,-50,-50},{50,-50,-50},{50,50,-50},{-50,50,-50}};

static Pindex migpoly1[4] = {0,1,2,3};
static Pindex migpoly2[4] = {0,3,2,1};

static sPolygon migpolysa[] = {{C_MIG7,C_MIG2,4,migpoly1}};
static sPolygon migpolysb[] = {{C_MIG7,C_MIG2,4,migpoly2}};
sShape mig29_s1a = {4,migpts, 1,NULL,{0,0,0},{0,0,0},8,0};
sShape mig29_s1b = {4,migpts, 1,NULL,{0,0,0},{0,0,0},8,0};
Texture	tex;
int texcol;
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
			poly->colour = texcol + 256;
			//poly->colour = 256;
		} else {
			poly->norm = zero;
		}
		s->polygons[i] = poly;
	}
}

static void setup_shapes(void)
{
	int i;

	// Setup texture
	{
		//typedef struct {
		//	int width, height;
		//	uchar *map;
		// 	sVector2 pts[4];
		//} Texture;
	
		/* Returns the colour number of the texture */
		//extern int add_texture(Texture *t);
		//  migpts[i].colour = texcol + 256;
		
		tex.width = 256;
		tex.height = 256;
		i = 8;
		tex.pts[0].x = 0, tex.pts[0].y = 0;
		tex.pts[1].x = i, tex.pts[1].y = 0;
		tex.pts[2].x = i, tex.pts[2].y = i;
		tex.pts[3].x = 0, tex.pts[3].y = i;

		if ((tex.map = malloc (tex.width * tex.height)) == NULL)
			stop ("Failed to allocate texture");

		for ( i = 0; i < tex.width * tex.height; i++)
		{
			if (i % 3 == 0)
				tex.map[i] = 253;
			else if (i % 2 == 0)
				tex.map[i] = 254;
			else
				tex.map[i] = 255;
		}
		texcol = add_texture (&tex);
	}

	for (i = 0; i < 4; i++) {
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
#include <stdarg.h>
void stop(char *message, ...)
{
	FILE *fp;

	if (message != NULL) {
		fp = fopen("log", "a");
		if (fp != NULL) {
			//fprintf(fp, "%s\n", message);
			
			va_list	v;
			va_start (v, message);
			fprintf (fp, "%s\n", message);
			fprintf (fp, "%08X\n", *(int *) v [0]);
			fflush (fp);
		  	vfprintf (fp, message, v);
		    fflush (fp);
			fprintf (fp, "printed message\n");
			va_end (v);
			fprintf (fp, "var end \n");
			fclose(fp);
		}
	} 
	tidyup_graphics();
	if (message != NULL) {
    	printf("%s\n", message);
	}
    exit(0);
}

void dontstop(char *message, ...)
{
	FILE *fp;
#ifdef GOGOGO
	return;
#endif

	if (message != NULL) {
		fp = fopen("log", "a");
		if (fp != NULL) {
			//fprintf(fp, "%s\n", message);
			
			va_list	v;
			va_start (v, message);
		  	vfprintf (fp, message, v);
			va_end (v);
			fclose(fp);
		}
	} 
}

void _dontstop(char *message, ...)
{
	FILE *fp;

	if (message != NULL) {
		fp = fopen("log", "a");
		if (fp != NULL) {
			//fprintf(fp, "%s\n", message);
			
			va_list	v;
			va_start (v, message);
		  	vfprintf (fp, message, v);
			va_end (v);
			fclose(fp);
		}
	} 
}



void debug_int (int i)
{
	FILE *fp;
	
		fp = fopen("log", "at");
		if (fp != NULL) {
			fprintf (fp, "%d\n", i);
			fclose(fp);
		}
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
#ifdef GOGOGO
    for (i = 0, d = 16, a = 0; i < 500/*i < iters*/; i++, a += d) {
#else
    for (i = 0, d = 16, a = 0; i < 1/*i < iters*/; i++, a += d) {
#endif
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
		//a = d * 20;
//#ifdef GOGOGO
		rotx (a << 4);
//#endif
        roty (a << 4);
        //rotz (a << 5);	//hmmm
/*		scale_matrix(1);*/

        draw_mig29();
        pop_matrix();

		if (kbhit())
		{
			c = getch();
			if (c == 27)
				break;
		}
		//getch ();

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

	config_fpu_cw ();

	{
		float f = 1.5;
		i = FTOI (f);
		_dontstop ("f = %f, i = %d\n", f, i);
		i = 3;
		f = ITOF (i);
		_dontstop ("f = %f, i = %d", f, i);
	}
	restore_fpu_cw ();


	clockwise_test = 0;

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

 	palette_prim (253, 255, 0, 0);
	palette_prim (254, 0, 255, 0);
	palette_prim (255, 0, 0, 255);

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

	//getch();

	free (tex.map);


	tidyup_timer();
	tidyup_graphics();

	printf("centisecs = %d\n", time2 - time1);
	printf("counter = %d\n", counter);

	return(0);
}