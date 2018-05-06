/* VGA Mode 0x13 graphics primitives */

#include <stdlib.h>

#include "graph.h"

/*extern void raw_hline_13(int x1, int x2, int y, int colour);*/
extern void c_polytri(int npts, sVector2 pts[], int col);
extern void c_polygon(int npts, sVector2 pts[], int col);

extern int32 divtab[];

/* Contains pointers to each line of draw screen */
uchar *linetab[MAX_SCREEN_Y + 1];

uchar *screen_memory = NULL;
uchar *screen_buffer = NULL;

static void *_x386_zero_base_ptr = 0;
static uchar *screen_base = NULL;
static unsigned screen_size = 0;

static void use_screen_13(void)
{
	int i;
	uchar *p;

	p = screen_base;
	for (i = 0; i <= screen_height; i++, p += screen_width) {
		linetab[i] = p;
	}
}

static void use_buffer_13(void)
{
	int i;
	uchar *p;

	p = screen_buffer;
	for (i = 0; i <= screen_height; i++, p += screen_width) {
		linetab[i] = p;
	}
}

static void update_screen_13(int y1, int y2)
{
	int n;
	unsigned *p1, *p2;

	p1 = (unsigned *)linetab[y1];
	n = (int)(linetab[y2 + 1] - (uchar *)p1) >> 2;
	p2 = (unsigned *)(screen_base + (y1 * screen_width));
	while (n--) {
		*p2++ = *p1++;
	}
}

void vsync_13(void)
{
}

static void swap_13(int draw_screen, int view_screen)
{
	if (draw_screen == view_screen) {
		use_screen_13();
	} else {
		use_buffer_13();
		update_screen_13(clip_region.top, clip_region.bottom);
	}
}

void palette_13(int n, int r, int g, int b)
{
	if (n >= 0) {
		outp(0x3c8, n);
	}
	outp(0x3c9, r >> 2);
	outp(0x3c9, g >> 2);
	outp(0x3c9, b >> 2);
}

void clip_13(int left, int right, int top, int bottom)
{
}

void clear_13(int y1, int y2, int col)
{
	int n;
	unsigned *p;

	if (y1 < clip_region.top)
		y1 = clip_region.top;
	if (y2 > clip_region.bottom)
		y2 = clip_region.bottom;
	p = (unsigned *)linetab[y1];
	n = (int)(linetab[y2 + 1] - (uchar *)p) >> 2;
	while (n--) {
		*p++ = col;
	}
}

void pixel_13(int x, int y, int col)
{
	uchar *p;

	if ((y >= clip_region.top) && (y <= clip_region.bottom) &&
		(x >= clip_region.left) && (x <= clip_region.right)) {
		p = linetab[y] + x;
		*p = col;
	}
}

int getpixel_13(int x, int y)
{
	uchar *p;

	if ((y >= clip_region.top) && (y <= clip_region.bottom) &&
		(x >= clip_region.left) && (x <= clip_region.right)) {
		p = linetab[y] + x;
		return *p;
	}
	return 0;
}

void block_13(int x, int y, int colour)
{
	unsigned short *p;

	if ((y >= clip_region.top) && (y < clip_region.bottom) &&
		(x >= clip_region.left) && (x < clip_region.right)) {
		p = (unsigned short *)(linetab[y] + x);
		*p = colour;
		*(p + screen_width) = colour;
	}
}

static void tex_line_13(int x1, int y1, int x2, int y2, uchar *pixels, int length)
{
    int dx, dy, x_step, y_step, gc, x, y, n, col;
	int32 big_t, t_grad;
	uchar *pp;

	if ((dx = x2 - x1) < 0) {
        dx = -dx;
		x_step = -1;
    } else {
		x_step = 1;
    }
	if ((dy = y2 - y1) < 0) {
        dy = -dy;
        y_step = -screen_width;
    } else {
        y_step = screen_width;
    }
	x = x1;
	y = y1;
	big_t = 0;
	pp = linetab[y1] + x1;
    if (dx > dy) {
		t_grad = divtab[dx] * length;
        gc = dx >> 1;
		n = dx;
		while (n--) {
			if ((col = pixels[big_t >> 16]) != 0)
				*pp = col;
			pp += x_step;
			big_t += t_grad;
            gc -= dy;
            if (gc < 0) {
				if (col)
					*pp = col;
				pp += y_step;
                gc += dx;
            }
        }
		if ((col = pixels[big_t >> 16]) != 0)
			*pp = col;
    } else {
		t_grad = divtab[dy] * length;
        gc = dy >> 1;
		n = dy;
		while (n--) {
			if ((col = pixels[big_t >> 16]) != 0)
				*pp = col;
			pp += y_step;
			big_t += t_grad;
            gc -= dx;
            if (gc < 0) {
				if (col)
					*pp = col;
                pp += x_step;
                gc += dy;
            }
        }
		if ((col = pixels[big_t >> 16]) != 0)
			*pp = col;
    }
}

