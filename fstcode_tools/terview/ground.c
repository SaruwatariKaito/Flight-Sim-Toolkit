/* FST Terrain Viewer */
/* Ground drawing functions */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "defs.h"

#define HEADER_SIZE 128
//#define SKY_BRUSH 0x2f
#define FRONT_BRUSH BLACK_BRUSH
#define FRONT_PEN BLACK_PEN

extern HWND main_window;

static Vector view_pos = {0L, -5000L, 10000L};
static Svector view_rot = {0, DEG(0), 0};
static int32 view_step = 1000L;
static Colour loaded_palette[256];
static HPALETTE view_palette = NULL, old_palette;
static HDC hdc_cache;
static HBITMAP hbm_cache, hbm_old_cache;

static int terrain_x_size = 0;
static int terrain_y_size = 0;
static int max_height = 15, min_height = 0;

static short *terrain_data = NULL;

//static int sort_needed = TRUE;
//static int x_start = 0, y_start = 0;

static int get_terrain_height(int tx, int ty);
static int update_required = TRUE;

//static char msg[80];

extern int BACK_BRUSH;
extern int back_color;

extern int current_tool;

extern int solid_polys;
extern int white_polys;

static void draw_view(HDC hdc);

static int hdg = 0;

int screen_width = 320, screen_height = 200;

Screen_clip clip_region = {0, 319, 0, 199};

HDC graph_hdc;

