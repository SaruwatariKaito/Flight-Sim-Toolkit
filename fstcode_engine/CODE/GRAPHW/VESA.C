/* vesa graphics prims */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dos.h>

#include "graph.h"

extern void c_polygon(int npts, sVector2 pts[], int col);
extern void c_tex_hline(int x1, int x2, int y, int32 btx1, int32 btx2, int32 bty1, int32 bty2, int tw, uchar *tex);
extern void c_tex_line(int x1, int y1, int x2, int y2, uchar *pixels, int length);

extern int32 divtab[];

extern uchar *linetab[];

extern void setup_screen_buffer(void);
extern uchar *screen_memory;
extern uchar *screen_buffer;

static void *_x386_zero_base_ptr = 0;
static uchar *screen_base = NULL;

static int line_size = 640;
static int32 window_size = 64L * 1024L;
static int32 page_size = 64L * 1024L;
static int pages_per_window = 1;

static int get_page(void)
{
	union REGS r;

	r.x.eax = 0x4f05;
	r.x.ebx = 0x0100;
	int386(0x10, &r, &r);
	return(r.x.edx);
}

static void set_page(int n)
{
	union REGS r;

	r.x.eax = 0x4f05;
	r.x.ebx = 0x0000;
	r.x.edx = n;
	int386(0x10, &r, &r);
}

static void standard_mode_info(void)
{
	page_size = 0x40 * 1024;
	window_size = 0x40 * 1024;
	line_size = 640;
	pages_per_window = window_size / page_size;
}

static void skip_line(FILE *fp)
{
	int c;

	while ((c = fgetc(fp)) != '\n')
		;
}

static void setup_mode_info(void)
{
	int i, val;
	FILE *fp;
	char s1[40], s2[40];

	fp = fopen("VESA.DAT", "r");
	if (fp) {
		for (i = 0; i < 7; i++)
			skip_line(fp);
		fscanf(fp, "%s %s %x\n", s1, s2, &val);
		page_size = val * 1024;
		fscanf(fp, "%s %s %x\n", s1, s2, &val);
		window_size = val * 1024;
		for (i = 0; i < 2; i++)
			skip_line(fp);
		fscanf(fp, "%s %s", s1, s2);
		fscanf(fp, "%s %s %d\n", s1, s2, &val);
		line_size = val;
	} else {
		standard_mode_info();
	}
	if (page_size > window_size)
		page_size = window_size;
	if (page_size > 0)
		pages_per_window = window_size / page_size;
	else
		pages_per_window = 0;
/*	printf("page size = %x\n", page_size);
	printf("window size = %x\n", window_size);
	printf("line size = %d\n", line_size);
	printf("pages per window = %x\n", pages_per_window);*/
}

static union REGS pr1, pr2, pr3;
static int current_window = 0;
static int current_read_window = 0;
static int32 window_base = 0;

static uchar *screen_address(int x, int y)
{
	int32 offset;
	int window;

	offset = (y * line_size) + x;
	window = offset / window_size;
	if (window != current_window) {
		current_window = window;
		window_base = window * window_size;
		pr1.x.edx = window * pages_per_window;
		int386(0x10, &pr1, &pr3);
	}
	return (screen_base + (offset - window_base));
}

static uchar *read_screen_address(int x, int y)
{
	int32 offset;
	int window;

	offset = (y * line_size) + x;
	window = offset / window_size;
	if (window != current_window) {
		current_window = window;
		window_base = window * window_size;
		pr1.x.edx = window * pages_per_window;
		int386(0x10, &pr1, &pr3);
	}
	if (window != current_read_window) {
		current_read_window = window;
		window_base = window * window_size;
		pr2.x.edx = window * pages_per_window;
		int386(0x10, &pr2, &pr3);
	}
	return (screen_base + (offset - window_base));
}

static void init_vesa(void)
{
	setup_mode_info();
	screen_base = (uchar *)_x386_zero_base_ptr + 0xa0000;
	pr1.x.eax = 0x4f05;
	pr1.x.ebx = 0x0000;
	pr1.x.edx = 0;
	pr2.x.eax = 0x4f05;
	pr2.x.ebx = 0x0001;
	pr2.x.edx = 0;
	set_page(0);
}

static void term_vesa(void)
{
	set_page(0);
}

static void pixel_vesa(int x, int y, int col)
{
	uchar *p;

	if ((x >= clip_region.left) && (x <= clip_region.right) &&
		(y >= clip_region.top) && (y <= clip_region.bottom)) {
		p = screen_address(x, y);
		*p = col;
	}
}

static int getpixel_vesa(int x, int y)
{
	uchar *p;

	if ((x >= clip_region.left) && (x <= clip_region.right) &&
		(y >= clip_region.top) && (y <= clip_region.bottom)) {
		p = read_screen_address(x, y);
		return (*p);
	}
	return 0;
}

static void getbitmap_vesa(int x, int y, int w, uchar *bitmap)
{
	uchar *p;

	p = read_screen_address(x, y);
	while (w--)
		*bitmap++ = *p++;
}

