/* graph2.c */

#include <stdlib.h>

#include "graph.h"

static char *copyright = "Graphics software copyright Simis Ltd 1992";

sVector zero = {0, 0, 0};

Screen_clip clip_region = {0, 319, 0, 199};

void clipping(int left, int right, int top, int bottom)
{
	if (top < 0)
		top = 0;
	if (bottom >= screen_height)
		bottom = screen_height - 1;
	clip_region.left = left;
    clip_region.right = right;
    clip_region.top = top;
    clip_region.bottom = bottom;
	clip_prim(left, right, top, bottom);
}

void rot2d(sVector2 *v1, sVector2 *v2, int ang)
{
    int sa, ca;
    int x, y;

    sa = SIN(ang); ca = COS(ang);
    x = v1->x; y = v1->y;
    v2->x = (int)SR((MUL(x,ca) - MUL(y,sa)), SSHIFT);
    v2->y = (int)SR((MUL(x,sa) + MUL(y,ca)), SSHIFT);
}

void remap_screen(int x1, int y1, int x2, int y2, uchar *map)
{
	int y, w;

	w = (x2 - x1) + 1;
	for (y = y1; y <= y2; y++) {
   		remap_prim(x1, y, w, map);
	}
}

static void num0(int x, int y, int col)
{
    hline_prim(x - 1, x + 1, y - 2, col);
    hline_prim(x - 1, x + 1, y + 2, col);
    vline_prim(y - 1, y + 1, x - 1, col);
    vline_prim(y - 1, y + 1, x + 1, col);
}

static void num1(int x, int y, int col)
{
    vline_prim(y - 2, y + 2, x, col);
}

static void num2(int x, int y, int col)
{
    hline_prim(x - 1, x + 1, y - 2, col);
    pixel_prim(x + 1, y - 1, col);
    hline_prim(x - 1, x + 1, y, col);
    pixel_prim(x - 1, y + 1, col);
    hline_prim(x - 1, x + 1, y + 2, col);
}

static void num3(int x, int y, int col)
{
    hline_prim(x - 1, x + 1, y - 2, col);
    pixel_prim(x + 1, y - 1, col);
    hline_prim(x - 1, x + 1, y, col);
    pixel_prim(x + 1, y + 1, col);
    hline_prim(x - 1, x + 1, y + 2, col);
}

static void num4(int x, int y, int col)
{
    vline_prim(y - 2, y, x - 1, col);
    hline_prim(x, x + 1, y, col);
    vline_prim(y - 1, y + 2, x + 1, col);
}

static void num5(int x, int y, int col)
{
    hline_prim(x - 1, x + 1, y - 2, col);
    pixel_prim(x - 1, y - 1, col);
    hline_prim(x - 1, x + 1, y, col);
    pixel_prim(x + 1, y + 1, col);
    hline_prim(x - 1, x + 1, y + 2, col);
}

static void num6(int x, int y, int col)
{
    hline_prim(x - 1, x + 1, y - 2, col);
    pixel_prim(x - 1, y - 1, col);
    hline_prim(x - 1, x + 1, y, col);
    pixel_prim(x - 1, y + 1, col);
    pixel_prim(x + 1, y + 1, col);
    hline_prim(x - 1, x + 1, y + 2, col);
}

static void num7(int x, int y, int col)
{
    hline_prim(x - 1, x + 1, y - 2, col);
    vline_prim(y - 1, y + 2, x + 1, col);
}

static void num8(int x, int y, int col)
{
    hline_prim(x - 1, x + 1, y - 2, col);
    pixel_prim(x - 1, y - 1, col);
    pixel_prim(x + 1, y - 1, col);
    hline_prim(x - 1, x + 1, y, col);
    pixel_prim(x - 1, y + 1, col);
    pixel_prim(x + 1, y + 1, col);
    hline_prim(x - 1, x + 1, y + 2, col);
}

