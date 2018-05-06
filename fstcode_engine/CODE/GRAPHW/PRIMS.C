/* Generic graphics primitives */

#include <stdlib.h>

#include "graph.h"

extern int32 divtab[];

void c_getbitmap(int x, int y, int w, uchar *col)
{
	int max;

	for (max = x + w; x < max; x++)
		*col++ = getpixel_prim(x, y);
}

void c_remap(int x, int y, int w, uchar *map)
{
	int i;
	uchar bitmap[640];

	if ((y >= clip_region.top) && (y <= clip_region.bottom)) {
		getbitmap_prim(x, y, w, bitmap);
		for (i = 0; i < w; i++)
			bitmap[i] = map[bitmap[i]];
		bitmap_prim(x, y, w, bitmap);
	}
}

void c_tex_hline(int x1, int x2, int y, int32 btx1, int32 btx2, int32 bty1, int32 bty2, int tw, uchar *tex)
{
	int x, dx, col, pixels;
	int32 tx_grad, ty_grad;
	uchar *p, line_buffer[MAX_SCREEN_X];

	if ((y < 0) || (y >= screen_height))
		return;
	if (x1 < x2) {
		if ((x1 < 0) || (x2 >= screen_width))
			return;
		dx = (x2 - x1) + 1;
		if (dx > 0) {
			tx_grad = (btx2 - btx1) / dx;
			ty_grad = (bty2 - bty1) / dx;
		} else {
			tx_grad = btx2 - btx1;
			ty_grad = bty2 - bty1;
		}
		p = line_buffer;
		pixels = 0;
		for (x = x1; x <= x2; x++) {
			btx1 += tx_grad;
			bty1 += ty_grad;
			col = *((uchar *)tex + (tw * (bty1 >> 16)) + (btx1 >> 16));
			if (col != 0) {
				*p++ = col;
				pixels++;
			} else {
				if (pixels > 0) {
					bitmap_prim(x - pixels, y, pixels, line_buffer);
					pixels = 0;
					p = line_buffer;
				}
			}
		}
		if (pixels > 0) {
			bitmap_prim(x - pixels, y, pixels, line_buffer);
		}
	} else {
		if ((x2 < 0) || (x1 >= screen_width))
			return;
		dx = (x1 - x2) + 1;
		if (dx > 0) {
			tx_grad = (btx1 - btx2) / dx;
			ty_grad = (bty1 - bty2) / dx;
		} else {
			tx_grad = btx1 - btx2;
			ty_grad = bty1 - bty2;
		}
		p = line_buffer;
		pixels = 0;
		for (x = x2; x <= x1; x++) {
			btx2 += tx_grad;
			bty2 += ty_grad;
			col = *((uchar *)tex + (tw * (bty2 >> 16)) + (btx2 >> 16));
			if (col != 0) {
				*p++ = col;
				pixels++;
			} else {
				if (pixels > 0) {
					bitmap_prim(x - pixels, y, pixels, line_buffer);
					pixels = 0;
					p = line_buffer;
				}
			}
		}
		if (pixels > 0) {
			bitmap_prim(x - pixels, y, pixels, line_buffer);
		}
	}
}

void c_tex_line(int x1, int y1, int x2, int y2, uchar *pixels, int length)
{
    int dx, dy, x_step, y_step, gc, x, y;
	int32 big_t, t_grad;

	if ((dx = x2 - x1) < 0) {
        dx = -dx;
		x_step = -1;
    } else {
		x_step = 1;
    }
	if ((dy = y2 - y1) < 0) {
        dy = -dy;
        y_step = -1;
    } else {
        y_step = 1;
    }
	x = x1;
	y = y1;
	big_t = 0;
    if (dx > dy) {
		t_grad = divtab[dx] * length;
        gc = dx >> 1;
		while (x != x2) {
			pixel_prim(x += x_step, y, pixels[big_t >> 16]);
			big_t += t_grad;
            gc -= dy;
            if (gc < 0) {
				pixel_prim(x, y, pixels[big_t >> 16]);
                y += y_step;
                gc += dx;
            }
        }
		pixel_prim(x, y, pixels[big_t >> 16]);
    } else {
		t_grad = divtab[dy] * length;
        gc = dy >> 1;
		while (y != y2) {
			pixel_prim(x, y += y_step, pixels[big_t >> 16]);
			big_t += t_grad;
            gc -= dx;
            if (gc < 0) {
				pixel_prim(x, y, pixels[big_t >> 16]);
                x += x_step;
                gc += dy;
            }
        }
		pixel_prim(x, y, pixels[big_t >> 16]);
    }
}