static void bitmap_vesa(int x, int y, int w, uchar *bitmap)
{
	int gap, offset;
	uchar *p;

	p = screen_address(x, y);
	offset = p - screen_base;
	if ((offset + w) <= window_size) {
		while (w--)
			*p++ = *bitmap++;
	} else {
		gap = window_size - offset;
		w -= gap;
		x += gap;
		while (gap--)
			*p++ = *bitmap++;
		p = screen_address(x, y);
		while (w--)
			*p++ = *bitmap++;
	}
}

static void remap_vesa(int x, int y, int w, uchar *map)
{
	uchar *p;

	p = screen_address(x, y);
	while (w--)
		*p++ = map[*p];
}

static void char_vesa(int x, int y, int width, int pattern, int col)
{
	int x2;
	uchar *p;

	if ((y >= clip_region.top) && (y <= clip_region.bottom)) {
		x2 = x + width;
		if ((x < clip_region.right) && (x2 > clip_region.left)) {
			if (x < clip_region.left) {
				width -= clip_region.left - x;
				pattern <<= clip_region.left - x;
				x = clip_region.left;
			}
			if (x2 > clip_region.right) {
				x2 = clip_region.right;
				width = x2 - x;
			}
			p = screen_address(x, y);
			while (width--) {
				if (pattern & 0x8000)
					*p++ = col;
				else
					p++;
				pattern <<= 1;
			}
		}
	}
}

static void hline_vesa(int x1, int x2, int y, int col)
{
	int x;
	uchar *p;

	if ((y >= clip_region.top) && (y <= clip_region.bottom)) {
		if (x1 > x2) {
			x = x1;
			x1 = x2;
			x2 = x;
		}
		if ((x1 < clip_region.right) && (x2 > clip_region.left)) {
			if (x1 < clip_region.left)
				x1 = clip_region.left;
			if (x2 > clip_region.right)
				x2 = clip_region.right;
			p = screen_address(x1, y);
			for (x = x1; x <= x2; x++)
				*p++ = col;
		}
	}
}

static void vline_vesa(int y1, int y2, int x, int col)
{
	int y;
	uchar *p;

	if ((x <= clip_region.right) && (x >= clip_region.left)) {
		if (y2 < y1){
			y = y2;
			y2 = y1;
			y1 = y;
		}
		if ((y1 < clip_region.bottom) && (y2 > clip_region.top)) {
	   		if (y1 < clip_region.top)
				y1 = clip_region.top;
			if (y2 > clip_region.bottom)
				y2 = clip_region.bottom;
			p = screen_address(x, y1);
			for (y = y1; y <= y2; y++, p += line_size)
				*p = col;
		}
	}
}

static void block_vesa(int x, int y, int col)
{
	uchar *p;

	if ((x >= clip_region.left) && (x < clip_region.right) &&
		(y >= clip_region.top) && (y < clip_region.bottom)) {
		p = screen_address(x, y);
		*p++ = col;
		*p = col;
		p += line_size;
		*p-- = col;
		*p = col;
	}
}

static void clear_vesa(int y1, int y2, int col)
{
	int y;

	for (y = y1; y <= y2; y++) {
		hline_vesa(0, screen_width, y, col);
	}
}

static void line_vesa(int x1, int y1, int x2, int y2, int col)
{
    int dx, dy, step, gc, x, y;

	if ((dx = x2 - x1) < 0) {
        dx = -dx;
        x = x2;
		x2 = x1;
        dy = y1 - y2;
        y = y2;
		y2 = y1;
    } else {
        x = x1;
        dy = y2 - y1;
        y = y1;
    }
    if (dy < 0) {
        dy = -dy;
        step = -1;
    } else {
        step = 1;
    }
    if (dx > dy) {
        gc = dx >> 1;
		while (x <= x2) {
			pixel_prim(x++, y, col);
            gc -= dy;
            if (gc < 0) {
                y += step;
                gc += dx;
            }
        }
    } else {
        gc = dy >> 1;
		while (y != y2) {
			pixel_prim(x, y += step, col);
            gc -= dx;
            if (gc < 0) {
                x++;
                gc += dy;
            }
        }
		pixel_prim(x, y, col);
    }
}

/* Assumes source and dest are longword aligned */
static void fast_memcopy(uchar *source, uchar *dest, int n)
{
	long *ps, *pd;

	n >>= 2;
	ps = (long *)source;
	pd = (long *)dest;
	while (n--)
		*pd++ = *ps++;
}