static void num9(int x, int y, int col)
{
    hline_prim(x - 1, x + 1, y - 2, col);
    pixel_prim(x - 1, y - 1, col);
    pixel_prim(x + 1, y - 1, col);
    hline_prim(x - 1, x + 1, y, col);
    pixel_prim(x + 1, y + 1, col);
    hline_prim(x - 1, x + 1, y + 2, col);
}

static void (* num_calls[10])(int, int, int) = {num0, num1, num2,
    num3, num4, num5, num6, num7, num8, num9
};

void vector_digit(int val, int x, int y, int col)
{
	if ((val >= 0) && (val <= 9))
    	(* num_calls[val])(x, y, COL(col));
}

static int rem_tab[100] = {
	0,1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6,7,8,9,
	0,1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6,7,8,9,
	0,1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6,7,8,9,
	0,1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6,7,8,9,
	0,1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6,7,8,9,
};
static int div_tab[100] = {
	0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,
	2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,
	4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,
	6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7,7,
	8,8,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,9,
};

void vector_number(int val, int x, int y, int col)
{
    int v, c;

	c = COL(col);
	v = ABS(val);
    while (v >= 10) {
		if (v < 100) {
    		(* num_calls[rem_tab[v]])(x, y, c);
        	v = div_tab[v];
		} else {
    		(* num_calls[v % 10])(x, y, c);
        	v /= 10;
		}
        x -= 4;
    }
    (* num_calls[v])(x, y, c);
    if (val < 0)
        hline_prim(x - 5, x - 3, y, c);
}

void character(int x, int y, sCharacter *s, int colour)
{
	register int max;
	register unsigned *p;

	colour = COL(colour);
	for (max = y + s->height, p = s->data; y < max; y++, p++) {
		char_prim(x, y, s->width, *p, colour);
	}
}

extern void rectangle(int x1, int y1, int x2, int y2, int colour)
{
	register int y;

	colour = COL(colour);
	if (y1 < y2) {
		for (y = y1; y <= y2; y++)
			hline_prim(x1, x2, y, colour);
	} else {
		for (y = y2; y <= y1; y++)
			hline_prim(x1, x2, y, colour);
	}
}

void ellipse(int cx, int cy, int radius, int x_squash, int y_squash, int colour)
{
	int x, sx, y, sy, delta, d;

	colour = COL(colour);
	x = 0;
	y = radius;
	delta = (1 - radius) << 1;
	sy = (y * y_squash) >> 10;
	pixel_prim(cx, cy + sy, colour);
	pixel_prim(cx, cy - sy, colour);
	while (y > 0) {
		if (delta < 0) {
			d = (delta << 1) + (y << 1) - 1;
			if (d <= 0) {
				x += 1;
				delta += (x << 1) + 1;
			} else {
				x += 1;
				y -= 1;
				delta += (x << 1) - (y << 1) + 2;
			}
		} else if (delta > 0) {
			d = (delta << 1) - (x << 1) - 1;
			if (d <= 0) {
				x += 1;
				y -= 1;
				delta += (x << 1) - (y << 1) + 2;
			} else {
				y -= 1;
				delta -= (y << 1) + 1;
			}
		} else {
			x += 1;
			y -= 1;
			delta += (x << 1) - (y << 1) + 2;
		}
		sx = (x * x_squash) >> 10;
		sy = (y * y_squash) >> 10;
		pixel_prim(cx + sx, cy + sy, colour);
		pixel_prim(cx + sx, cy - sy, colour);
		pixel_prim(cx - sx, cy + sy, colour);
		pixel_prim(cx - sx, cy - sy, colour);
	}
}

