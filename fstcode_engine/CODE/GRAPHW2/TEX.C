/* Textured polygon drawing */

#include <stdlib.h>
#include "graph.h"

static int total_textures = 0;
static int used_textures = 0;
Texture **textures = NULL;

extern int divtab[];

static int line_stack[490];
static float zline_stack[490];
static int tx_stack[490], ty_stack[490];
static float ftx_stack[490], fty_stack[490];

void raw_tex_poly(int npts, sVector2 pts[], sVector2 ti[], Texture *tex)
{
	int x, y, tx, ty;
	int32 big_x, gradient;
	int32 big_tx, tx_grad;
	int32 big_ty, ty_grad;
	int i, stacking, dy, ady, last_dy, sdy;
	int dx, last_x;
	int *line_stack_ptr;
	int32 *tx_stack_ptr, *ty_stack_ptr;

	line_stack_ptr = line_stack;
	tx_stack_ptr = tx_stack;
	ty_stack_ptr = ty_stack;
	x = pts[npts - 1].x;
	y = pts[npts - 1].y;
	tx = ti[npts - 1].x;
	ty = ti[npts - 1].y;
	stacking = TRUE;
	last_dy = 0;
	for (i = 0; i < npts; i++) {
/*		vector_number(x, 240, (i * 8) + 6, 15);
		vector_number(y, 260, (i * 8) + 6, 15);
		vector_number(tx, 280, (i * 8) + 6, 15);
		vector_number(ty, 300, (i * 8) + 6, 15);*/
		dy = pts[i].y - y;
		if (dy != 0) {
			if (dy > 0) {
				if (last_dy < 0) {
					*line_stack_ptr++ = last_x;
					*tx_stack_ptr++ = big_tx;
					*ty_stack_ptr++ = big_ty;
					stacking = FALSE;
				}
				last_dy = dy;
				ady = dy;
				sdy = 1;
			} else {
				if (last_dy > 0) {
					*line_stack_ptr++ = last_x;
					*tx_stack_ptr++ = big_tx;
					*ty_stack_ptr++ = big_ty;
					stacking = FALSE;
				}
				last_dy = dy;
				ady = -dy;
				sdy = -1;
			}
			big_tx = (int32)tx << 16;
			big_ty = (int32)ty << 16;
			tx_grad = divtab[ady] * (ti[i].x - tx);
			ty_grad = divtab[ady] * (ti[i].y - ty);
			dx = pts[i].x - x;
			gradient = divtab[ady] * dx;
			big_x = ((int32)x << 16) + 0x8000L;
			if (dy > 0) {
				if (dx > 0) {
					big_x += gradient;
					x = (int)(big_x >> 16);
/*					big_tx += tx_grad;
					big_ty += ty_grad;*/
				}
			} else {
				if (dx < 0) {
					big_x += gradient;
					x = (int)(big_x >> 16);
/*					big_tx += tx_grad;
					big_ty += ty_grad;*/
				}
			}
			if (stacking) {
				while (ady-- > 0) {
					*line_stack_ptr++ = x;
					*tx_stack_ptr++ = big_tx;
					*ty_stack_ptr++ = big_ty;
					big_tx += tx_grad;
					big_ty += ty_grad;
					y += sdy;
					big_x += gradient;
					x = (int)(big_x >> 16);
				}
			} else {
				while (ady-- > 0) {
					tex_hline_prim(x, *(--line_stack_ptr), y, big_tx, *(--tx_stack_ptr), big_ty, *(--ty_stack_ptr), tex->width, tex->map);
					big_tx += tx_grad;
					big_ty += ty_grad;
					y += sdy;
					big_x += gradient;
					x = (int)(big_x >> 16);
					if (line_stack_ptr == line_stack) {
						stacking = TRUE;
						while (ady-- > 0) {
							*line_stack_ptr++ = x;
							*tx_stack_ptr++ = big_tx;
							*ty_stack_ptr++ = big_ty;
							big_tx += tx_grad;
							big_ty += ty_grad;
							y += sdy;
							big_x += gradient;
							x = (int)(big_x >> 16);
						}
						break;
					}
				}
			}
			last_x = x;
		}
		x = pts[i].x;
		y = pts[i].y;
		tx = ti[i].x;
		ty = ti[i].y;
    }
	if (line_stack_ptr > line_stack) {
		tex_hline_prim(last_x, *(--line_stack_ptr), y, big_tx, *(--tx_stack_ptr), big_ty, *(--ty_stack_ptr), tex->width, tex->map);
	}
}