/*
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void toolbar_pressed(int wx, int wy, RECT *rect)
{
	int button;

	if (wx < 20) {
		button = (wy / 20) + 1;
		if (button < 4) {
			current_tool = button;
			switch (button) {

				case 1:
                    	solid_polys = TRUE;
                         white_polys = FALSE;
                         break;

				case 2:
                    	solid_polys = FALSE;
                         white_polys = TRUE;
                         break;

				case 3:
                    	solid_polys = FALSE;
                         white_polys = FALSE;
                         break;
			}
			update_required = TRUE;
		}
		else {
			rect->left = 20;
			switch (button) {

				case 4:
					if (view_pos.z > view_step) {
						view_pos.z -= view_step;
						update_required = TRUE;
					}
					break;

				case 5:
					view_pos.z += view_step;
					update_required = TRUE;
					break;
			}
		}
	}
}
*/

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void view_centered(int wx, int wy, RECT *rect, int right)
{
	int	sx, sy;

	rect = rect;

     sx = abs(xcentre - wx);
     sy = abs(ycentre - wy);

	if (wx + wy > 0) {
		if ((sx < 25) && (sy < 25)) {
			view_rot.x = 0;
			view_rot.y = 0;
			view_rot.z = 0;
          }
	}
	update_required = TRUE;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void view_clicked(int wx, int wy, RECT *rect, int right, int key)
{
	int turn_amount;
	rect = rect;

     if (key > 0) {
		turn_amount = DEG_1;
     }
     else {
		turn_amount = DEG(10);
     }

	if (wx + wy > 0) {
     	if (right) {
			// Rotate U/D
			if (wy < (ycentre / 2)) {
				view_rot.z -= turn_amount;
			}
			else if (wy > ((ycentre * 3) / 2)) {
				view_rot.z += turn_amount;
			}
          }
          else {
			// Rotate U/D
			if (wy < (ycentre / 2)) {
				view_rot.x -= turn_amount;
			}
			else if (wy > ((ycentre * 3) / 2)) {
				view_rot.x += turn_amount;
			}
          }

		// Rotate L/R
		if (wx < (xcentre / 2)) {
			view_rot.y -= turn_amount;
		}
		else if (wx > ((xcentre * 3) / 2)) {
               if (view_rot.y == 32760) {
	               view_rot.y = -30940;
               }
               else {
				view_rot.y += turn_amount;
          	}
		}
	}
	update_required = TRUE;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void mouse_moved(HWND hwnd, int wx, int wy)
{
	hwnd = hwnd; wx = wx; wy = wy;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void mouse_up(HWND hwnd, int wx, int wy, int right, int key)
{
	RECT rect;

	GetClientRect(hwnd, &rect);
	view_clicked(wx, wy, &rect, right, key);
	InvalidateRect(hwnd, &rect, FALSE);
	UpdateWindow(hwnd);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void mouse_down(HWND hwnd, int wx, int wy, int right)
{
	hwnd = hwnd;
	wx = wx;
     wy = wy;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void mouse_double_pressed(HWND hwnd, int wx, int wy, int right)
{
	RECT rect;

	GetClientRect(hwnd, &rect);
	view_centered(wx, wy, &rect, right);
	InvalidateRect(hwnd, &rect, FALSE);
	UpdateWindow(hwnd);
}

/*
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void draw_rect(HDC hdc, int x1, int y1, int x2, int y2)
{
	POINT pts[4];

	pts[0].x = x1;
	pts[0].y = y1;
	pts[1].x = x2;
	pts[1].y = y1;
	pts[2].x = x2;
	pts[2].y = y2;
	pts[3].x = x1;
	pts[3].y = y2;
	Polygon(hdc, pts, 4);
}
*/

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void redraw_client(HDC hdc, RECT *rect)
{
	int		width, height;
	HBRUSH	hbrush;
	HPEN 	light, dark, old;
	HCURSOR	hcursor;
	HPALETTE	old_pal;

	width  = rect->right  - rect->left;
	height = rect->bottom - rect->top;

	old_pal = SelectPalette(hdc, view_palette, 0);

	dark  = CreatePen(PS_SOLID, 1, RGB(128, 128, 128));
	light = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));

	RealizePalette(hdc);

	if (update_required) {

		hcursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

		switch (back_color) {
			case 2:
				hbrush = GetStockObject(WHITE_BRUSH);
				break;
			case 1:
				hbrush = CreateSolidBrush(PALETTEINDEX(16));	//GetStockObject(SKY_BRUSH);
				break;
			case 0:
				hbrush = GetStockObject(BLACK_BRUSH);
				break;
		}
		FillRect(hdc_cache, rect, hbrush);
		draw_view(hdc_cache);
		SetCursor(hcursor);
		update_required = FALSE;
	}

	BitBlt(hdc, rect->left, rect->top, width, height, hdc_cache, 0, 0, SRCCOPY);

	SelectObject(hdc, dark);
	MoveTo(hdc, rect->left ,rect->bottom);
	LineTo(hdc, rect->left ,rect->top   );
	LineTo(hdc, rect->right,rect->top   );

	SelectObject(hdc, light);
	MoveTo(hdc, rect->right - 1,rect->top       );
	LineTo(hdc, rect->right - 1,rect->bottom - 1);
	LineTo(hdc, rect->left - 1 ,rect->bottom - 1);

	SelectPalette(hdc, old_pal, 0);
	DeleteObject(dark);
	DeleteObject(light);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void create_client(HWND hwnd)
{
	HDC hdc;
	RECT rect;
	HRGN hrgn;
	int width, height;

	GetClientRect (hwnd, &rect);

	width  = rect.right - rect.left;
	height = rect.bottom - rect.top;

	hdc = GetDC(hwnd);
	hdc_cache = CreateCompatibleDC(hdc);

	hrgn = CreateRectRgn(rect.left, rect.top, rect.right, rect.bottom);
	SelectClipRgn(hdc_cache, hrgn);
	DeleteObject(hrgn);

	hbm_cache = CreateCompatibleBitmap(hdc, width, height);
	hbm_old_cache = SelectObject(hdc_cache, hbm_cache);

	old_palette = SelectPalette(hdc_cache, view_palette, 0);

	ReleaseDC(hwnd, hdc);
	update_required = TRUE;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void resize_client(HWND hwnd, int width, int height)
{
	HDC hdc;
	RECT rect;
	HRGN hrgn;

	screen_width	= width;
	screen_height	= height;
	xcentre		= screen_width  / 2;
	ycentre		= screen_height / 2;
	rect.left		= 0;
	rect.right	= width;
	rect.top		= 0;
	rect.bottom	= height;

	hdc = GetDC(hwnd);

	hrgn = CreateRectRgn(rect.left, rect.top, rect.right, rect.bottom);
	SelectClipRgn(hdc_cache, hrgn);
	DeleteObject(hrgn);

	SelectObject(hdc_cache, hbm_old_cache);
	DeleteObject(hbm_cache);

	hbm_cache = CreateCompatibleBitmap(hdc, width, height);

	SelectObject(hdc_cache, hbm_cache);
	SelectPalette(hdc_cache, view_palette, 0);

	ReleaseDC(hwnd, hdc);
	update_required = TRUE;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void draw_line(int x1, int y1, int x2, int y2, int col)
{
	HPEN pen, old_pen;
	int good_pen;

	pen = CreatePen(PS_SOLID, 1, PALETTEINDEX(col));
	if (pen != NULL) {
		good_pen = TRUE;
	}
	else {
		good_pen = FALSE;
		pen = GetStockObject(FRONT_PEN);
	}
	old_pen = SelectObject(graph_hdc, pen);
	MoveTo(graph_hdc, x1, y1);
	LineTo(graph_hdc, x2, y2);
	SelectObject(graph_hdc, old_pen);
	if (good_pen) {
		DeleteObject(pen);
	}
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void draw_polygon(int npts, Vector2 *pts, int col)
{
	HBRUSH	brush, old_brush;
	HPEN		pen, old_pen;

	int new_brush;
	int new_pen;

	if (white_polys) {
		brush = CreateSolidBrush(RGB( 255, 255, 255));
	}
     else {
		brush = CreateSolidBrush(PALETTEINDEX(col));
     }
	pen   = CreatePen(PS_SOLID, 1, RGB( 64, 64, 64));

	if (brush != NULL) {
		new_brush = TRUE;
		new_pen   = TRUE;
	}
	else {
		new_brush = FALSE;
		new_pen   = FALSE;
		brush = GetStockObject(FRONT_BRUSH);
	}
	old_brush = SelectObject(graph_hdc, brush);
	old_pen   = SelectObject(graph_hdc, pen);

	Polygon(graph_hdc, (LPPOINT)pts, npts);

	SelectObject(graph_hdc, old_brush);
	SelectObject(graph_hdc, old_pen);

	if (new_brush) {
		DeleteObject(brush);
		DeleteObject(pen);
	}
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void draw_square(int tx, int ty)
{
	int cx, cy, col, height;
	int i, j, zero_count;
	int h[4];
	Vector p;
	Vector2 spts[4], bspts[4];
     long int count;

	cx = terrain_x_size / 2;
	cy = terrain_y_size / 2;
	zero_count = 0;

	p.x = (tx - cx) * 500;
	p.z = (ty - cy) * 500;
	p.y = get_terrain_height(tx, ty);
	if ((h[0] = p.y) == 0) {
		zero_count++;
	}
	perspect(&p, &spts[0]);

	p.z += 500;
	p.y = get_terrain_height(tx, ty + 1);
	if ((h[1] = p.y) == 0) {
		zero_count++;
	}
	perspect(&p, &spts[1]);

	p.x += 500;
	p.y = get_terrain_height(tx + 1, ty + 1);
	if ((h[2] = p.y) == 0) {
		zero_count++;
	}
	perspect(&p, &spts[2]);

	p.z -= 500;
	p.y = get_terrain_height(tx + 1, ty);
	if ((h[3] = p.y) == 0) {
		zero_count++;
	}
	perspect(&p, &spts[3]);

	height = (h[0] + h[1] + h[2] + h[3]) >> 2;
	col = ((height - min_height) * 16) / (max_height - min_height);
	if (++col > 15) {
		col = 15;
	}

	if ((h[0] == h[1]) && (h[0] == h[2]) && (h[0] == h[3])) {
		if (h[0] == 0) {
			col = 0;
		}
		draw_polygon(4, spts, col);
	}
	else {
		if (zero_count == 3) {
			for (i = 0; (h[i] == 0) && (i < 4); i++)
				;
			if (--i < 0) {
				i = 3;
			}
			for (j = 0; j < 3; j++) {
				bspts[j] = spts[i];
				if (++i > 3) {
					i = 0;
				}
			}
			draw_polygon(3, bspts, col);
			if (--i < 0) {
				i = 3;
			}
			for (j = 0; j < 3; j++) {
				bspts[j] = spts[i];
				if (++i > 3) {
					i = 0;
				}
			}
			col = 0;
			draw_polygon(3, bspts, col);
		}
		else {
			draw_polygon(4, spts, col);
		}
	}
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void draw_ground(Vector *pos, Svector *rot)
{
	int tx, ty;

	push_matrix();
	rotx(DEG(-30));
	translate(pos->x, pos->y, pos->z);

	roty(-rot->y);
	rotx(-rot->x);
	rotz(-rot->z);

	//-----------------------          /\ X,Y
	// Quad Draw Orientations         /  \
	//-----------------------        /    \
	//  #1 000~045º = Y- X-         /N    E\
	//  #2 045~090º = X- Y-        /  \  /  \
	//  #3 090~135º = X- Y+   0,Y /    \/    \
	//  #4 135~180º = Y+ X-       \    /\    / X,0
	//  #5 180~225º = Y+ X+        \  /  \  /
	//  #6 225~270º = X+ Y+         \W    S/
     //  #7 270~315º = X+ Y-          \    /
	//  #8 315~360º = Y- X+           \  /
	//-----------------------      0,0 \/

	// Quad #1
	if (hdg >= 0 && hdg < 45) {
		for (ty = terrain_y_size - 2; ty >= 0; ty--) {
			for (tx = terrain_x_size - 2; tx >= 0; tx--) {
				draw_square(tx, ty);
			}
		}
	}
	// Quad #2
	else if (hdg >= 45 && hdg < 90) {
		for (tx = terrain_x_size - 2; tx >= 0; tx--) {
			for (ty = terrain_y_size - 2; ty >= 0; ty--) {
				draw_square(tx, ty);
			}
		}
	}
	// Quad #3
	else if (hdg >= 90 && hdg < 135) {
		for (tx = terrain_x_size - 2; tx >= 0; tx--) {
			for (ty = 0; ty <= terrain_y_size - 2; ty++) {
				draw_square(tx, ty);
			}
		}
	}
	// Quad #4
	else if (hdg >= 135 && hdg < 180) {
		for (ty = 0; ty <= terrain_y_size - 2; ty++) {
			for (tx = terrain_x_size - 2; tx >= 0; tx--) {
				draw_square(tx, ty);
			}
		}
	}
	// Quad #5
	else if (hdg >= 180 && hdg < 225) {
		for (ty = 0; ty <= terrain_y_size - 2; ty++) {
			for (tx = 0; tx <= terrain_x_size - 2; tx++) {
				draw_square(tx, ty);
			}
		}
	}
	// Quad #6
	else if (hdg >= 225 && hdg < 270) {
		for (tx = 0; tx <= terrain_x_size - 2; tx++) {
			for (ty = 0; ty <= terrain_y_size - 2; ty++) {
				draw_square(tx, ty);
			}
		}
	}
	// Quad #7
	else if (hdg >= 270 && hdg < 315) {
		for (tx = 0; tx <= terrain_x_size - 2; tx++) {
			for (ty = terrain_y_size - 2; ty >= 0; ty--) {
				draw_square(tx, ty);
			}
		}
	}
	// Quad #8
	else if (hdg >= 315 && hdg < 360) {
		for (ty = terrain_y_size - 2; ty >= 0; ty--) {
			for (tx = 0; tx <= terrain_x_size - 2; tx++) {
				draw_square(tx, ty);
			}
		}
	}
	pop_matrix();
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void draw_view(HDC hdc)
{
	HFONT	hfont, hfontold;
	char		s[40];
     RECT		rect;

	graph_hdc = hdc;

	if (solid_polys || white_polys) {
		clockwise_test = TRUE;
	}
	else {
		clockwise_test = FALSE;
	}

	hdg = (int)(view_rot.y / DEG_1);
	if (hdg < 0) {
		hdg = hdg + 360;
	}

	draw_ground(&view_pos, &view_rot);

	hfont	= CreateFont(-12,0,0,0,500,0,0,0,0,0,0,0,0,"Arial");
	hfontold	= SelectObject(hdc, hfont);

	switch (back_color) {

		case 2:	// White
			SetTextColor(hdc, RGB(0, 0, 0));
			break;

		case 1:	// Sky
			SetTextColor(hdc, RGB(0, 0, 128));
			break;

		case 0:	// Black
			SetTextColor(hdc, RGB(255,255,255));
			break;
	}

	rect.left		= 0;
	rect.right	= screen_width - 1;
	rect.top		= 0;
	rect.bottom	= screen_height - 1;

	SetBkMode(hdc, TRANSPARENT);
	sprintf(s, "ROT: %03d %d", hdg, view_rot.y);
	//sprintf(s, "HDG: %03dº", hdg);
    	DrawText (hdc, s, -1, &rect, DT_SINGLELINE | DT_CENTER | DT_TOP);

	SelectObject(hdc, hfontold);
	DeleteObject(hfont);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
static int load_colourmap(LOGPALETTE *pal, char *file_name)
{
	FILE *fp;
	int i, j, r, g, b, num_cols;
	num_cols = 256;
	pal = pal;
	fp = fopen(file_name,"r");
	if (fp != NULL) {
		fscanf(fp, "%d\n", &num_cols);
		for(i = 0; i < num_cols; i++) {
			fscanf(fp, "%d %x,%x,%x\n", &j, &r, &g, &b);
			cols[j] = j;
			loaded_palette[i].red = r;
			loaded_palette[i].green = g;
			loaded_palette[i].blue = b;
		}
		fclose(fp);
		return TRUE;
	}
	return FALSE;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void copy_colour(LOGPALETTE *pal, int n1, int n2)
{
	pal->palPalEntry[n1].peRed   = loaded_palette[n2].red;
	pal->palPalEntry[n1].peGreen = loaded_palette[n2].green;
	pal->palPalEntry[n1].peBlue  = loaded_palette[n2].blue;
	pal->palPalEntry[n1].peFlags = NULL;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void setup_colours(char *file_name)
{
	int i;
	LOGPALETTE *paldef;

	paldef = (LOGPALETTE *)malloc(sizeof(LOGPALETTE) + (18 * sizeof(PALETTEENTRY)));
	paldef->palVersion = 0x300;
	paldef->palNumEntries = 18;
	if (load_colourmap(paldef, file_name)) {
		// Ground color
		copy_colour(paldef,  0, 0x20);
		copy_colour(paldef,  1, 0x30);
		copy_colour(paldef,  2, 0x34);
		copy_colour(paldef,  3, 0x38);
		copy_colour(paldef,  4, 0x3c);
		copy_colour(paldef,  5, 0x40);
		copy_colour(paldef,  6, 0x44);
		copy_colour(paldef,  7, 0x48);
		copy_colour(paldef,  8, 0x4c);
		copy_colour(paldef,  9, 0x50);
		copy_colour(paldef, 10, 0x54);
		copy_colour(paldef, 11, 0x58);
		copy_colour(paldef, 12, 0x5c);
		copy_colour(paldef, 13, 0x60);
		copy_colour(paldef, 14, 0x64);
		copy_colour(paldef, 15, 0x68);
		// Sky color
     	copy_colour(paldef, 16, 0x2f);
		// Black color
     	copy_colour(paldef, 17, 0x1e);

	}
     else {
		for (i = 0; i < 16; i++) {
			paldef->palPalEntry[i].peRed = 0;
			paldef->palPalEntry[i].peGreen = 128 + (8 * i);
			paldef->palPalEntry[i].peBlue = 0;
			paldef->palPalEntry[i].peFlags = NULL;
		}
	}
	view_palette = CreatePalette(paldef);
	free(paldef);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void load_colours(char *proj_path)
{
	char cols_path[128];
	char project_file[128];
	char dummy[20];
	char cols_filename[20];
	int x, y;
	FILE *fp;

	sprintf(project_file, "%sfst.fpj", proj_path);
	fp = fopen(project_file, "r");
	if (fp) {
		fscanf(fp, "%s %d %d %s", dummy, &x, &y, cols_filename);
		fclose(fp);
	}
	else {
		strcpy(cols_filename, "cols.fcd");
	}
	if (strlen(proj_path) > 0) {
		sprintf(cols_path, "%s%s", proj_path, cols_filename);
	}
	else {
		sprintf(cols_path, "%s", cols_filename);
	}
	setup_colours(cols_path);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
static int get_terrain_height(int tx, int ty)
{
	int index;

	if ((tx >= 0) && (tx < terrain_x_size) && (ty >= 0) && (ty < terrain_y_size)) {
		index = (ty * terrain_x_size) + tx;
		return terrain_data[index];
	}
	return 0;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void init_terrain(void)
{
	int i, max;

	min_height = 0;
	max_height = 15;
	max = terrain_y_size * terrain_x_size;
	for (i = 0; i < max; i++) {
		terrain_data[i] = 0;
	}
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void new_terrain(int width, int height)
{
	if ((width != terrain_x_size) || (height != terrain_y_size)) {
		terrain_x_size = width;
		terrain_y_size = height;
		if (terrain_data != NULL) {
			free(terrain_data);
		}
		terrain_data = (short *)calloc(terrain_x_size * terrain_y_size, sizeof(short));
		if (terrain_data == NULL) {
			stop("No memory for terrain data");
		}
	}
	init_terrain();
	update_required = TRUE;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void analyse_terrain(void)
{
	int tx, ty, height;

	for (ty = 0; ty < terrain_y_size; ty++) {
		for (tx = 0; tx < terrain_x_size; tx++) {
			height = get_terrain_height(tx, ty);
			if (height < min_height) {
				min_height = height;
			}
			else if (height > max_height) {
				max_height = height;
			}
		}
	}
	if (max_height < 15) {
		max_height = 15;
	}
	view_pos.z = (int32)terrain_x_size * 500L;
	view_pos.y = -((long)max_height + (view_pos.z / 2L));
	view_step = view_pos.z / 5L;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
int load_terrain(char *file, int startup)
{
	FILE *fp;
	int size;
	char header[HEADER_SIZE], *p;

	startup = startup;
	fp = fopen(file, "rb");
	if (fp != NULL) {
		fread(header, 1, HEADER_SIZE, fp);
		p = strstr(header, "SIZE=") + 5;
		size = atoi(p);
		new_terrain(size, size);
		fread(terrain_data, 2, terrain_y_size * terrain_x_size, fp);
		fclose(fp);
		analyse_terrain();
		return TRUE;
	}
	return FALSE;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void setup_ground(void)
{
	setup_graphics2();
	setup_graphics3();
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void tidyup_ground(void)
{
	SelectObject(hdc_cache, hbm_old_cache);
	DeleteObject(hbm_cache);
	SelectPalette(hdc_cache, old_palette, 0);
	RealizePalette(hdc_cache);
	DeleteObject(view_palette);
	DeleteDC(hdc_cache);
}