void circle(int cx, int cy, int radius, int colour)
{
	int x, y, delta, d;

	colour = COL(colour);
	x = 0;
	y = radius;
	delta = (1 - radius) << 1;
	pixel_prim(cx, cy + y, colour);
	pixel_prim(cx, cy - y, colour);
	pixel_prim(cx + y, cy, colour);
	pixel_prim(cx - y, cy, colour);
	while (y > x) {
		if (delta < 0) {
			d = (delta << 1) + (y << 1) - 1;
			if (d <= 0) {
				x += 1;
				delta += (x << 1) + 1;
			} else {
				x += 1;
				y -= 1;
				delta += (x << 1) - (y << 1) + 2;
			}
		} else if (delta > 0) {
			d = (delta << 1) - (x << 1) - 1;
			if (d <= 0) {
				x += 1;
				y -= 1;
				delta += (x << 1) - (y << 1) + 2;
			} else {
				y -= 1;
				delta -= (y << 1) + 1;
			}
		} else {
			x += 1;
			y -= 1;
			delta += (x << 1) - (y << 1) + 2;
		}
		pixel_prim(cx + x, cy + y, colour);
		pixel_prim(cx + y, cy + x, colour);
		pixel_prim(cx + x, cy - y, colour);
		pixel_prim(cx + y, cy - x, colour);
		pixel_prim(cx - x, cy + y, colour);
		pixel_prim(cx - y, cy + x, colour);
		pixel_prim(cx - x, cy - y, colour);
		pixel_prim(cx - y, cy - x, colour);
	}
}

void circle_f(int cx, int cy, int radius, int colour)
{
	int x, y, last_x, last_y, delta, d;

	colour = COL(colour);
	last_x = x = 0;
	last_y = y = radius;
	delta = (1 - radius) << 1;
	while (y > 0) {
		if (delta < 0) {
			d = (delta << 1) + (y << 1) - 1;
			if (d <= 0) {
				x += 1;
				delta += (x << 1) + 1;
			} else {
				x += 1;
				y -= 1;
				delta += (x << 1) - (y << 1) + 2;
			}
		} else if (delta > 0) {
			d = (delta << 1) - (x << 1) - 1;
			if (d <= 0) {
				x += 1;
				y -= 1;
				delta += (x << 1) - (y << 1) + 2;
			} else {
				y -= 1;
				delta -= (y << 1) + 1;
			}
		} else {
			x += 1;
			y -= 1;
			delta += (x << 1) - (y << 1) + 2;
		}
		if (y != last_y) {
			hline_prim(cx - last_x, cx + last_x, cy - last_y, colour);
			hline_prim(cx - last_x, cx + last_x, cy + last_y, colour);
		}
		last_x = x;
		last_y = y;
	}
	hline_prim(cx - last_x, cx + last_x, cy, colour);
}

static void remap_clipped(int x, int y, int w, uchar *map)
{
	int x2;

	x2 = x + w;
	if ((x >= clip_region.left) && (x2 < clip_region.right)) {
		remap_prim(x, y, w, map);
	} else {
		if (x >= clip_region.left) {
			if (x < clip_region.right)
				remap_prim(x, y, clip_region.right - x, map);
		} else {
			if (x2 > clip_region.left)
				remap_prim(clip_region.left, y, x2, map);
		}
	}
}

void circle_r(int cx, int cy, int radius, uchar *map)
{
	int x, y, delta, l, d;

	x = 0;
	y = radius;
	delta = (1 - radius) << 1;
	l = 0;
	while (y > l) {
		if (delta < 0) {
			d = (delta << 1) + (y << 1) - 1;
			if (d <= 0) {
				x += 1;
				delta += (x << 1) + 1;
			} else {
				x += 1;
				y -= 1;
				delta += (x << 1) - (y << 1) + 2;
			}
		} else if (delta > 0) {
			d = (delta << 1) - (x << 1) - 1;
			if (d <= 0) {
				x += 1;
				y -= 1;
				delta += (x << 1) - (y << 1) + 2;
			} else {
				y -= 1;
				delta -= (y << 1) + 1;
			}
		} else {
			x += 1;
			y -= 1;
			delta += (x << 1) - (y << 1) + 2;
		}
   		remap_clipped(cx - x, cy - y, 2 * x, map);
   		remap_clipped(cx - x, cy + y, 2 * x, map);
	}
}