int linestack[490];

/* Points must be in clockwise order */

void c_polygon(int npts, sVector2 pts[], int col)
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
					raw_hline_prim(big_x >> 16, *(--stack_ptr) >> 16, y, col);
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
		raw_hline_prim(big_x >> 16, *(--stack_ptr) >> 16, y, col);
	} else if (last_dy == 0) {
		for (i = 0; i < npts; i++) {
			dx = pts[i].x - x;
			if (dx != 0) {
				raw_hline_prim(x, pts[i].x, y, col);
			}
			x = pts[i].x;
		}
	}
}

void c_triangle(sVector2 pts[3], int col)
{
	int dx_l, dy_l, dx_r, dy_r, y;
	int32 grad_l, grad_r;
	int32 bx_l, bx_r;
	sVector2 top, left, right;

	if (pts[0].y < pts[1].y) {
		if (pts[0].y < pts[2].y) {
			top = pts[0];
			if (pts[1].x < pts[2].x) {
				left = pts[1];
				right = pts[2];
			} else {
				left = pts[2];
				right = pts[1];
			}
		} else {
			top = pts[2];
			if (pts[0].x < pts[1].x) {
				left = pts[0];
				right = pts[1];
			} else {
				left = pts[1];
				right = pts[0];
			}
		}
	} else {
		if (pts[1].y < pts[2].y) {
			top = pts[1];
			if (pts[0].x < pts[2].x) {
				left = pts[0];
				right = pts[2];
			} else {
				left = pts[2];
				right = pts[0];
			}
		} else {
			top = pts[2];
			if (pts[0].x < pts[1].x) {
				left = pts[0];
				right = pts[1];
			} else {
				left = pts[1];
				right = pts[0];
			}
		}
	}
	dy_l = left.y - top.y;
	dy_r = right.y - top.y;
	dx_l = left.x - top.x;
	dx_r = right.x - top.x;
	grad_l = divtab[dy_l] * dx_l;
	grad_r = divtab[dy_r] * dx_r;
	bx_l = bx_r = ((int32)top.x << 16) + 0x8000L;
	y = top.y;
	if (dy_l < dy_r) {
		while (dy_l-- > 0) {
			bx_l += grad_l;
			bx_r += grad_r;
			hline_prim(bx_l >> 16, bx_r >> 16, y++, col);
		}
		dy_l = right.y - left.y;
		dx_l = right.x - left.x;
		grad_l = divtab[dy_l] * dx_l;
		bx_l = ((int32)left.x << 16) + 0x8000L;
		while (dy_l-- > 0) {
			bx_l += grad_l;
			bx_r += grad_r;
			hline_prim(bx_l >> 16, bx_r >> 16, y++, col);
		}
	} else {
		while (dy_r-- > 0) {
			bx_l += grad_l;
			bx_r += grad_r;
			hline_prim(bx_l >> 16, bx_r >> 16, y++, col);
		}
		dy_r = left.y - right.y;
		dx_r = left.x - right.x;
		grad_r = divtab[dy_r] * dx_r;
		bx_r = ((int32)right.x << 16) + 0x8000L;
		while (dy_r-- > 0) {
			bx_l += grad_l;
			bx_r += grad_r;
			hline_prim(bx_l >> 16, bx_r >> 16, y++, col);
		}
	}
}

void c_polytri(int npts, sVector2 pts[], int col)
{
	int i;
	sVector2 tripts[3];

	if (npts == 3) {
		c_triangle(pts, col);
	} else if (npts == 4) {
		c_triangle(pts, col);
		tripts[0] = pts[2];
		tripts[1] = pts[3];
		tripts[2] = pts[0];
		c_triangle(tripts, col);
	} else {
		c_triangle(pts, col);
		for (i = 3; i < npts; i++) {
			tripts[0] = pts[i - 1];
			tripts[1] = pts[i];
			tripts[2] = pts[0];
			c_triangle(tripts, col);
		}
	}
}

