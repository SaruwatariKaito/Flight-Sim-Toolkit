/* Bitmap and sprite functions */

#ifndef bitmap_h
#define bitmap_h

#include "graph.h"

typedef struct Sprite_line {
	int x, w;
	sBitmap *pixels;
	struct Sprite_line *next;
} Sprite_line;

typedef struct {
	int height;
	Sprite_line **lines;
} Sprite;

extern sBitmap *alloc_bitmap(unsigned width, unsigned height);
extern void free_bitmap(sBitmap *b);
extern void read_bitmap(sBitmap *dest, int span, int sx, int sy, int dx, int dy, int width, int height);
extern void write_bitmap(sBitmap *source, int span, int sx, int sy, int dx, int dy, int width, int height);
extern void copy_bitmap_lines(sBitmap *source, sBitmap *dest, int span, int sy, int dy, int height);
extern void add_bitmap(sBitmap *source, int span, int sx, int sy, int dx, int dy, int width, int height);
extern int get_bitmap_pixel(sBitmap *source, int span, int x, int y);
extern void put_bitmap_pixel(sBitmap *b, int span, int x, int y, int col);
extern void put_bitmap_hline(sBitmap *b, int span, int x1, int x2, int y, int col);
extern void put_bitmap_circle(sBitmap *b, int span, int x, int y, int rad, int col);

extern Sprite *create_sprite(sBitmap *b, int span, int sx, int sy, int width, int height, int trans_col);
extern void destroy_sprite(Sprite *spr);
extern void draw_sprite(Sprite *spr, int x, int y, int height);

#endif