/* Writes direct to frame buffer */
static void tex_hline_13(int x1, int x2, int y, int32 btx1, int32 btx2, int32 bty1, int32 bty2, int tw, uchar *tex)
{
	int x, dx, col;
	int32 tx_grad, ty_grad;
	uchar *pp;

	if ((y < 0) || (y >= screen_height))
		return;
	if (x1 <= x2) {
		if ((x1 < 0) || (x2 >= screen_width))
			return;
		dx = (x2 - x1) + 1;
		tx_grad = (btx2 - btx1) / dx;
		ty_grad = (bty2 - bty1) / dx;
		pp = linetab[y] + x1;
		for (x = x1; x <= x2; x++) {
			btx1 += tx_grad;
			bty1 += ty_grad;
			col = *((uchar *)tex + (tw * (bty1 >> 16)) + (btx1 >> 16));
			if (col != 0)
				*pp++ = col;
			else
				pp++;
		}
	} else {
		if ((x2 < 0) || (x1 >= screen_width))
			return;
		dx = (x1 - x2) + 1;
		tx_grad = (btx1 - btx2) / dx;
		ty_grad = (bty1 - bty2) / dx;
		pp = linetab[y] + x2;
		for (x = x2; x <= x1; x++) {
			btx2 += tx_grad;
			bty2 += ty_grad;
			col = *((uchar *)tex + (tw * (bty2 >> 16)) + (btx2 >> 16));
			if (col != 0)
				*pp++ = col;
			else
				pp++;
		}
	}
}

void hline_13(int x1, int x2, int y, int colour)
{
	int x, i, n;
	uchar *pp;
	unsigned *p;

	if ((y >= clip_region.top) && (y <= clip_region.bottom)) {
		if (x1 <= x2) {
			if ((x2 < clip_region.left) || (x1 > clip_region.right))
				return;
			if (x1 < clip_region.left)
				x1 = clip_region.left;
			if (x2 > clip_region.right)
				x2 = clip_region.right;
			pp = linetab[y] + x1;
			n = x2 - x1 + 1;
			i = (x1 & 3);
			if (i) {
				i = 4 - i;
				n -= i;
				while (i--) {
					*pp++ = colour;
				}
			}
			p = (unsigned *)pp;
			while (n >= 4) {
				*p++ = colour;
				n -= 4;
			}
			if (n > 0) {
				pp = (uchar *)p;
				while (n--) {
					*pp++ = colour;
				}
			}
/*			for (x = x1; x <= x2; x++)
				*pp++ = colour;*/
		} else {
			if ((x1 < clip_region.left) || (x2 > clip_region.right))
				return;
			if (x2 < clip_region.left)
				x2 = clip_region.left;
			if (x1 > clip_region.right)
				x1 = clip_region.right;
			pp = linetab[y] + x2;
			n = x1 - x2 + 1;
			i = (x2 & 3);
			if (i) {
				i = 4 - i;
				n -= i;
				while (i--) {
					*pp++ = colour;
				}
			}
			p = (unsigned *)pp;
			while (n >= 4) {
				*p++ = colour;
				n -= 4;
			}
			if (n > 0) {
				pp = (uchar *)p;
				while (n--) {
					*pp++ = colour;
				}
			}
/*			for (x = x2; x <= x1; x++)
				*pp++ = colour;*/
		}
	}
}