int clip_xedge(sVector2 *s1, sVector2 *s2)
{
    register int n = 1, dx;

    if (s2->x >= s1->x) {
        if (s1->x < clip_region.left) {
            if (s2->x <= clip_region.left)
                return 0;
			dx = s2->x - s1->x;
			if (dx != 0)
				s1->y += MULDIV((s2->y - s1->y), (clip_region.left - s1->x), dx);
            s1->x = clip_region.left;
            n = 1;
        }
        if (s2->x > clip_region.right) {
            if (s1->x > clip_region.right)
                return 0;
            if (s1->x == clip_region.right)
                return 1;
			dx = s2->x - s1->x;
			if (dx != 0)
				s2->y -= MULDIV((s2->y - s1->y), (s2->x - clip_region.right), dx);
            s2->x = clip_region.right;
            n = 2;
        }
        return n;
    } else {
        if (s1->x > clip_region.right) {
            if (s2->x >= clip_region.right)
                return 0;
			dx = s1->x - s2->x;
			if (dx != 0)
				s1->y -= MULDIV((s1->y - s2->y), (s1->x - clip_region.right), dx);
            s1->x = clip_region.right;
            n = 1;
        }
        if (s2->x < clip_region.left) {
            if (s1->x < clip_region.left)
                return 0;
            if (s1->x == clip_region.left)
                return 1;
			dx = s1->x - s2->x;
			if (dx != 0)
				s2->y += MULDIV((s1->y - s2->y), (clip_region.left - s2->x), dx);
            s2->x = clip_region.left;
            n = 2;
        }
        return n;
    }
}

int clip_yedge(sVector2 *s1, sVector2 *s2)
{
    register int n = 1, dy;

    if (s2->y >= s1->y) {
        if (s1->y < clip_region.top) {
            if (s2->y <= clip_region.top)
                return 0;
			dy = s2->y - s1->y;
			if (dy != 0)
				s1->x += MULDIV((s2->x - s1->x), (clip_region.top - s1->y), dy);
            s1->y = clip_region.top;
            n = 1;
        }
        if (s2->y > clip_region.bottom) {
            if (s1->y > clip_region.bottom)
                return 0;
            if (s1->y == clip_region.bottom)
                return 1;
			dy = s2->y - s1->y;
			if (dy != 0)
				s2->x -= MULDIV((s2->x - s1->x), (s2->y - clip_region.bottom), dy);
            s2->y = clip_region.bottom;
            n = 2;
        }
        return n;
    } else {
        if (s1->y > clip_region.bottom) {
            if (s2->y >= clip_region.bottom)
                return 0;
			dy = s1->y - s2->y;
			if (dy != 0)
				s1->x -= MULDIV((s1->x - s2->x), (s1->y - clip_region.bottom), dy);
            s1->y = clip_region.bottom;
            n = 1;
        }
        if (s2->y < clip_region.top) {
            if (s1->y < clip_region.top)
                return 0;
            if (s1->y == clip_region.top)
                return 1;
			dy = s1->y - s2->y;
			if (dy != 0)
				s2->x += MULDIV((s1->x - s2->x), (clip_region.top - s2->y), dy);
            s2->y = clip_region.top;
            n = 2;
        }
        return n;
    }
}