// Perspective correct version
void raw_pctex_poly(int npts, sVector2 pts[], int *zpts, sVector2 ti[], Texture *tex)
{
	int x, y;
	int32 big_x, gradient;
	float z, big_z, z_gradient, dz, last_z;
	float tx, ty, tx_grad, ty_grad;
	int i, stacking, dy, ady, last_dy, sdy;
	int dx, last_x;
	int *line_stack_ptr;
	float *zline_stack_ptr;
	float *tx_stack_ptr, *ty_stack_ptr, adyr;
	sVectorFloat	fti [MAX_PTS];	// Floating point texture co-ordinates and screen z

	config_fpu_cw ();	// Configure fpu for round down single-precision

	// Perspective project final z and texture coordinates
	{
		if (zpts[npts - 1] != 0)
			fti[npts - 1].z = 1.0f / ITOF(zpts[npts - 1]>>SSHIFT);
		else
			fti[i].z = 0.0f;

		fti[npts - 1].x = ITOF(ti[npts - 1].x) * fti[npts - 1].z;
		fti[npts - 1].y = ITOF(ti[npts - 1].y) * fti[npts - 1].z;
	}
	
	line_stack_ptr = line_stack;
	zline_stack_ptr = zline_stack;
	tx_stack_ptr = ftx_stack;
	ty_stack_ptr = fty_stack;
	x = pts[npts - 1].x;
	y = pts[npts - 1].y;
	z = fti[npts - 1].z;
	tx = fti[npts - 1].x;
	ty = fti[npts - 1].y;
	stacking = TRUE;
	last_dy = 0;
	for (i = 0; i < npts; i++) {
		
		// Perspective project z and texture coordinates
		if (i < (npts - 1))
		{
			if (zpts[i] != 0)
				fti[i].z = 1.0f / ITOF(zpts[i]>>SSHIFT);
			else
				fti[i].z = 0.0f;

			fti[i].x = ITOF(ti[i].x) * fti[i].z;
			fti[i].y = ITOF(ti[i].y) * fti[i].z;
		}
		
		dy = pts[i].y - y;
		if (dy != 0) {
			if (dy > 0) {
				if (last_dy < 0) {
					*line_stack_ptr++ = last_x;
					*zline_stack_ptr++ = last_z;
					*tx_stack_ptr++ = tx;
					*ty_stack_ptr++ = ty;
					stacking = FALSE;
				}
				last_dy = dy;
				ady = dy;
				sdy = 1;
			} else {
				if (last_dy > 0) {
					*line_stack_ptr++ = last_x;
					*zline_stack_ptr++ = last_z;
					*tx_stack_ptr++ = tx;
					*ty_stack_ptr++ = ty;
					stacking = FALSE;
				}
				last_dy = dy;
				ady = -dy;
				sdy = -1;
			}
			
			adyr = 1.0f / ITOF(ady);
			tx_grad = (fti[i].x - tx) * adyr;
			ty_grad = (fti[i].y - ty) * adyr;

			dx = pts[i].x - x;
			gradient = divtab[ady] * dx;
			big_x = ((int32)x << 16) + 0x8000L;
			dz = fti[i].z - z;
			z_gradient = dz * adyr;
			big_z = z;
			if (dy > 0) {
				if (dx > 0) {
					big_x += gradient;
					x = (int)(big_x >> 16);
					big_z += z_gradient;
					z = big_z;
				}
			} else {
				if (dx < 0) {
					big_x += gradient;
					x = (int)(big_x >> 16);
					big_z += z_gradient;
					z = big_z;
				}
			}
			if (stacking) {
				while (ady-- > 0) {
					*line_stack_ptr++ = x;
					*zline_stack_ptr++ = z;
					*tx_stack_ptr++ = tx;
					*ty_stack_ptr++ = ty;
					tx += tx_grad;
					ty += ty_grad;
					y += sdy;
					big_x += gradient;
					x = (int)(big_x >> 16);
					big_z += z_gradient;
					z = big_z;
				}
			} else {
				while (ady-- > 0) {
					pctex_hline_prim(x, *(--line_stack_ptr), z, *(--zline_stack_ptr), y, tx, *(--tx_stack_ptr), ty, *(--ty_stack_ptr), tex->width, tex->map);
					tx += tx_grad;
					ty += ty_grad;
					y += sdy;
					big_x += gradient;
					x = (int)(big_x >> 16);
					big_z += z_gradient;
					z = big_z;
					if (line_stack_ptr == line_stack) {
						stacking = TRUE;
						while (ady-- > 0) {
							*line_stack_ptr++ = x;
							*zline_stack_ptr++ = z;
							*tx_stack_ptr++ = tx;
							*ty_stack_ptr++ = ty;
							tx += tx_grad;
							ty += ty_grad;
							y += sdy;
							big_x += gradient;
							x = (int)(big_x >> 16);
							big_z += z_gradient;
							z = big_z;
						}
						break;
					}
				}
			}
			last_x = x;
			last_z = z;
		}
		x = pts[i].x;
		y = pts[i].y;
		z = fti[i].z;
		tx = fti[i].x;
		ty = fti[i].y;
    }
	if (line_stack_ptr > line_stack) {
		pctex_hline_prim(last_x, *(--line_stack_ptr), last_z, *(--zline_stack_ptr), y, tx, *(--tx_stack_ptr), ty, *(--ty_stack_ptr), tex->width, tex->map);
	}

	restore_fpu_cw ();	// Restore previous fpu state (just in case!)
}