static void update_screen_vesa(int y1, int y2)
{
	int window, page, total_size, first_size, y;
	int32 offset;
	uchar *p1, *p2;

	if (line_size == screen_width) {
		total_size = ((y2 + 1) - y1) * screen_width;
		offset = y1 * screen_width;
		p1 = screen_buffer + offset;
		window = offset / window_size;
		page = window * pages_per_window;
		offset -= window * window_size;
		p2 = screen_base + offset;
		first_size = window_size - offset;
		current_window = window++;
		pr1.x.edx = page;
		page += pages_per_window;
		int386(0x10, &pr1, &pr3);
		fast_memcopy(p1, p2, first_size);
		total_size -= first_size;
		p1 += first_size;
		p2 = screen_base;
		while (total_size > 0) {
			current_window = window++;
			pr1.x.edx = page;
			page += pages_per_window;
			int386(0x10, &pr1, &pr3);
			if (total_size > window_size)
				fast_memcopy(p1, p2, window_size);
			else
				fast_memcopy(p1, p2, total_size);
			total_size -= window_size;
			p1 += window_size;
		}
		window_base = current_window * window_size;
	} else {
		p1 = screen_buffer + (y1 * screen_width);
		for (y = y1; y <= y2; y++) {
			p2 = screen_address(0, y);
			fast_memcopy(p1, p2, screen_width);
			p1 += screen_width;
		}
	}
}

extern void c_polygon(int npts, sVector2 pts[], int colour);

static void use_screen_vesa(void)
{
	int i;
	uchar *p;

	p = screen_base;
	for (i = 0; i <= screen_height; i++, p += screen_width) {
		linetab[i] = p;
	}
	clear_prim = clear_vesa;
	pixel_prim = pixel_vesa;
	getpixel_prim = getpixel_vesa;
	block_prim  = block_vesa;
	hline_prim  = hline_vesa;
	vline_prim  = vline_vesa;
	bitmap_prim = bitmap_vesa;
	getbitmap_prim = getbitmap_vesa;
	char_prim = char_vesa;
	remap_prim = remap_vesa;
	raw_hline_prim = hline_vesa;
}

extern void vsync_13(void);
extern void palette_13(int n, int r, int g, int b);
extern void clip_13(int left, int right, int top, int bottom);
extern void clear_13(int y1, int y2, int col);
extern void pixel_13(int x, int y, int col);
extern int getpixel_13(int x, int y);
extern void block_13(int x, int y, int colour);
extern void hline_13(int x1, int x2, int y, int colour);
extern void vline_13(int y1, int y2, int x, int colour);
extern void bitmap_13(int x, int y, int w, uchar *bitmap);
extern void getbitmap_13(int x, int y, int w, uchar *bitmap);
extern void char_13(int x, int y, int width, int pattern, int colour);
extern void remap_13(int x, int y, int w, uchar *map);
extern void raw_hline_13(int x1, int x2, int y, int colour);

static void use_buffer_vesa(void)
{
	int i;
	uchar *p;

	p = screen_buffer;
	for (i = 0; i <= screen_height; i++, p += screen_width) {
		linetab[i] = p;
	}
	clear_prim = clear_13;
	pixel_prim = pixel_13;
	getpixel_prim = getpixel_13;
	block_prim = block_13;
	hline_prim = hline_13;
	vline_prim = vline_13;
	bitmap_prim = bitmap_13;
	getbitmap_prim = getbitmap_13;
	char_prim = char_13;
	remap_prim = remap_13;
	raw_hline_prim = raw_hline_13;
}

static void swap_vesa(int draw_screen, int view_screen)
{
	if (draw_screen == view_screen) {
		use_screen_vesa();
	} else {
		use_buffer_vesa();
		update_screen_vesa(clip_region.top, clip_region.bottom);
	}
}

static void setup_prims(void)
{
	use_screen = use_screen_vesa;
	use_buffer = use_buffer_vesa;
	update_screen = update_screen_vesa;
	vsync = vsync_13;
	swap_hardware = swap_vesa;
	palette_prim = palette_13;
	clip_prim = clip_13;
	clear_prim = clear_vesa;
	pixel_prim = pixel_vesa;
	getpixel_prim = getpixel_vesa;
	block_prim = block_vesa;
	hline_prim = hline_vesa;
	vline_prim = vline_vesa;
	line_prim = line_vesa;
	poly_prim = c_polygon;
	getbitmap_prim = getbitmap_vesa;
	bitmap_prim = bitmap_vesa;
	char_prim = char_vesa;
	remap_prim = remap_vesa;
	raw_hline_prim  = hline_vesa;
	tex_hline_prim  = c_tex_hline;
	tex_line_prim  = c_tex_line;
}

void setup_vesa(void)
{
	screen_mode = 0x101;
	screen_width = 640;
	screen_height = 480;
	screen_colours = 256;
	double_buffered = FALSE;
	setup_prims();
	setup_screen_buffer();
	set_mode(screen_mode);
	init_vesa();
	use_screen_vesa();
}

void reset_vesa(void)
{
	screen_mode = 0x101;
	screen_width = 640;
	screen_height = 480;
	screen_colours = 256;
	double_buffered = FALSE;
	setup_prims();
	setup_screen_buffer();
	set_mode(screen_mode);
	init_vesa();
	use_screen_vesa();
}

void tidyup_vesa(void)
{
}