static int clip_xedge_s(sVector2 *s1, sVector2 *s2, sVector2 *clip)
{
    register int dx;

    if (s2->x >= s1->x) {
        if (s1->x < clip_region.left) {
            if (s2->x <= clip_region.left)
                return 0;
			dx = s2->x - s1->x;
			if (dx != 0)
				clip->y = s1->y + MULDIV((s2->y - s1->y), (clip_region.left - s1->x), dx);
			else
				clip->y = s1->y;
            clip->x = clip_region.left;
        } else {
            *clip = *s1;
        }
        if (s2->x > clip_region.right) {
            if (s1->x > clip_region.right)
                return 0;
            if (s1->x == clip_region.right)
                return 1;
			clip++;
			dx = s2->x - s1->x;
			if (dx != 0)
				clip->y = s2->y - MULDIV((s2->y - s1->y), (s2->x - clip_region.right), dx);
			else
				clip->y = s2->y;
            clip->x = clip_region.right;
            return 2;
        }
        return 1;
    } else {
        if (s1->x > clip_region.right) {
            if (s2->x >= clip_region.right)
                return 0;
			dx = s1->x - s2->x;
			if (dx != 0)
				clip->y = s1->y - MULDIV((s1->y - s2->y), (s1->x - clip_region.right), dx);
			else
				clip->y = s1->y;
            clip->x = clip_region.right;
        } else {
            *clip = *s1;
        }
        if (s2->x < clip_region.left) {
            if (s1->x < clip_region.left)
                return 0;
            if (s1->x == clip_region.left)
                return 1;
			clip++;
			dx = s1->x - s2->x;
			if (dx != 0)
				clip->y = s2->y + MULDIV((s1->y - s2->y), (clip_region.left - s2->x), dx);
			else
				clip->y = s2->y;
            clip->x = clip_region.left;
            return 2;
        }
        return 1;
    }
}

static int clip_yedge_s(sVector2 *s1, sVector2 *s2, sVector2 *clip)
{
    register int dy;

    if (s2->y >= s1->y) {
        if (s1->y < clip_region.top) {
            if (s2->y <= clip_region.top)
                return 0;
			dy = s2->y - s1->y;
			if (dy != 0)
				clip->x = s1->x + MULDIV((s2->x - s1->x), (clip_region.top - s1->y), dy);
			else
				clip->x = s1->x;
			clip->y = clip_region.top;
        } else {
            *clip = *s1;
        }
        if (s2->y > clip_region.bottom) {
            if (s1->y > clip_region.bottom)
                return 0;
            if (s1->y == clip_region.bottom)
                return 1;
			clip++;
			dy = s2->y - s1->y;
			if (dy != 0)
				clip->x = s2->x - MULDIV((s2->x - s1->x), (s2->y - clip_region.bottom), dy);
			else
				clip->x = s2->x;
            clip->y = clip_region.bottom;
            return 2;
        }
        return 1;
    } else {
        if (s1->y > clip_region.bottom) {
            if (s2->y >= clip_region.bottom)
                return 0;
			dy = s1->y - s2->y;
			if (dy != 0)
				clip->x = s1->x - MULDIV((s1->x - s2->x), (s1->y - clip_region.bottom), dy);
			else
				clip->x = s1->x;
            clip->y = clip_region.bottom;
        } else {
            *clip = *s1;
        }
        if (s2->y < clip_region.top) {
            if (s1->y < clip_region.top)
                return 0;
            if (s1->y == clip_region.top)
                return 1;
			clip++;
			dy = s1->y - s2->y;
			if (dy != 0)
				clip->x = s2->x + MULDIV((s1->x - s2->x), (clip_region.top - s2->y), dy);
			else
				clip->x = s2->x;
            clip->y = clip_region.top;
            return 2;
        }
        return 1;
    }
}

