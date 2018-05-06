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

#define MAX_PTS 16
#define MAX_EDGES 8
#define MAX_SHAPE_PTS 1024

#define COL(N) cols[N]

typedef struct {
	uchar red, green, blue, average;
} sColour;

typedef struct {
	int left, right, top, bottom;
} Screen_clip;

typedef struct {
	int x1, y1, x2, y2;
} sBox;

typedef struct {
	int front, back, left, right, top, bottom;
} Collide_box;

typedef struct {
	int width, height;
	unsigned int *data;
} sCharacter;

/* This structure is coerced to a sVector so must match */
typedef struct {
	int x, y, z, sx, sy, flag;
} Cache_pt;

typedef struct {
	int x, y, z;
	sVector norm;
	int colour, shade;
} Vertex;

typedef struct {
    int base_colour, colour;
	int npoints;
	Pindex *points;
	sVector norm;
} sPolygon;

typedef struct {
    int npts;
    Vertex *pts;
    int npolygons;
    sPolygon **polygons;
	sVector min_size, max_size;
    int scale, radius;
} sShape;

typedef struct {
	int width, height;
	uchar *map;
	sVector2 pts[4];
} Texture;

typedef struct {
    int a, b, c, d, e, f, g, h, i;
	int32 j, k, l;
} Transdata;

/* main.c */
extern void stop(char *message, ...);

/* trig.c */
extern int sctab[], tantab[], acostab[];
extern int fast_atan(int32 x, int32 y);
extern int fast_tan(int a);

/* trans.c */
extern int pshift;
extern int zoom_shift;
extern Transdata *matrix;
extern int32 magnitude(sVector *v);
extern int magnitude2(sVector2 *v);
extern void normalise(sVector *v);
extern void poly_normal(Vertex *p1, Vertex *p2, Vertex *p3, sVector *v);
extern int vector_azimuth(sVector *v);
extern int vector_elevation(sVector *v);
extern void reset_matrix(void);
extern void zero_matrix(void);
extern void push_matrix(void);
extern void pop_matrix(void);
extern void scale_matrix(int shift);
extern void zoom_view(int factor);
extern void ac_axis_rotate(sVector *r, sVector *dr);
extern void rotx(int angle);
extern void roty(int angle);
extern void rotz(int angle);
extern void translate(int32 x, int32 y, int32 z);
extern void s_translate(int x, int y, int z);
extern void s_translate_abs(int x, int y, int z);
extern int32 perspect(sVector *in, sVector2 *out);
extern void rotate(sVector *in, sVector *out);
extern void invrotate(sVector *in, sVector *out);
extern void vertex_rotate(Vertex *v1, Vertex *v2);
extern void transform(sVector *in, sVector *out);
extern void invtransform(sVector *in, sVector *out);
extern void trans_point(sVector *v1, sVector *v2);
extern void setup_trans(sVector *angles, Transdata *t);
extern void rot3d(sVector *in, sVector *out, Transdata *t);
extern void invrot3d(sVector *in, sVector *out, Transdata *t);

/* graph3.c */
extern int xcentre, ycentre, clockwise_test;
extern int sun_size;
extern sVector sun_position;
extern void draw_sun(int colour);
extern void set_sun_angle(int rx, int ry);
extern void setup_graphics3(void);
extern void point3(sVector *pos, int colour);
extern void block3(sVector *pos, int colour);
extern void sphere3(sVector *pos, int radius, int colour);
extern void smoke3(sVector *pos, int radius, int colour, uchar *map);
extern void line3(sVector *pos1, sVector *pos2, int colour);
extern void polygon3(int npts, sVector pts[], int col);
extern void far_shape3(sShape *s, int shade);
extern void shape3(sShape *s, int shade);
extern void near_shape3(sShape *s, int shade);

/* collide.c */
extern int collide3(sShape *s, Collide_box *b);

/* graph2.c */
extern sVector zero;
extern Screen_clip clip_region;
extern void clipping(int left, int right, int top, int bottom);
extern void rot2d(sVector2 *in, sVector2 *out, int angle);
extern void remap_screen(int x1, int y1, int x2, int y2, uchar *map);
extern void vector_digit(int val, int x, int y, int colour);
extern void vector_number(int val, int x, int y, int colour);
extern void character(int x, int y, sCharacter *s, int colour);
extern void rectangle(int x1, int y1, int x2, int y2, int colour);
extern void ellipse(int cx, int cy, int radius, int x_squash, int y_squash, int colour);
extern void circle(int cx, int cy, int radius, int colour);
extern void circle_f(int cx, int cy, int radius, int colour);
extern void circle_r(int cx, int cy, int radius, uchar *map);
extern void polygon(int npts, sVector2 pts[], int colour);
extern void line(int x1, int y1, int x2, int y2, int colour);
extern int clip_xedge(sVector2 *v1, sVector2 *v2);
extern int clip_yedge(sVector2 *v1, sVector2 *v2);

/* tex.c */
extern void raw_tex_poly(int npts, sVector2 pts[], sVector2 ti[], Texture *tex);
extern void tex_poly_prim(int npts, sVector2 pts[], int tex_num);
extern void tex_polygon(int npts, sVector2 pts[], int tex_num);
extern void clipped_tex_polygon(sPolygon *poly, Cache_pt *tpts);
extern int add_texture(Texture *t);

/* graphpc.c */
extern int draw_screen, view_screen;
extern int screen_width, screen_height, screen_colours;
extern int screen_type, screen_mode;
extern int double_buffered;
extern int overide_mode;
extern unsigned int *cols;

extern void reset_colours(int num_colours);
extern void reset_graphics(int type);
extern void setup_graphics(int type);
extern void tidyup_graphics(void);
extern void set_mode(int mode);
extern int get_mode(void);
extern void load_palette(sColour *pal, int first, int last);

extern void swap_screen(void);
extern void merge_screen(void);
extern void split_screen(void);

extern void (*use_screen)(void);
extern void (*use_buffer)(void);
extern void (*update_screen)(int y1, int y2);
extern void (*vsync)(void);
extern void (*swap_hardware)(int draw_screen, int view_screen);

extern void (*palette_prim)(int n, int r, int g, int b);
extern void (*clip_prim)(int left, int right, int top, int bottom);
extern void (*clear_prim)(int y1, int y2, int col);
extern void (*pixel_prim)(int x, int y, int col);
extern int (*getpixel_prim)(int x, int y);
extern void (*block_prim)(int x, int y, int colour);
extern void (*hline_prim)(int x1, int x2, int y, int colour);
extern void (*vline_prim)(int y1, int y2, int x, int colour);
extern void (*line_prim)(int x1, int y1, int x2, int y2, int col);
extern void (*poly_prim)(int npts, sVector2 pts[], int colour);
extern void (*bitmap_prim)(int x, int y, int w, sBitmap *pixels);
extern void (*getbitmap_prim)(int x, int y, int w, sBitmap *pixels);
extern void (*char_prim)(int x, int y, int width, int pattern, int colour);
extern void (*remap_prim)(int x, int y, int w, uchar *map);
extern void (*raw_hline_prim)(int x1, int x2, int y, int colour);
extern void (*tex_hline_prim)(int x1, int x2, int y, int32 btx1, int32 btx2, int32 bty1, int32 bty2, int tw, uchar *tex);
extern void (*tex_line_prim)(int x1, int y1, int x2, int y2, uchar *pixels, int length);

#endif