// Perspective correct poly
void pctex_poly_prim(int npts, sVector2 pts[], int *zpts, int tex_num)
{
	Texture *tex;
	tex = textures[tex_num];
	raw_pctex_poly(npts, pts, zpts, &tex->pts, tex);
}

void tex_poly_prim(int npts, sVector2 pts[], int tex_num)
{
	Texture *tex;
	tex = textures[tex_num];
	raw_tex_poly(npts, pts, &tex->pts, tex);
}

int clip_xedge_t(sVector2 *s1, sVector2 *s2, sVector2 *clip, sVector2 *t1, sVector2 *t2, sVector2 *tclip)
{
    register int n = 1, d, dx;

    if (s2->x >= s1->x) {
        if (s1->x < clip_region.left) {
            if (s2->x <= clip_region.left)
                return 0;
			dx = s2->x - s1->x;
			d = clip_region.left - s1->x;
			clip->y = s1->y + MULDIV(s2->y - s1->y, d, dx);
            clip->x = clip_region.left;
			tclip->x = t1->x + MULDIV(t2->x - t1->x, d, dx);
			tclip->y = t1->y + MULDIV(t2->y - t1->y, d, dx);
        } else {
            *clip = *s1;
            *tclip = *t1;
        }
        if (s2->x > clip_region.right) {
            if (s1->x > clip_region.right)
                return 0;
            if (s1->x == clip_region.right)
                return 1;
			clip++;
			tclip++;
			dx = s2->x - s1->x;
			d = s2->x - clip_region.right;
			clip->y = s2->y - MULDIV(s2->y - s1->y, d, dx);
            clip->x = clip_region.right;
			tclip->x = t2->x - MULDIV(t2->x - t1->x, d, dx);
			tclip->y = t2->y - MULDIV(t2->y - t1->y, d, dx);
            n = 2;
        }
        return n;
    } else {
        if (s1->x > clip_region.right) {
            if (s2->x >= clip_region.right)
                return 0;
			dx = s1->x - s2->x;
			d = s1->x - clip_region.right;
			clip->y = s1->y - MULDIV(s1->y - s2->y, d, dx);
            clip->x = clip_region.right;
			tclip->x = t1->x - MULDIV(t1->x - t2->x, d, dx);
			tclip->y = t1->y - MULDIV(t1->y - t2->y, d, dx);
        } else {
            *clip = *s1;
            *tclip = *t1;
        }
        if (s2->x < clip_region.left) {
            if (s1->x < clip_region.left)
                return 0;
            if (s1->x == clip_region.left)
                return 1;
			clip++;
			tclip++;
			dx = s1->x - s2->x;
			d = clip_region.left - s2->x;
			clip->y = s2->y + MULDIV(s1->y - s2->y, d, dx);
            clip->x = clip_region.left;
			tclip->x = t2->x + MULDIV(t1->x - t2->x, d, dx);
			tclip->y = t2->y + MULDIV(t1->y - t2->y, d, dx);
            n = 2;
        }
        return n;
    }
}