/*static int clip_xedge_s(sVector2 *s1, sVector2 *s2, sVector2 *clip)
{
    sVector2 n1, n2, n3;

    if (s2->x >= s1->x) {
        if (s1->x < clip_region.left) {
            if (s2->x <= clip_region.left)
                return 0;
			n1 = *s1;
			n2 = *s2;
			while (n1.x < clip_region.left) {
    			n3.x = (n1.x + n2.x) >> 1;
    			n3.y = (n1.y + n2.y) >> 1;
				if (n3.x <= clip_region.left)
        			n1 = n3;
    			else
        			n2 = n3;
			}
			*clip = n1;
        } else {
            *clip = *s1;
        }
        if (s2->x > clip_region.right) {
            if (s1->x > clip_region.right)
                return 0;
            if (s1->x == clip_region.right)
                return 1;
			n1 = *s1;
			n2 = *s2;
			while (n2.x > clip_region.right) {
    			n3.x = (n1.x + n2.x) >> 1;
    			n3.y = (n1.y + n2.y) >> 1;
				if (n3.x >= clip_region.right)
        			n2 = n3;
    			else
        			n1 = n3;
			}
			clip++;
			*clip = n2;
            return 2;
        }
        return 1;
    } else {
        if (s1->x > clip_region.right) {
            if (s2->x >= clip_region.right)
                return 0;
			n2 = *s1;
			n1 = *s2;
			while (n2.x > clip_region.right) {
    			n3.x = (n1.x + n2.x) >> 1;
    			n3.y = (n1.y + n2.y) >> 1;
				if (n3.x >= clip_region.right)
        			n2 = n3;
    			else
        			n1 = n3;
			}
			*clip = n2;
        } else {
            *clip = *s1;
        }
        if (s2->x < clip_region.left) {
            if (s1->x < clip_region.left)
                return 0;
            if (s1->x == clip_region.left)
                return 1;
			n1 = *s2;
			n2 = *s1;
			while (n1.x < clip_region.left) {
    			n3.x = (n1.x + n2.x) >> 1;
    			n3.y = (n1.y + n2.y) >> 1;
				if (n3.x <= clip_region.left)
        			n1 = n3;
    			else
        			n2 = n3;
			}
			clip++;
			*clip = n1;
            return 2;
        }
        return 1;
    }
}

static int clip_yedge_s(sVector2 *s1, sVector2 *s2, sVector2 *clip)
{
    sVector2 n1, n2, n3;

    if (s2->y >= s1->y) {
        if (s1->y < clip_region.top) {
            if (s2->y <= clip_region.top)
                return 0;
			n1 = *s1;
			n2 = *s2;
			while (n1.y < clip_region.top) {
    			n3.x = (n1.x + n2.x) >> 1;
    			n3.y = (n1.y + n2.y) >> 1;
				if (n3.y <= clip_region.top)
        			n1 = n3;
    			else
        			n2 = n3;
			}
			*clip = n1;
        } else {
            *clip = *s1;
        }
        if (s2->y > clip_region.bottom) {
            if (s1->y > clip_region.bottom)
                return 0;
            if (s1->y == clip_region.bottom)
                return 1;
			n1 = *s1;
			n2 = *s2;
			while (n2.y > clip_region.bottom) {
    			n3.x = (n1.x + n2.x) >> 1;
    			n3.y = (n1.y + n2.y) >> 1;
				if (n3.y >= clip_region.bottom)
        			n2 = n3;
    			else
        			n1 = n3;
			}
			clip++;
			*clip = n2;
            return 2;
        }
        return 1;
    } else {
        if (s1->y > clip_region.bottom) {
            if (s2->y >= clip_region.bottom)
                return 0;
			n2 = *s1;
			n1 = *s2;
			while (n2.y > clip_region.bottom) {
    			n3.x = (n1.x + n2.x) >> 1;
    			n3.y = (n1.y + n2.y) >> 1;
				if (n3.y >= clip_region.bottom)
        			n2 = n3;
    			else
        			n1 = n3;
			}
			*clip = n2;
        } else {
            *clip = *s1;
        }
        if (s2->y < clip_region.top) {
            if (s1->y < clip_region.top)
                return 0;
            if (s1->y == clip_region.top)
                return 1;
			n1 = *s2;
			n2 = *s1;
			while (n1.y < clip_region.top) {
    			n3.x = (n1.x + n2.x) >> 1;
    			n3.y = (n1.y + n2.y) >> 1;
				if (n3.y <= clip_region.top)
        			n1 = n3;
    			else
        			n2 = n3;
			}
			clip++;
			*clip = n1;
            return 2;
        }
        return 1;
    }
}*/

/* Points must be in clockwise order */

