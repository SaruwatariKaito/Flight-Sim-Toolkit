/* Definitions for graphics library */

#ifndef graph_h
#define graph_h

#ifndef gdefs_h
#include "gdefs.h"
#endif

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#ifndef ABS
#define ABS(X) (((X) < 0) ? (-(X)) : (X))
#endif
#ifndef SIGN
#define SIGN(X) (((X) < 0) ? -1 : 1)
#endif
#ifndef MAX
#define MAX(X,Y) (((X) > (Y)) ? (X) : (Y))
#endif
#ifndef MIN
#define MIN(X,Y) (((X) < (Y)) ? (X) : (Y))
#endif
#ifndef BETWEEN
#define BETWEEN(X,TOP,BOTTOM) (((X) < (TOP)) && ((X) > (BOTTOM)))
#endif
#ifndef SR
#define SR(V,N) (((V) >= 0) ? ((V) >> (N)) : -(-(V) >> (N)))
#endif

#define SIZE(X,Y,Z) MAX(MAX((X), (Y)), (Z))

#define MAX_PTS 16
#define MAX_EDGES 8
#define MAX_SHAPE_PTS 1024

#define COL(N) cols[N]

typedef struct {
	uchar red, green, blue, average;
} Colour;

typedef struct {
	int left, right, top, bottom;
} Screen_clip;

typedef struct {
	int x1, y1, x2, y2;
} Box;

typedef struct {
	int front, back, left, right, top, bottom;
} Collide_box;

typedef struct {
	int width, height;
	unsigned int *data;
} Character;

typedef struct {
    int base_colour, colour;
	int npoints;
	Pindex *points;
} Poly;

typedef struct {
    int npts;
    Scoord *pts;
    int npolygons;
    Poly **polygons;
	Svector min_size, max_size;
    int scale, radius;
} Shape;

typedef struct {
    int a, b, c, d, e, f, g, h, i;
	int32 j, k, l;
} Transdata;

/* main.c */
extern void stop(char *message, ...);

/* trig.c */
extern int sctab[], tantab[];
extern int fast_atan(long x, long y);
extern int fast_tan(int a);

/* trans.c */
extern int pshift;
extern Transdata *matrix;
extern void reset_matrix(void);
extern void zero_matrix(void);
extern void push_matrix(void);
extern void pop_matrix(void);
extern void scale_matrix(int shift);
extern void zoom_view(int factor);
extern void rotx(int angle);
extern void roty(int angle);
extern void rotz(int angle);
extern void translate(int32 x, int32 y, int32 z);
extern void poly_normal(Scoord *p1, Scoord *p2, Scoord *p3, Svector *v);
extern int32 perspect(Vector *in, Vector2 *out);
extern int fast_perspect(Svector *in, Vector2 *out);
extern void setup_trans(Svector *angles, Transdata *t);
extern void invrot3d(Vector *v1, Vector *v2, Transdata *t);
extern void coord_rotate(Scoord *v1, Scoord *v2);
extern void s_rot3d(Svector *in, Svector *out, Transdata *t);

/* graph3.c */
extern int xcentre, ycentre, clockwise_test;
extern Svector sun_position;
extern Vector zero;
extern Svector szero;
extern int cols[];
extern void set_sun_angle(int rx, int ry);
extern void setup_graphics2(void);
extern void setup_graphics3(void);
extern void far_shape3(Shape *s);

/* shape.c */
extern int screen_width, screen_height;
extern Screen_clip clip_region;
extern void raw_polygon(int npts, Vector2 pts[], int colour);
extern void draw_polygon(int npts, Vector2 pts[], int colour);
extern void draw_line(int x1, int y1, int x2, int y2, int colour);

#endif