void vline_13(int y1, int y2, int x, int colour)
{
	int y;
	uchar *pp;

	if ((x >= clip_region.left) && (x <= clip_region.right)) {
		if (y1 <= y2) {
			if (y1 < clip_region.top)
				y1 = clip_region.top;
			if (y2 > clip_region.bottom)
				y2 = clip_region.bottom;
			pp = linetab[y1] + x;
			for (y = y1; y <= y2; y++, pp += screen_width)
				*pp = colour;
		} else {
			if (y2 < clip_region.top)
				y2 = clip_region.top;
			if (y1 > clip_region.bottom)
				y1 = clip_region.bottom;
			pp = linetab[y2] + x;
			for (y = y2; y <= y1; y++, pp += screen_width)
				*pp = colour;
		}
	}
}

void bitmap_13(int x, int y, int w, uchar *bitmap)
{
	int d;
	uchar *pp;

	if ((y >= clip_region.top) && (y <= clip_region.bottom)) {
		if (x < clip_region.left) {
			d = clip_region.left - x;
			w -= d;
			if (w <= 0)
				return;
			bitmap += d;
			x = clip_region.left;
		}
		if ((x + w) > clip_region.right) {
			d = (x + w) - clip_region.right;
			w -= d;
			if (w <= 0)
				return;
		}
		pp = linetab[y] + x;
		while (w--) {
			*pp++ = *bitmap++;
		}
	}
}

void getbitmap_13(int x, int y, int w, uchar *bitmap)
{
	int d;
	uchar *pp;

	if ((y >= clip_region.top) && (y <= clip_region.bottom)) {
		if (x < clip_region.left) {
			d = clip_region.left - x;
			w -= d;
			if (w <= 0)
				return;
			bitmap += d;
			x = clip_region.left;
		}
		if ((x + w) > clip_region.right) {
			d = (x + w) - clip_region.right;
			w -= d;
			if (w <= 0)
				return;
		}
		pp = linetab[y] + x;
		while (w--) {
			*bitmap++ = *pp++;
		}
	}
}

void char_13(int x, int y, int width, int pattern, int colour)
{
	int d;
	uchar *pp;

	if ((y >= clip_region.top) && (y <= clip_region.bottom)) {
		if (x < clip_region.left) {
			d = clip_region.left - x;
			width -= d;
			if (width <= 0)
				return;
			while (d--) {
				pattern <<= 1;
			}
			x = clip_region.left;
		}
		if ((x + width) > clip_region.right) {
			d = (x + width) - clip_region.right;
			width -= d;
			if (width <= 0)
				return;
		}
		pp = linetab[y] + x;
		while (width--) {
			if (pattern & 0x8000)
				*pp++ = colour;
			else
				pp++;
			pattern <<= 1;
		}
	}
}

void remap_13(int x, int y, int w, uchar *map)
{
}

void raw_hline_13(int x1, int x2, int y, int colour)
{
	int x, i, n;
	uchar *pp;
	unsigned *p;

	if (x1 <= x2) {
		pp = linetab[y] + x1;
		n = x2 - x1 + 1;
		i = (x1 & 3);
	} else {
		pp = linetab[y] + x2;
		n = x1 - x2 + 1;
		i = (x2 & 3);
	}
	if (i) {
		i = 4 - i;
		n -= i;
		while (i--) {
			*pp++ = colour;
		}
	}
	p = (unsigned *)pp;
	while (n >= 4) {
		*p++ = colour;
		n -= 4;
	}
	if (n > 0) {
		pp = (uchar *)p;
		while (n--) {
			*pp++ = colour;
		}
	}
/*	if (x1 <= x2) {
		pp = linetab[y] + x1;
		for (x = x1; x <= x2; x++)
			*pp++ = colour;
	} else {
		pp = linetab[y] + x2;
		for (x = x2; x <= x1; x++)
			*pp++ = colour;
	}*/
}

static void line_13(int x1, int y1, int x2, int y2, int colour)
{
    int dx, dy, step, gc, i;
	uchar *p;

	dx = x2 - x1;
    dy = y2 - y1;
    if (dy < 0) {
        dy = -dy;
        step = -screen_width;
    } else {
        step = screen_width;
    }
	p = linetab[y1] + x1;
	if (dx > dy) {
        gc = dx >> 1;
        for (i = 0; i <= dx; i++) {
			*p++ = colour;
            gc -= dy;
            if (gc < 0) {
                p += step;
                gc += dx;
            }
        }
    } else {
        gc = dy >> 1;
        for (i = 0; i <= dy; i++) {
			*p = colour;
			p += step;
            gc -= dx;
            if (gc < 0) {
                p++;
                gc += dy;
            }
        }
    }
}