int clip_yedge_t(sVector2 *s1, sVector2 *s2, sVector2 *clip, sVector2 *t1, sVector2 *t2, sVector2 *tclip)
{
    register int n = 1, d, dy;

    if (s2->y >= s1->y) {
        if (s1->y < clip_region.top) {
            if (s2->y <= clip_region.top)
                return 0;
			dy = s2->y - s1->y;
			d = clip_region.top - s1->y;
			clip->x = s1->x + MULDIV(s2->x - s1->x, d, dy);
			clip->y = clip_region.top;
			tclip->x = t1->x + MULDIV(t2->x - t1->x, d, dy);
			tclip->y = t1->y + MULDIV(t2->y - t1->y, d, dy);
        } else {
            *clip = *s1;
            *tclip = *t1;
        }
        if (s2->y > clip_region.bottom) {
            if (s1->y > clip_region.bottom)
                return 0;
            if (s1->y == clip_region.bottom)
                return 1;
			clip++;
			tclip++;
			dy = s2->y - s1->y;
			d = s2->y - clip_region.bottom;
			clip->x = s2->x - MULDIV(s2->x - s1->x, d, dy);
            clip->y = clip_region.bottom;
			tclip->x = t2->x - MULDIV(t2->x - t1->x, d, dy);
			tclip->y = t2->y - MULDIV(t2->y - t1->y, d, dy);
            n = 2;
        }
        return n;
    } else {
        if (s1->y > clip_region.bottom) {
            if (s2->y >= clip_region.bottom)
                return 0;
			dy = s1->y - s2->y;
			d = s1->y - clip_region.bottom;
			clip->x = s1->x - MULDIV(s1->x - s2->x, d, dy);
            clip->y = clip_region.bottom;
			tclip->x = t1->x - MULDIV(t1->x - t2->x, d, dy);
			tclip->y = t1->y - MULDIV(t1->y - t2->y, d, dy);
        } else {
            *clip = *s1;
            *tclip = *t1;
        }
        if (s2->y < clip_region.top) {
            if (s1->y < clip_region.top)
                return 0;
            if (s1->y == clip_region.top)
                return 1;
			clip++;
			tclip++;
			dy = s1->y - s2->y;
			d = clip_region.top - s2->y;
			clip->x = s2->x + MULDIV(s1->x - s2->x, d, dy);
            clip->y = clip_region.top;
			tclip->x = t2->x + MULDIV(t1->x - t2->x, d, dy);
			tclip->y = t2->y + MULDIV(t1->y - t2->y, d, dy);
            n = 2;
        }
        return n;
    }
}

// Perspective correct version clips z's too
int clip_xedge_pct(sVector2 *s1, sVector2 *s2, sVector2 *clip, sVector2 *t1, sVector2 *t2, sVector2 *tclip, int *z1, int *z2, int *zclip)
{
    register int n = 1, d, dx;

    if (s2->x >= s1->x) {
        if (s1->x < clip_region.left) {
            if (s2->x <= clip_region.left)
                return 0;
			dx = s2->x - s1->x;
			d = clip_region.left - s1->x;
			clip->y = s1->y + MULDIV(s2->y - s1->y, d, dx);
            clip->x = clip_region.left;
			tclip->x = t1->x + MULDIV(t2->x - t1->x, d, dx);
			tclip->y = t1->y + MULDIV(t2->y - t1->y, d, dx);
			*zclip = *z1 + MULDIV(*z2 - *z1, d, dx);
        } else {
            *clip = *s1;
            *tclip = *t1;
			*zclip = *z1;
        }
        if (s2->x > clip_region.right) {
            if (s1->x > clip_region.right)
                return 0;
            if (s1->x == clip_region.right)
                return 1;
			clip++;
			tclip++;
			zclip++;
			dx = s2->x - s1->x;
			d = s2->x - clip_region.right;
			clip->y = s2->y - MULDIV(s2->y - s1->y, d, dx);
            clip->x = clip_region.right;
			tclip->x = t2->x - MULDIV(t2->x - t1->x, d, dx);
			tclip->y = t2->y - MULDIV(t2->y - t1->y, d, dx);
			*zclip = *z2 - MULDIV(*z2 - *z1, d, dx);
            n = 2;
        }
        return n;
    } else {
        if (s1->x > clip_region.right) {
            if (s2->x >= clip_region.right)
                return 0;
			dx = s1->x - s2->x;
			d = s1->x - clip_region.right;
			clip->y = s1->y - MULDIV(s1->y - s2->y, d, dx);
            clip->x = clip_region.right;
			tclip->x = t1->x - MULDIV(t1->x - t2->x, d, dx);
			tclip->y = t1->y - MULDIV(t1->y - t2->y, d, dx);
			*zclip = *z1 - MULDIV(*z1 - *z2, d, dx);
        } else {
            *clip = *s1;
            *tclip = *t1;
			*zclip = *z1;
        }
        if (s2->x < clip_region.left) {
            if (s1->x < clip_region.left)
                return 0;
            if (s1->x == clip_region.left)
                return 1;
			clip++;
			tclip++;
			dx = s1->x - s2->x;
			d = clip_region.left - s2->x;
			clip->y = s2->y + MULDIV(s1->y - s2->y, d, dx);
            clip->x = clip_region.left;
			tclip->x = t2->x + MULDIV(t1->x - t2->x, d, dx);
			tclip->y = t2->y + MULDIV(t1->y - t2->y, d, dx);
			*zclip = *z2 + MULDIV(*z1 - *z2, d, dx);
            n = 2;
        }
        return n;
    }
}

