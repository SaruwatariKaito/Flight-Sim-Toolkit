/* Bitmap and sprite functions */

#include <stdlib.h>

#include "bitmap.h"

extern void *alloc_bytes(unsigned int nbytes);

sBitmap *alloc_bitmap(unsigned width, unsigned height)
{
	return (sBitmap *)alloc_bytes(width * height);
}

void free_bitmap(sBitmap *b)
{
	free((void *)b);
}

void read_bitmap(sBitmap *b, int span, int sx, int sy, int dx, int dy, int width, int height)
{
	uchar *p;
	int x;

	if (sx < clip_region.left) {
		if ((sx + width) < clip_region.left)
			return;
		dx += clip_region.left - sx;
		width -= clip_region.left - sx;
		sx = clip_region.left;
	}
	if (sx > clip_region.right)
		return;
	if ((sx + width) > clip_region.right) {
		width -= (sx + width) - (clip_region.right + 1);
	}
	if (sy < clip_region.top) {
		if ((sy + height) < clip_region.top)
			return;
		dy += clip_region.top - sy;
		height -= clip_region.top - sy;
		sy = clip_region.top;
	}
	if (sy > clip_region.bottom)
		return;
	if ((sy + height) > clip_region.bottom) {
		height -= (sy + height) - (clip_region.bottom + 1);
	}
	if ((width > 0) && (height > 0)) {
		p = b + (dy * span) + dx;
		for (height += sy; sy < height; sy++, p += span) {
			getbitmap_prim(sx, sy, width, p);
		}
	}
}

void write_bitmap(sBitmap *b, int span, int sx, int sy, int dx, int dy, int width, int height)
{
	uchar *p;

	if (dx < clip_region.left) {
		if ((dx + width) < clip_region.left)
			return;
		sx += clip_region.left - dx;
		width -= clip_region.left - dx;
		dx = clip_region.left;
	}
	if (dx > clip_region.right)
		return;
	if ((dx + width) > clip_region.right) {
		width -= (dx + width) - (clip_region.right + 1);
	}
	if (dy < clip_region.top) {
		if ((dy + height) < clip_region.top)
			return;
		sy += clip_region.top - dy;
		height -= clip_region.top - dy;
		dy = clip_region.top;
	}
	if (dy > clip_region.bottom)
		return;
	if ((dy + height - 1) > clip_region.bottom) {
		height -= (dy + height) - (clip_region.bottom + 1);
	}
	if ((width > 0) && (height > 0)) {
		p = b + (sy * span) + sx;
		for (height += dy; dy < height; dy++, p += span) {
			bitmap_prim(dx, dy, width, p);
		}
	}
}

void copy_bitmap_lines(sBitmap *source, sBitmap *dest, int span, int sy, int dy, int height)
{
	uchar *ps, *pd;
	unsigned i, size;

	if (dy < 0) {
		sy -= dy;
		height += dy;
		dy = 0;
	}
	if ((dy + height) >= screen_height) {
		height -= (dy + height) - screen_height;
	}
	if ((sy >= 0) && (height > 0)) {
		ps = source + (sy * span);
		pd = dest + (dy * span);
		size = (unsigned)span * (unsigned)height;
/*		for (i = 0; i < size; i++)
			*pd++ = *ps++;*/
		memcpy(pd, ps, size);
	}
}

void add_bitmap(sBitmap *b, int span, int sx, int sy, int dx, int dy, int width, int height)
{
	uchar *p, *p1;
	int x, pix;

	if (dx < 0) {
		sx -= dx;
		width += dx;
		dx = 0;
	}
	if ((dx + width) >= screen_width) {
		width -= (dx + width) - screen_width;
	}
	if (dy < 0) {
		sy -= dy;
		height += dy;
		dy = 0;
	}
	if ((dy + height) >= screen_height) {
		height -= (dy + height) - screen_height;
	}
	if ((width > 0) && (height > 0)) {
		p1 = b + (sy * span) + sx;
		for (height += dy, width += dx; dy < height; dy++, p1 += span) {
			for (p = p1, x = dx; x < width; x++) {
				pix = *p++;
				if (pix != 0)
					pixel_prim(x, dy, pix);
			}
		}
	}
}