static int linestack[260];

static void poly_13(int npts, sVector2 pts[], int colour)
{
	int x, y;
	int32 big_x, gradient;
	int i, stacking, dy, ady, last_dy, sdy;
	int dx;
	int *stack_ptr;

	stack_ptr = linestack;
	x = pts[npts - 1].x;
	y = pts[npts - 1].y;
	stacking = TRUE;
	last_dy = 0;
	for (i = 0; i < npts; i++) {
		dy = pts[i].y - y;
		if (dy != 0) {
			dx = pts[i].x - x;
			if (dy > 0) {
				if (last_dy < 0) {
					*stack_ptr++ = big_x;
					stacking = FALSE;
				}
				last_dy = dy;
				ady = dy;
				sdy = 1;
				gradient = divtab[ady] * dx;
				big_x = ((int32)x << 16) + 0x8000L;
				if (dx > 0)
					big_x += gradient;
			} else {
				if (last_dy > 0) {
					*stack_ptr++ = big_x;
					stacking = FALSE;
				}
				last_dy = dy;
				ady = -dy;
				sdy = -1;
				gradient = divtab[ady] * dx;
				big_x = ((int32)x << 16) + 0x8000L;
				if (dx < 0)
					big_x += gradient;
			}
			if (stacking) {
				while (ady-- > 0) {
					*stack_ptr++ = big_x;
					big_x += gradient;
				}
			} else {
				while (ady-- > 0) {
					raw_hline_prim(big_x >> 16, *(--stack_ptr) >> 16, y, colour);
					y += sdy;
					big_x += gradient;
					if (stack_ptr == linestack) {
						stacking = TRUE;
						while (ady-- > 0) {
							*stack_ptr++ = big_x;
							big_x += gradient;
						}
						break;
					}
				}
			}
		}
		x = pts[i].x;
		y = pts[i].y;
    }
	if (stack_ptr > linestack) {
		raw_hline_prim(big_x >> 16, *(--stack_ptr) >> 16, y, colour);
	} else if (last_dy == 0) {
		for (i = 0; i < npts; i++) {
			dx = pts[i].x - x;
			if (dx != 0) {
				raw_hline_prim(x, pts[i].x, y, colour);
			}
			x = pts[i].x;
		}
	}
}


static void setup_prims(void)
{
	use_screen = use_screen_13;
	use_buffer = use_buffer_13;
	update_screen = update_screen_13;
	vsync = vsync_13;
	swap_hardware = swap_13;
	palette_prim = palette_13;
	clip_prim = clip_13;
	clear_prim = clear_13;
	pixel_prim = pixel_13;
	getpixel_prim = getpixel_13;
	block_prim = block_13;
	hline_prim = hline_13;
	vline_prim = vline_13;
	line_prim = line_13;
	poly_prim = poly_13;
	getbitmap_prim = getbitmap_13;
	bitmap_prim = bitmap_13;
	char_prim = char_13;
	remap_prim = remap_13;
	raw_hline_prim  = raw_hline_13;
	tex_hline_prim  = tex_hline_13;
	tex_line_prim  = tex_line_13;
}

void setup_screen_buffer(void)
{
	unsigned size, i, *p;

	screen_base = (uchar *)_x386_zero_base_ptr + 0xa0000;
	size = screen_width * (screen_height + 2);
	if (size > screen_size) {
		screen_size = size;
		if (screen_memory)
			free(screen_memory);
		screen_memory = malloc(screen_size);
		if (screen_memory == NULL)
			stop("No memory for screen buffer");
		screen_buffer = screen_memory + screen_width;
	}
	p = (unsigned int *)screen_memory;
	for (i = screen_size / (unsigned)sizeof(int); i; i--)
		*p++ = 0;
}

void setup_vga13(void)
{
	screen_mode = 0x13;
	screen_width = 320;
	screen_height = 200;
	screen_colours = 256;
	double_buffered = FALSE;
	setup_prims();
	setup_screen_buffer();
	use_screen_13();
	set_mode(screen_mode);
}

void reset_vga13(void)
{
	screen_mode = 0x13;
	screen_width = 320;
	screen_height = 200;
	screen_colours = 256;
	double_buffered = FALSE;
	setup_prims();
	setup_screen_buffer();
	use_screen_13();
	set_mode(screen_mode);
}

void tidyup_vga13(void)
{
}