// Perspective correct version clips z's too
int clip_yedge_pct(sVector2 *s1, sVector2 *s2, sVector2 *clip, sVector2 *t1, sVector2 *t2, sVector2 *tclip, int *z1, int *z2, int *zclip)
{
    register int n = 1, d, dy;

    if (s2->y >= s1->y) {
        if (s1->y < clip_region.top) {
            if (s2->y <= clip_region.top)
                return 0;
			dy = s2->y - s1->y;
			d = clip_region.top - s1->y;
			clip->x = s1->x + MULDIV(s2->x - s1->x, d, dy);
			clip->y = clip_region.top;
			tclip->x = t1->x + MULDIV(t2->x - t1->x, d, dy);
			tclip->y = t1->y + MULDIV(t2->y - t1->y, d, dy);
			*zclip = *z1 + MULDIV(*z2 - *z1, d, dy);
        } else {
            *clip = *s1;
            *tclip = *t1;
			*zclip = *z1;
        }
        if (s2->y > clip_region.bottom) {
            if (s1->y > clip_region.bottom)
                return 0;
            if (s1->y == clip_region.bottom)
                return 1;
			clip++;
			tclip++;
			zclip++;
			dy = s2->y - s1->y;
			d = s2->y - clip_region.bottom;
			clip->x = s2->x - MULDIV(s2->x - s1->x, d, dy);
            clip->y = clip_region.bottom;
			tclip->x = t2->x - MULDIV(t2->x - t1->x, d, dy);
			tclip->y = t2->y - MULDIV(t2->y - t1->y, d, dy);
			*zclip = *z2 - MULDIV(*z2 - *z1, d, dy);
            n = 2;
        }
        return n;
    } else {
        if (s1->y > clip_region.bottom) {
            if (s2->y >= clip_region.bottom)
                return 0;
			dy = s1->y - s2->y;
			d = s1->y - clip_region.bottom;
			clip->x = s1->x - MULDIV(s1->x - s2->x, d, dy);
            clip->y = clip_region.bottom;
			tclip->x = t1->x - MULDIV(t1->x - t2->x, d, dy);
			tclip->y = t1->y - MULDIV(t1->y - t2->y, d, dy);
			*zclip = *z1 - MULDIV(*z1 - *z2, d, dy);
        } else {
            *clip = *s1;
            *tclip = *t1;
			*zclip = *z1;
        }
        if (s2->y < clip_region.top) {
            if (s1->y < clip_region.top)
                return 0;
            if (s1->y == clip_region.top)
                return 1;
			clip++;
			tclip++;
			zclip++;
			dy = s1->y - s2->y;
			d = clip_region.top - s2->y;
			clip->x = s2->x + MULDIV(s1->x - s2->x, d, dy);
            clip->y = clip_region.top;
			tclip->x = t2->x + MULDIV(t1->x - t2->x, d, dy);
			tclip->y = t2->y + MULDIV(t1->y - t2->y, d, dy);
			*zclip = *z2 + MULDIV(*z1 - *z2, d, dy);
            n = 2;
        }
        return n;
    }
}


/* Points must be in clockwise order starting at top left */