int get_bitmap_pixel(sBitmap *b, int span, int x, int y)
{
	sBitmap *p;

	p = b + (y * span) + x;
	return *p;
}

void put_bitmap_pixel(sBitmap *b, int span, int x, int y, int col)
{
	sBitmap *p;

	p = b + (y * span) + x;
	*p = col;
}

void put_bitmap_hline(sBitmap *b, int span, int x1, int x2, int y, int col)
{
	sBitmap *p;
	int x;

	if ((y < 0) || (y >= screen_height))
		return;
	if (x1 > x2) {
		x = x2;
		x2 = x1;
		x1 = x;
	}
	if ((x2 < 0) || (x1 >= span))
		return;
	if (x1 < 0)
		x1 = 0;
	if (x2 >= span)
		x2 = span - 1;
	p = b + (y * span) + x1;
	for (x = x1; x <= x2; x++)
		*p++ = col;
}

static sBitmap *temp_bitmap = NULL;
static int temp_span = 0;

static void temp_hline_prim(int x1, int x2, int y, int col)
{
	put_bitmap_hline(temp_bitmap, temp_span, x1, x2, y, col);
}

void put_bitmap_circle(sBitmap *b, int span, int x, int y, int rad, int col)
{
	void (*old_hline_prim)(int x1, int x2, int y, int colour);

	temp_bitmap = b;
	temp_span = span;
	old_hline_prim = hline_prim;
	hline_prim = temp_hline_prim;
	circle_f(x, y, rad, col);
	hline_prim = old_hline_prim;
}

/* Sprite functions */

static uchar spare_sprite_line[MAX_SCREEN_X];

static Sprite_line *create_sprite_line(sBitmap *p, int width, int trans_col)
{
	int x1, x2;
	sBitmap *start;
	Sprite_line *spl1, *spl;

	spl1 = NULL;
	for (x1 = 0; x1 < width; x1 = x2) {
		while ((x1 < width) && (*p == trans_col)) {
			x1++;
			p++;
		}
		start = p;
		x2 = x1;
		while ((x2 < width) && (*p != trans_col)) {
			x2++;
			p++;
		}
		if (x1 < width) {
			if (spl1 == NULL) {
				spl1 = spl = alloc_bytes(sizeof(Sprite_line));
				if (spl == NULL)
					return NULL;
			} else {
				spl->next = alloc_bytes(sizeof(Sprite_line));
				if (spl->next == NULL)
					return NULL;
				spl = spl->next;
			}
			spl->next = NULL;
			spl->x = x1;
			spl->w = x2 - x1;
			spl->pixels = alloc_bytes(spl->w);
			if (spl->pixels == NULL)
				spl->pixels = spare_sprite_line;
			memcpy(spl->pixels, start, spl->w);
		}
	}
	return spl1;
}

static Sprite spare_sprite;

Sprite *create_sprite(sBitmap *b, int span, int sx, int sy, int width, int height, int trans_col)
{
	int y;
	Sprite *spr;
	Sprite_line **l;

	spr = alloc_bytes(sizeof(Sprite));
	if (spr == NULL) {
		spr = &spare_sprite;
		spr->height = 0;
		spr->lines = NULL;
		return spr;
	}
	spr->height = height;
	l = alloc_bytes(height * sizeof(Sprite_line *));
	if (l == NULL) {
		spr = &spare_sprite;
		spr->height = 0;
		spr->lines = NULL;
		return spr;
	}
	spr->lines = l;
	b += ((int32)sy * (int32)span) + (int32)sx;
	for (y = 0; y < height; y++, l++, b += span) {
		*l = create_sprite_line(b, width, trans_col);
	}
	return spr;
}

void destroy_sprite(Sprite *spr)
{
}

void draw_sprite(Sprite *spr, int x, int y, int height)
{
	Sprite_line *spl, **l;
	sBitmap *p;
	register int x1, max_y;

	if (spr != NULL) {
		if ((height == 0) || (height > spr->height))
			height = spr->height;
		l = spr->lines;
		for (max_y = y + height; y < max_y; y++) {
			for (spl = *l++; spl != NULL; spl = spl->next) {
				x1 = x + spl->x;
				if (spl->w == 1)
					pixel_prim(x1, y, *spl->pixels);
				else
        			bitmap_prim(x1, y, spl->w, spl->pixels);
			}
		}
	}
}