void polygon(int npts, sVector2 pts[], int col)
{
    int i, n1, n;
    sVector2 *p1, *p2, xclip[MAX_PTS], clip[MAX_PTS];

    p1 = &pts[npts - 1];
	for (i = 0, n1 = 0; i < npts; i++) {
        p2 = &pts[i];
        n1 += clip_xedge_s(p1, p2, &xclip[n1]);
        p1 = p2;
    }
    p1 = &xclip[n1 - 1];
    for (i = 0, n = 0; i < n1; i++) {
        p2 = &xclip[i];
        n += clip_yedge_s(p1, p2, &clip[n]);
        p1 = p2;
    }
	if (n > 0)
		poly_prim(n, clip, COL(col));
}

void line(int x1, int y1, int x2, int y2, int col)
{
    if (y1 == y2) {
		hline_prim(x1, x2, y1, COL(col));
	} else if (x1 == x2) {
		vline_prim(y1, y2, x1, COL(col));
	} else if (x2 >= x1) {
        if (x1 < clip_region.left) {
            if (x2 < clip_region.left)
                return;
			y1 += MULDIV((y2 - y1), (clip_region.left - x1), (x2 - x1));
			x1 = clip_region.left;
        }
        if (x2 > clip_region.right) {
            if (x1 > clip_region.right)
                return;
			y2 -= MULDIV((y2 - y1), (x2 - clip_region.right), (x2 - x1));
            x2 = clip_region.right;
        }
        if (y2 >= y1) {
            if (y1 < clip_region.top) {
                if (y2 < clip_region.top)
                    return;
				x1 += MULDIV((x2 - x1), (clip_region.top - y1), (y2 - y1));
                y1 = clip_region.top;
            }
            if (y2 > clip_region.bottom) {
                if (y1 > clip_region.bottom)
                    return;
				x2 -= MULDIV((x2 - x1), (y2 - clip_region.bottom), (y2 - y1));
                y2 = clip_region.bottom;
            }
        }
        else {
            if (y1 > clip_region.bottom) {
                if (y2 > clip_region.bottom)
                    return;
				x1 -= MULDIV((x1 - x2), (y1 - clip_region.bottom), (y1 - y2));
                y1 = clip_region.bottom;
            }
            if (y2 < clip_region.top) {
                if (y1 < clip_region.top)
                    return;
				x2 += MULDIV((x1 - x2), (clip_region.top - y2), (y1 - y2));
                y2 = clip_region.top;
            }
        }
       	line_prim(x1, y1, x2, y2, COL(col));
    }
    else {
        if (x1 > clip_region.right) {
            if (x2 > clip_region.right)
                return;
			y1 -= MULDIV((y1 - y2), (x1 - clip_region.right), (x1 - x2));
            x1 = clip_region.right;
        }
        if (x2 < clip_region.left) {
            if (x1 < clip_region.left)
                return;
			y2 += MULDIV((y1 - y2), (clip_region.left - x2), (x1 - x2));
            x2 = clip_region.left;
        }
        if (y2 >= y1) {
            if (y1 < clip_region.top) {
                if (y2 < clip_region.top)
                    return;
				x1 += MULDIV((x2 - x1), (clip_region.top - y1), (y2 - y1));
                y1 = clip_region.top;
            }
            if (y2 > clip_region.bottom) {
                if (y1 > clip_region.bottom)
                    return;
				x2 -= MULDIV((x2 - x1), (y2 - clip_region.bottom), (y2 - y1));
                y2 = clip_region.bottom;
            }
        }
        else {
            if (y1 > clip_region.bottom) {
                if (y2 > clip_region.bottom)
                    return;
				x1 -= MULDIV((x1 - x2), (y1 - clip_region.bottom), (y1 - y2));
                y1 = clip_region.bottom;
            }
            if (y2 < clip_region.top) {
                if (y1 < clip_region.top)
                    return;
				x2 += MULDIV((x1 - x2), (clip_region.top - y2), (y1 - y2));
                y2 = clip_region.top;
            }
        }
       	line_prim(x2, y2, x1, y1, COL(col));
    }
}