void tex_polygon(int npts, sVector2 pts[], int tex_num)
{
    int i, j, n1, n;
	Texture *tex;
    sVector2 xclip[MAX_PTS], clip[MAX_PTS];
	sVector2 txclip[MAX_PTS], tclip[MAX_PTS];

	tex = textures[tex_num];
	for (i = 0, j = 1, n1 = 0; i < npts; i++, j++) {
		if (j == npts) j = 0;
        n1 += clip_xedge_t(&pts[i], &pts[j], &xclip[n1], &tex->pts[i], &tex->pts[j], &txclip[n1]);
    }
    for (i = 0, j = 1, n = 0; i < n1; i++, j++) {
		if (j == n1) j = 0;
        n += clip_yedge_t(&xclip[i], &xclip[j], &clip[n], &txclip[i], &txclip[j], &tclip[n]);
    }
	if (n > 2)
		raw_tex_poly(n, clip, tclip, tex);
}

// Perspective correct poly
void pctex_polygon(int npts, sVector2 pts[], int *zpts, int tex_num)
{
    int i, j, n1, n;
	Texture *tex;
    sVector2 xclip[MAX_PTS], clip[MAX_PTS];
	sVector2 txclip[MAX_PTS], tclip[MAX_PTS];
	int	xclipz[MAX_PTS], zclip[MAX_PTS];

	tex = textures[tex_num];
	for (i = 0, j = 1, n1 = 0; i < npts; i++, j++) {
		if (j == npts) j = 0;
        n1 += clip_xedge_pct(&pts[i], &pts[j], &xclip[n1], &tex->pts[i], &tex->pts[j], &txclip[n1], &zpts[i], &zpts[j], &xclipz[n1]);
    }
    for (i = 0, j = 1, n = 0; i < n1; i++, j++) {
		if (j == n1) j = 0;
        n += clip_yedge_pct(&xclip[i], &xclip[j], &clip[n], &txclip[i], &txclip[j], &tclip[n], &xclipz[i], &xclipz[j], &zclip[n]);
    }
	
	if (n > 2)
		raw_pctex_poly(n, clip, zclip, tclip, tex);
}


#define NEAR_CLIP (64L << SSHIFT)

static Cache_pt clip_pts[16];
static Cache_pt *clip_pt = clip_pts;
static sVector2 tclip_pts[16];
static sVector2 *tclip_pt = tclip_pts;

static int z_clip(Cache_pt **pp1, Cache_pt **pp2, sVector2 **pt1, sVector2 **pt2)
{
    Cache_pt *p1, *p2;
	sVector2 *t1, *t2;
    sVector n1, n2, n3;
    sVector2 s1, s2, s3;

	p1 = *pp1;
	p2 = *pp2;
	t1 = *pt1;
	t2 = *pt2;
	if (p2->z >= p1->z) {
        if (p1->z >= NEAR_CLIP)
            return 1;
        if (p2->z < NEAR_CLIP)
            return 0;
        if (p2->z == NEAR_CLIP) {
			p1 = clip_pt++;
			t1 = tclip_pt++;
            p1->x = p2->x;
            p1->y = p2->y;
        	p1->z = NEAR_CLIP;
            t1->x = t2->x;
            t1->y = t2->y;
			*pp1 = p1;
			*pt1 = t1;
        	return 1;
        }
    	n1 = *((sVector *)p1);
    	n2 = *((sVector *)p2);
    	s1 = *t1;
    	s2 = *t2;
    	while (n1.z < NEAR_CLIP) {
        	n3.x = (n1.x + n2.x) >> 1;
        	n3.y = (n1.y + n2.y) >> 1;
        	n3.z = (n1.z + n2.z) >> 1;
        	s3.x = (s1.x + s2.x) >> 1;
        	s3.y = (s1.y + s2.y) >> 1;
        	if (n3.z <= NEAR_CLIP) {
            	n1 = n3;
            	s1 = s3;
        	} else {
            	n2 = n3;
            	s2 = s3;
			}
    	}
		p1 = clip_pt++;
		t1 = tclip_pt++;
    	p1->x = n1.x;
    	p1->y = n1.y;
    	p1->z = NEAR_CLIP;
    	t1->x = s1.x;
    	t1->y = s1.y;
		*pp1 = p1;
		*pt1 = t1;
    	return 1;
    } else {
        if (p2->z >= NEAR_CLIP)
            return 1;
        if (p1->z < NEAR_CLIP)
            return 0;
        if (p1->z == NEAR_CLIP) {
			p2 = clip_pt++;
			t2 = tclip_pt++;
            p2->x = p1->x;
            p2->y = p1->y;
        	p2->z = NEAR_CLIP;
            t2->x = t1->x;
            t2->y = t1->y;
			*pp2 = p2;
			*pt2 = t2;
        	return 2;
        }
    	n1 = *((sVector *)p2);
    	n2 = *((sVector *)p1);
		s1 = *t2;
		s2 = *t1;
    	while (n1.z < NEAR_CLIP) {
        	n3.x = (n1.x + n2.x) >> 1;
        	n3.y = (n1.y + n2.y) >> 1;
        	n3.z = (n1.z + n2.z) >> 1;
        	s3.x = (s1.x + s2.x) >> 1;
        	s3.y = (s1.y + s2.y) >> 1;
        	if (n3.z <= NEAR_CLIP) {
            	n1 = n3;
            	s1 = s3;
        	} else {
            	n2 = n3;
            	s2 = s3;
			}
    	}
		p2 = clip_pt++;
		t2 = tclip_pt++;
    	p2->x = n1.x;
    	p2->y = n1.y;
    	p2->z = NEAR_CLIP;
    	t2->x = s1.x;
    	t2->y = s1.y;
		*pp2 = p2;
		*pt2 = t2;
    	return 2;
	}
}

static void outline_polygon(int npts, sVector2 *s, int col)
{
	int i, j;

    for (i = 0, j = 1; i < npts; i++, j++) {
		if (j == npts) j = 0;
        line(s[i].x, s[i].y, s[j].x, s[j].y, col);
    }
}

void clipped_tex_polygon(sPolygon *poly, Cache_pt *tpts)
{
    int i, j, c, n, n1, npts;
	Pindex *points;
    int32 zf;
	Texture *tex;
    Cache_pt *p1, *p2, *np;
    sVector2 *t1, *t2, spts[MAX_PTS];
    sVector2 xclip[MAX_PTS], clip[MAX_PTS];
	sVector2 tzclip[MAX_PTS], txclip[MAX_PTS], tclip[MAX_PTS];

	clip_pt = clip_pts;
	tclip_pt = tclip_pts;
	tex = textures[poly->colour - 256];
	npts = poly->npoints;
	points = poly->points;
   	np = &tpts[points[0]];
	for (i = 0, j = 1, n = 0; i < npts; i++, j++) {
		if (j == npts) j = 0;
        p1 = np;
		np = &tpts[points[j]];
        p2 = np;
		t1 = &tex->pts[i];
		t2 = &tex->pts[j];
        c = z_clip(&p1, &p2, &t1, &t2);
		if (c > 0) {
            zf = p1->z >> pshift;
            spts[n].x = (int)(xcentre + (p1->x / zf));
            spts[n].y = (int)(ycentre - (p1->y / zf));
			tzclip[n].x = t1->x;
			tzclip[n++].y = t1->y;
/*			vector_number(p1->x >> SSHIFT, 10+(n*30), 6, 15);
			vector_number(p1->y >> SSHIFT, 10+(n*30), 12, 15);
			vector_number(p1->z >> SSHIFT, 10+(n*30), 18, 15);
			vector_number(spts[n-1].x, 10+(n*30), 46, 15);
			vector_number(spts[n-1].y, 10+(n*30), 52, 15);
			vector_number(tzclip[n-1].x, 10+(n*30), 60, 15);
			vector_number(tzclip[n-1].y, 10+(n*30), 66, 15);*/
            if (c > 1) {
                if (p2->z != np->z) {
                	zf = p2->z >> pshift;
                  	spts[n].x = (int)(xcentre + (p2->x / zf));
                   	spts[n].y = (int)(ycentre - (p2->y / zf));
					tzclip[n].x = t2->x;
					tzclip[n++].y = t2->y;
/*					vector_number(p2->x >> SSHIFT, 10+(n*30), 26, 15);
					vector_number(p2->y >> SSHIFT, 10+(n*30), 32, 15);
					vector_number(p2->z >> SSHIFT, 10+(n*30), 38, 15);
					vector_number(spts[n-1].x, 10+(n*30), 46, 15);
					vector_number(spts[n-1].y, 10+(n*30), 52, 15);
					vector_number(tzclip[n-1].x, 10+(n*30), 60, 15);
					vector_number(tzclip[n-1].y, 10+(n*30), 66, 15);*/
                }
            }
        }
    }
    if ((n > 2) && (n <= 4)) {
/*		for (i = 0; i < n; i++) {
			vector_number(spts[i].x, 170+(i*25), 6, 15);
			vector_number(spts[i].y, 170+(i*25), 12, 15);
			vector_number(tzclip[i].x, 170+(i*25), 20, 15);
			vector_number(tzclip[i].y, 170+(i*25), 26, 15);
    	}*/
		for (i = 0, j = 1, n1 = 0; i < n; i++, j++) {
			if (j == n) j = 0;
        	n1 += clip_xedge_t(&spts[i], &spts[j], &xclip[n1], &tzclip[i], &tzclip[j], &txclip[n1]);
    	}
/*		for (i = 0; i < n1; i++) {
			vector_number(xclip[i].x, 170+(i*25), 34, 15);
			vector_number(xclip[i].y, 170+(i*25), 40, 15);
			vector_number(txclip[i].x, 170+(i*25), 48, 15);
			vector_number(txclip[i].y, 170+(i*25), 54, 15);
    	}*/
    	for (i = 0, j = 1, n = 0; i < n1; i++, j++) {
			if (j == n1) j = 0;
        	n += clip_yedge_t(&xclip[i], &xclip[j], &clip[n], &txclip[i], &txclip[j], &tclip[n]);
    	}
		if (n > 2) {
			raw_tex_poly(n, clip, tclip, tex);
/*			outline_polygon(n, clip, 0x10);*/
		}
	}
}

//Perspective correct version of clipped_tex_poly()
void clipped_pctex_polygon(sPolygon *poly, Cache_pt *tpts)
{
    int i, j, c, n, n1, npts;
	Pindex *points;
    int32 zf;
	Texture *tex;
    Cache_pt *p1, *p2, *np;
    sVector2 *t1, *t2, spts[MAX_PTS];
    sVector2 xclip[MAX_PTS], clip[MAX_PTS];
	sVector2 tzclip[MAX_PTS], txclip[MAX_PTS], tclip[MAX_PTS];
	int	zpts[MAX_PTS], xclipz[MAX_PTS], zclip[MAX_PTS];

	clip_pt = clip_pts;
	tclip_pt = tclip_pts;
	tex = textures[poly->colour - 256];
	npts = poly->npoints;
	points = poly->points;
   	np = &tpts[points[0]];
	for (i = 0, j = 1, n = 0; i < npts; i++, j++) {
		
 		if (j == npts) j = 0;
        p1 = np;
		np = &tpts[points[j]];
        p2 = np;
		t1 = &tex->pts[i];
		t2 = &tex->pts[j];
        c = z_clip(&p1, &p2, &t1, &t2);
 		
        if (c > 0) {
            zf = p1->z >> pshift;
            spts[n].x = (int)(xcentre + (p1->x / zf));
            spts[n].y = (int)(ycentre - (p1->y / zf));
			zpts[n] = p1->z;
			tzclip[n].x = t1->x;
			tzclip[n++].y = t1->y;

            if (c > 1) {
                if (p2->z != np->z) {
                	zf = p2->z >> pshift;
                  	spts[n].x = (int)(xcentre + (p2->x / zf));
                   	spts[n].y = (int)(ycentre - (p2->y / zf));
					zpts[n] = p2->z;
					tzclip[n].x = t2->x;
					tzclip[n++].y = t2->y;
                }
            }
        }
    }
	if ((n > 2) && (n <= 4)) {
		for (i = 0, j = 1, n1 = 0; i < n; i++, j++) {
			if (j == n) j = 0;
        	n1 += clip_xedge_pct (&spts[i], &spts[j], &xclip[n1], &tzclip[i], &tzclip[j], &txclip[n1], &zpts[i], &zpts[j], &xclipz[n1]);
    	}
    	for (i = 0, j = 1, n = 0; i < n1; i++, j++) {
			if (j == n1) j = 0;
        	n += clip_yedge_pct (&xclip[i], &xclip[j], &clip[n], &txclip[i], &txclip[j], &tclip[n], &xclipz[i], &xclipz[j], &zclip[n]);
    	}
		if (n > 2) {
			raw_pctex_poly(n, clip, zclip, tclip, tex);
		}
	}
}


/* Returns the colour number of the texture */
int add_texture(Texture *t)
{
	int i;
	Texture **old_textures;

	if (used_textures >= total_textures) {
		old_textures = textures;
		total_textures += 100;
		textures = (Texture **)calloc(total_textures, sizeof(Texture *));
		for (i = 0; i < used_textures; i++) {
			textures[i] = old_textures[i];
		}
		free(old_textures);
	}
	textures[used_textures] = t;
	return (used_textures++);
}