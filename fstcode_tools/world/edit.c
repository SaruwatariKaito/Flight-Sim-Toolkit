//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// FST World Editor
//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Provides Terrain editing and drawing functions
//
// Change control
//
// 25/09/93 - Added TIMEOFDAY, AUTOCOLS symbols
// 12/10/93 - Change drag action for raise/lower line to linking
// 01/21/98 - Incremented to WORLD version 5
// 01/21/98 - Added chaff and flare
// 05/01/98 - Added Animated objects class
// 07/01/98 - Added ground texture placement utils
//——————————————————————————————————————————————————————————————————————————————————————————————————————————————

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <bios.h>
#include <dir.h>
#include <math.h>

#include "defs.h"
#include "lists.h"
#include "terrain.h"

int world_file_version = 5;

extern BOOL view_active;

// Origin of terrain data is at bottom left of view
// ty coordinates have zero at the bottom
// wy coordinates have zero at the top

extern int max_ord;
extern int changed;
extern int latitude;
extern int longitude;
extern int show_latlong;
extern int show_texture;

HCURSOR selobj_cursor = NULL;

char proj_path[128] = "";				// full path name for project

typedef unsigned char uchar;

int edit_grid_size = 10; // Grid size in M

long x_base = 0L, y_base = 0L;

int terrain_x_size	= 0;
int terrain_y_size	= 0;
int edit_delta		= TRUE;
int edit_height	= 1;
int edit_radius	= 0;
int fractal_iters	= 10;

#define HEADER_SIZE 128
#define MAX_ZOOM 200
#define TEX_LIST_OFFSET 160

//#define R2D 182.0416666667

static short *terrain_data = NULL;
static short *texture_data = NULL;

static int max_height = 15, min_height = 0;
static int tx_start = 0, ty_start = 0;
static int zoom_factor = 1;
static int square_size = 4;
static int visible_tx = 0;
static int visible_ty = 0;

static int texture_tx = 0;
static int texture_ty = 0;

long grid_size = 500L;

//static char gauges_name[80] = "GAUGES.FGD";
//static char model_name[80] = "MODEL.FMD";

typedef struct {
	int  tex_no;
	int  tex_height;
	int  tex_width;
	char tex_image[14];
} Textures;

static int current_tex_no;
static int default_tex_no;

Textures **textures = NULL;
static int total_textures = 0;
static int used_textures = 0;

typedef struct {
	int factor, tx, ty, size;
} Zoom_def;

static List *zoom_list = NULL;
static int update_required = TRUE;
static HDC hdc_source;
static HRGN hrgn_source;
static BITMAPINFO *source_bm_info = NULL;
static uchar *source_pixels = NULL;

#define NUM_COLOURS 32

static HPALETTE terrain_palette = NULL;	//, old_palette;

HPEN		red_pen, white_pen, blk_pen, blu_pen, gry_pen, pur_pen;
HPEN		dash_pen;
HPEN		dot_pen;
HBRUSH	grey_brush;
HWND		edit_window = NULL;

static int vscroll_pos = 0, hscroll_pos = 0;

// Control size & position of terrain window
static int left_edge	= 52;
static int top_edge		= 1;
static int tedit_width	= 418;
static int tedit_height	= 418;

static long down_time = 0L;

static int down_wx  = 0, down_wy  = 0;
static int drag_wx1 = 0, drag_wy1 = 0; // start of box/line currently drawn
static int drag_wx2 = 0, drag_wy2 = 0; // end of box line currently drawn

// static int link_wx  = 0, link_wy  = 0; // current mouse position
// static int link_wx2 = 0, link_wy2 = 0; // last frame position

static BOOL linking = FALSE;					 // if line is currently drawn
static int dragging = FALSE, mouse_down = FALSE;

static char msg[80];

long FAR PASCAL _export WndProc_edit(HWND hwnd, UINT message, UINT wParam, LONG lParam);

static void draw_terrain_square(int x, int y);
static void setup_colours(void);
static void analyse_terrain(void);

static void mouse_moved(HWND hwnd, int wx, int wy);
static void mouse_pressed(HWND hwnd, int wx, int wy, BOOL right);
static void mouse_released(HWND hwnd, int wx, int wy, BOOL right);
static void mouse_double_pressed(HWND hwnd, int wx, int wy, BOOL right);

static void create_edit(HWND hwnd);
static void redraw_edit(HDC hdc, RECT *rect);
static void set_window_pos(int tx, int ty);
static void create_source_bitmap(int width, int height);
static void draw_box(HDC hdc, int x1, int y1, int x2, int y2);

static int get_terrain_height(int tx, int ty);
static int set_start_pos(int tx, int ty);
static int get_wx(int tx);
static int get_wy(int ty);

static int get_texture_value(int tx, int ty);

static void set_texture_value(int tx, int ty, int tex_val);
static void load_texture_list(void);

extern void show_mouse_coords(int wx, int wy);

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void replace_extension(char *file, char *new_ext)
{
	int i, c;

	for (i = 0; i < 26; i++) {
		c = *file++;
		if ((c == '.') || (c == '\0')) {
			break;
		}
	}
	if (c == '\0') {
		*(file - 1) = '.';
	}
	while ((c = *new_ext++) != '\0') {
		*file++ = c;
	}
	*file = '\0';
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
char* proj_filename(char *name)
{
	static char filename[128];

	if (strlen(proj_path) > 0) {
		sprintf(filename, "%s%s", proj_path, name);
	}
	else {
		sprintf(filename, "%s", name);
	}
	return filename;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static BOOL init_edit_window(HANDLE hInstance)
{
	WNDCLASS wndclass;

	wndclass.style			= CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wndclass.lpfnWndProc	= WndProc_edit;
	wndclass.cbClsExtra		= 0;
	wndclass.cbWndExtra		= 0;
	wndclass.hInstance		= hInstance;
	wndclass.hIcon			= LoadIcon (NULL, IDI_APPLICATION);
	wndclass.hCursor		= LoadCursor (NULL, IDC_ARROW);
	wndclass.hbrBackground	= GetStockObject (WHITE_BRUSH);
	wndclass.lpszMenuName	= NULL;
	wndclass.lpszClassName	= szEditName;

	return RegisterClass (&wndclass);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
HWND create_edit_window(HANDLE hInstance, HWND parent)
{
	if (!init_edit_window(hInstance)) {
		return NULL;
	}

	edit_window = CreateWindow (
		szEditName,									// window class name
		"",											// window caption
		WS_BORDER|WS_CHILD|WS_HSCROLL|WS_VSCROLL|WS_VISIBLE,	// window style
		left_edge,									// initial x position
		top_edge,										// initial y position
		tedit_width,									// initial x size
		tedit_height,									// initial y size
		parent,										// parent window handle
		2005,										// window menu handle
		hInstance,									// program instance handle
		NULL);										// creation parameters

	return edit_window;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Basic properties dialog box
static BOOL FAR PASCAL db_texture_proc(HWND hdlg, unsigned msg, WORD wParam, LONG lParam)
{
	char s[40];
	int  texno, i, h, w;

	lParam = lParam;

	switch(msg) {

		case WM_INITDIALOG:
			// Get texture number
			texno = get_texture_value(texture_tx, texture_ty);
			sprintf(s, "%d", texno);
			SetDlgItemText(hdlg, ID_DB23_TEXNO, s);
			// Set image size
			if (texno == 0) {
				sprintf(s, "(none)");
			}
			else {
				h = textures[texno + TEX_LIST_OFFSET]->tex_height;
				w = textures[texno + TEX_LIST_OFFSET]->tex_width;
				sprintf(s, "%d x %d", h, w);
			}
			SetDlgItemText(hdlg, ID_DB23_SIZE, s);
			// Setup texture list
			SendDlgItemMessage(hdlg, ID_DB23_TEXLST, CB_ADDSTRING, 0, (LPARAM)"(none)");
			for (i = 161; i < total_textures; i++) {
				if (textures[i] != NULL) {
					SendDlgItemMessage(hdlg, ID_DB23_TEXLST, CB_ADDSTRING, 0, (LPARAM)((LPSTR)textures[i]->tex_image));
				}
			}
			SendDlgItemMessage(hdlg, ID_DB23_TEXLST, CB_SETCURSEL, texno, 0);
			return(TRUE);

		case WM_COMMAND:

			switch (wParam) {

				case ID_DB23_CLEAR:
					set_texture_value(texture_tx, texture_ty, 0);
					EndDialog(hdlg, FALSE);
					return(TRUE);

				case ID_DB23_TEXLST:
					// Set texture number
					texno = (int)SendDlgItemMessage(hdlg, ID_DB23_TEXLST, CB_GETCURSEL, 0, 0);
					sprintf(s, "%d", texno);
					SetDlgItemText(hdlg, ID_DB23_TEXNO, s);
					// set image size
					if (texno == 0) {
						sprintf(s, "(none)");
					}
					else {
						h = textures[texno + TEX_LIST_OFFSET]->tex_height;
						w = textures[texno + TEX_LIST_OFFSET]->tex_width;
						sprintf(s, "%d x %d", h, w);
					}
					SetDlgItemText(hdlg, ID_DB23_SIZE, s);
					break;

				case IDOK:
					GetDlgItemText(hdlg, ID_DB23_TEXNO, s, 4);
					texno = atoi(s);
					set_texture_value(texture_tx, texture_ty, texno);
					default_tex_no = texno;
					EndDialog(hdlg, FALSE);
					return(TRUE);

				case IDCANCEL:
					EndDialog(hdlg, FALSE);
					return(TRUE);
			}
			break;

		case WM_DESTROY:
			EndDialog(hdlg, FALSE);
			return TRUE;
	}
	return(FALSE);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Edit window event handler
long FAR PASCAL _export WndProc_edit(HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
	HDC hdc;
	PAINTSTRUCT ps;
	int scroll_dx, scroll_dy, tx, ty;

	switch (message) {

		case WM_PAINT:
			//GetUpdateRect(hwnd, &rect, FALSE);
			hdc = BeginPaint (hwnd, &ps);
			//GetClientRect(hwnd, &rect);
			redraw_edit(hdc, &ps.rcPaint);
			EndPaint (hwnd, &ps);
			return 0;

		case WM_MOUSEMOVE:
			mouse_moved(hwnd, LOWORD(lParam), HIWORD(lParam));
			if (edit_mode == TERRAIN_MODE && current_tool == 7) {
				tx = get_tx(LOWORD(lParam));
				ty = get_ty(HIWORD(lParam));
				current_tex_no = get_texture_value(tx, ty);
			}
			show_mouse_coords(LOWORD(lParam), HIWORD(lParam));
			return 0;

		case WM_LBUTTONDOWN:
			mouse_pressed(hwnd, LOWORD(lParam), HIWORD(lParam), FALSE);
			return 0;

		case WM_LBUTTONUP:
			changed = TRUE;
			mouse_released(hwnd, LOWORD(lParam), HIWORD(lParam), FALSE);
			return 0;

		case WM_LBUTTONDBLCLK:
			mouse_double_pressed(hwnd, LOWORD(lParam), HIWORD(lParam), FALSE);
			return 0;

		case WM_RBUTTONDOWN:
			mouse_pressed(hwnd, LOWORD(lParam), HIWORD(lParam), TRUE);
			return 0;

		case WM_RBUTTONUP:
			changed = TRUE;
			mouse_released(hwnd, LOWORD(lParam), HIWORD(lParam), TRUE);
			return 0;

		case WM_RBUTTONDBLCLK:
			mouse_double_pressed(hwnd, LOWORD(lParam), HIWORD(lParam), TRUE);
			return 0;

		case WM_HSCROLL:
			scroll_dx = 0;

			switch (wParam) {

				case SB_LINEUP:
					scroll_dx = -1;
					break;

				case SB_LINEDOWN:
					scroll_dx = 1;
					break;

				case SB_PAGEUP:
					scroll_dx = -4;
					break;

				case SB_PAGEDOWN:
					scroll_dx = 4;
					break;

				case SB_THUMBPOSITION:
					hscroll_pos = LOWORD(lParam);
					break;

				case SB_THUMBTRACK:
					return DefWindowProc (hwnd, message, wParam, lParam);
			}
			if (scroll_dx == 0) {
				if (set_start_pos(hscroll_pos, terrain_y_size - visible_ty - vscroll_pos)) {
					hscroll_pos = tx_start;
					InvalidateRect(hwnd, NULL, FALSE);
					SetScrollPos(hwnd, SB_HORZ, hscroll_pos, TRUE);
				}
			}
			else {
				if (set_start_pos(tx_start + scroll_dx, ty_start)) {
					hscroll_pos = tx_start;
					scroll_dx = get_wx(tx_start + scroll_dx);
					ScrollWindow(hwnd, -scroll_dx, 0, NULL, NULL);
					SetScrollPos(hwnd, SB_HORZ, hscroll_pos, TRUE);
				}
			}
			if (view_active) {
				open_view_window();
			}
			return 0;

		case WM_VSCROLL:
			scroll_dy = 0;

			switch (wParam) {

				case SB_LINEUP:
					scroll_dy = -1;
					break;

				case SB_LINEDOWN:
					scroll_dy = 1;
					break;

				case SB_PAGEUP:
					scroll_dy = -4;
					break;

				case SB_PAGEDOWN:
					scroll_dy = 4;
					break;

				case SB_THUMBPOSITION:
					vscroll_pos = LOWORD(lParam);
					break;

				case SB_THUMBTRACK:
					return DefWindowProc (hwnd, message, wParam, lParam);
			}
			if (scroll_dy == 0) {
				if (set_start_pos(hscroll_pos, terrain_y_size - visible_ty - vscroll_pos)) {
					vscroll_pos = terrain_y_size - visible_ty - ty_start;
					InvalidateRect(hwnd, NULL, FALSE);
					SetScrollPos(hwnd, SB_VERT, vscroll_pos, TRUE);
				}
			}
			else {
				if (set_start_pos(tx_start, ty_start - scroll_dy)) {
					vscroll_pos = terrain_y_size - visible_ty - ty_start;
					InvalidateRect(hwnd, NULL, FALSE);
					SetScrollPos(hwnd, SB_VERT, vscroll_pos, TRUE);
				}
			}
			if (view_active) {
				open_view_window();
			}
			return 0;

		case WM_CREATE:
			create_edit(hwnd);
			SetScrollRange(hwnd, SB_VERT, 0, terrain_y_size, FALSE);
			SetScrollRange(hwnd, SB_HORZ, 0, terrain_x_size, FALSE);
			return 0;

		case WM_DESTROY:
			return 0;
	}
	return DefWindowProc (hwnd, message, wParam, lParam);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
int check_edit_palette(void)
{
	HDC hdc;
	HPALETTE old_pal;
	int n;

	hdc = GetDC(edit_window);
	old_pal = SelectPalette(hdc, terrain_palette, 0);
	n = RealizePalette(hdc);
	SelectPalette(hdc, old_pal, 0);
	ReleaseDC(edit_window, hdc);
	if (n > 0) {
		InvalidateRect(edit_window, NULL, FALSE);
		update_required = TRUE;
	}
	return n;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void set_square_size(void)
{
	int width, height;
	RECT rect;

	GetClientRect (edit_window, &rect);
	width  = rect.right - rect.left;
	height = rect.bottom - rect.top;
	if (width < height) {
		square_size = width / (terrain_x_size / zoom_factor);
	}
	else {
		square_size = height / (terrain_y_size / zoom_factor);
	}
	if (square_size < 2) {
		square_size = 2;
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static int get_wx(int tx)
{
	RECT rect;

	GetClientRect (edit_window, &rect);
	return (rect.left + ((tx - tx_start) * square_size));
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static int get_wy(int ty)
{
	RECT rect;

	GetClientRect (edit_window, &rect);
	return (rect.bottom - ((ty - ty_start) * square_size));
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
int get_tx(int wx)
{
	int tx;
	RECT rect;

	GetClientRect (edit_window, &rect);
	wx -= rect.left;
	tx = tx_start + (wx / square_size);
	if (tx < terrain_x_size) {
		return tx;
	}
	return terrain_x_size - 1;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
int get_ty(int wy)
{
	int ty;
	RECT rect;

	GetClientRect (edit_window, &rect);
	wy = rect.bottom - wy;
	ty = ty_start + (wy / square_size);
	if (ty < terrain_y_size) {
		return ty;
	}
	return terrain_y_size - 1;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// get_mx, get_my - position in world Meters
long get_mx(int wx)
{
	long mx;
	RECT rect;

	GetClientRect (edit_window, &rect);
	wx -= rect.left;
	mx = ((long)wx * grid_size) / (long)square_size;
	mx = x_base + mx + ((long)tx_start * grid_size);
	if (edit_grid_size > 1) {
		mx += edit_grid_size / 2;
		mx = (mx/edit_grid_size)*edit_grid_size;
	}
	return mx;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
long get_my(int wy)
{
	long my;
	RECT rect;

	GetClientRect (edit_window, &rect);
	wy = rect.bottom - wy;
	my = ((long)wy * grid_size) / (long)square_size;
	my = y_base + my + ((long)ty_start * grid_size);
	if (edit_grid_size > 1) {
		my += edit_grid_size / 2;
		my = (my/edit_grid_size)*edit_grid_size;
	}
	return my;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
int get_xsize(long mx)
{
	long wx;

	wx = (mx * (long)square_size) / grid_size;
	return ((int)wx);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
int get_ysize(long my)
{
	long wy;

	wy = (my * (long)square_size) / grid_size;
	return ((int)wy);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
int get_wx_mx(long mx)
{
	RECT rect;

	GetClientRect (edit_window, &rect);
	mx -= x_base + ((long)tx_start * grid_size);
	return (rect.left + get_xsize(mx));
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
int get_wy_my(long my)
{
	RECT rect;

	GetClientRect (edit_window, &rect);
	my -= y_base + ((long)ty_start * grid_size);
	return (rect.bottom - get_ysize(my));
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void get_terrain_rect(int tx1, int ty1, int tx2, int ty2, RECT *rc)
{
	int temp;

	if (tx1 > tx2) {
		temp = tx1; tx1 = tx2; tx2 = temp;
	}
	if (ty1 > ty2) {
		temp = ty1; ty1 = ty2; ty2 = temp;
	}
	rc->left = get_wx(tx1);
	rc->top = get_wy(ty2 + 1);
	rc->right = get_wx(tx2 + 1);
	rc->bottom = get_wy(ty1);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void hilight_rectangle(HDC hdc, int x, int y, int dx, int dy, BOOL reverse)
{
	HPEN light, dark, old;

	//light = GetStockObject(WHITE_PEN);
	dark  = CreatePen(PS_SOLID, 1, RGB(128, 128, 128));
	light  = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));

	old = SelectObject(hdc, (reverse) ? dark : light);

	MoveTo(hdc, x + 1 , y + dy);
	LineTo(hdc, x + dx, y + dy);
	LineTo(hdc, x + dx, y     );

	SelectObject(hdc, (reverse) ? light : dark);

	MoveTo(hdc, x      , y + dy - 1);
	LineTo(hdc, x      , y         );
	LineTo(hdc, x + dx , y         );

	SelectObject(hdc, old);
	DeleteObject(dark);
	DeleteObject(light);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void show_status_line(int tool)
{
	HDC		hdc;
	RECT		main_rect, rect;
	HFONT	hfont, hfontold;

	hdc		= GetDC(main_window);
	hfont	= CreateFont(-12,0,0,0,500,0,0,0,0,0,0,0,0,"Arial");
	hfontold	= SelectObject(hdc, hfont);

	GetClientRect (main_window, &main_rect);

	SetTextColor(hdc, RGB(  0,   0, 128));
	SetBkMode(hdc, TRANSPARENT);

	// Name of current tool & Name of selected Shape
	rect 		= main_rect;
	rect.top		= rect.bottom - 18;
	rect.bottom	= rect.top    + 15;
	rect.left		= main_rect.left + 220;
	rect.right	= main_rect.right - 5;
	FillRect(hdc, &rect, GetStockObject(LTGRAY_BRUSH));

	if (edit_mode == TERRAIN_MODE && current_tool == 7) {
		if (current_tex_no > 0) {
			sprintf(msg, "%s:  '%s'", tool_name(tool), textures[current_tex_no + TEX_LIST_OFFSET]->tex_image);
		}
		else {
			sprintf(msg, "%s:  '%s'", tool_name(tool), "none");
		}
	}
	else {
		sprintf(msg, "%s:  %s", tool_name(tool), get_selected_name());
	}

	DrawText (hdc, msg, -1, &rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
	hilight_rectangle(hdc, rect.left - 1, rect.top - 1, rect.right - rect.left + 1, rect.bottom - rect.top + 1, FALSE);

	SelectObject(hdc, hfontold);
	DeleteObject(hfont);
	ReleaseDC(main_window, hdc);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void calc_latlong(long int x, long int z, double *lat, double *lng)
{
	double angle, factor;

	*lat = latitude + z / 1855.0;
	angle = abs(*lat) / 60.0;
	angle = (angle / 180.0) * 3.14159265358979;
	factor = cos(angle);
	factor = 1855.0 * factor;
	*lng = longitude;
	if (factor > 0.001) {
		*lng += x / factor;
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
extern void show_mouse_coords(int wx, int wy)
{
	int		tx, ty, h, deg1, deg2, ofst;
	double	lat, lng, min1, min2;
	HDC		hdc;
	RECT		main_rect, rect;
	HFONT	hfont, hfontold;

	hdc		= GetDC(main_window);
	hfont	= CreateFont(-12,0,0,0,0,0,0,0,0,0,0,0,0,"Arial");
	hfontold	= SelectObject(hdc, hfont);
	ofst 	= 3;

	GetClientRect (main_window, &main_rect);

	SetBkColor(hdc,   RGB(192, 192, 192));
	SetBkMode(hdc, TRANSPARENT);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	hilight_rectangle(hdc, 2, 0, 44, 182, FALSE);						// control box
	hilight_rectangle(hdc, 51, 0, tedit_width + 1, tedit_height + 1, FALSE);	// terrain edit window

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	// Zoon text
	rect			= main_rect;
	rect.top		= main_rect.bottom - 213;
	rect.bottom	= rect.top + 15;
	rect.left		= ofst;
	rect.right	= rect.left + 43;
	FillRect(hdc, &rect, GetStockObject(LTGRAY_BRUSH));

	SetTextColor(hdc, RGB(  0,  0, 192));
	sprintf(msg, "%s", "Zoom");
	DrawText (hdc, msg, -1, &rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);

	// Zoom Factor
	rect.top		= rect.bottom;
	rect.bottom	= rect.top + 15;
	FillRect(hdc, &rect, GetStockObject(LTGRAY_BRUSH));

	SetTextColor(hdc, RGB(  0,  96, 128));
	sprintf(msg, "%d", zoom_factor);
	DrawText (hdc, msg, -1, &rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);

	hilight_rectangle(hdc, rect.left - 1, rect.top - 16, rect.right - rect.left + 1, rect.bottom - rect.top + 16, FALSE);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	// Grid text
	rect.top		= rect.bottom + 6;
	rect.bottom	= rect.top + 15;
	FillRect(hdc, &rect, GetStockObject(LTGRAY_BRUSH));

	SetTextColor(hdc, RGB(  0,  0, 192));
	sprintf(msg, "%s", "Grid");
	DrawText (hdc, msg, -1, &rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);

	// Grid Size
	rect.top		= rect.bottom;
	rect.bottom	= rect.top + 15;
	FillRect(hdc, &rect, GetStockObject(LTGRAY_BRUSH));

	SetTextColor(hdc, RGB(  0,  96, 128));
	sprintf(msg, "%dm", grid_size);
	DrawText (hdc, msg, -1, &rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);

	hilight_rectangle(hdc, rect.left - 1, rect.top - 16, rect.right - rect.left + 1, rect.bottom - rect.top + 16, FALSE);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	// Wind Speed text
	rect.top		= rect.bottom + 6;
	rect.bottom	= rect.top + 15;
	FillRect(hdc, &rect, GetStockObject(LTGRAY_BRUSH));

	SetTextColor(hdc, RGB(  0,  0, 192));
	sprintf(msg, "%s", "Winds");
	DrawText (hdc, msg, -1, &rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);

	// Wind speed
	rect.top		= rect.bottom;
	rect.bottom	= rect.top + 15;
	FillRect(hdc, &rect, GetStockObject(LTGRAY_BRUSH));

	SetTextColor(hdc, RGB(  0,  96, 128));
	sprintf(msg, "%d k", wind_speed);
	DrawText (hdc, msg, -1, &rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);

	// Wind direction
	rect.top		= rect.bottom;
	rect.bottom	= rect.top + 15;
	FillRect(hdc, &rect, GetStockObject(LTGRAY_BRUSH));

	SetTextColor(hdc, RGB(  0,  96, 128));
	sprintf(msg, "%03d°", wind_dirn);
	DrawText (hdc, msg, -1, &rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);

	hilight_rectangle(hdc, rect.left - 1, rect.top - 31, rect.right - rect.left + 1, rect.bottom - rect.top + 31, FALSE);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	// Time of day text
	rect.top		= rect.bottom + 6;
	rect.bottom	= rect.top + 15;
	FillRect(hdc, &rect, GetStockObject(LTGRAY_BRUSH));

	SetTextColor(hdc, RGB(  0,  0, 192));
	sprintf(msg, "%s", "Time");
	DrawText (hdc, msg, -1, &rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);

	// Time of day
	rect.top		= rect.bottom;
	rect.bottom	= rect.top + 15;
	FillRect(hdc, &rect, GetStockObject(LTGRAY_BRUSH));

	SetTextColor(hdc, RGB(  0,  96, 128));
	sprintf(msg, "%02d:%02d", time_of_day/60, time_of_day%60);
	DrawText (hdc, msg, -1, &rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);

	hilight_rectangle(hdc, rect.left - 1, rect.top - 16, rect.right - rect.left + 1, rect.bottom - rect.top + 16, FALSE);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	// Cursor position - in terrain squares and M
	tx = get_tx(wx);
	ty = get_ty(wy);

	// mouse posn in grid squares
	rect.top		= rect.bottom + 6;
	rect.bottom	= rect.top + 15;
	FillRect(hdc, &rect, GetStockObject(LTGRAY_BRUSH));

	SetTextColor(hdc, RGB(  64,  64, 64));
	sprintf(msg, "X: %02d", tx);
	DrawText (hdc, msg, -1, &rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);

	//rect		= main_rect;
	rect.top		= rect.bottom;
	rect.bottom	= rect.top    + 15;
	FillRect(hdc, &rect, GetStockObject(LTGRAY_BRUSH));

	SetTextColor(hdc, RGB(  64,  64, 64));
	sprintf(msg, "Y: %02d", ty);
	DrawText (hdc, msg, -1, &rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);

	hilight_rectangle(hdc, rect.left - 1, rect.top - 16, rect.right - rect.left + 1, rect.bottom - rect.top + 16, FALSE);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	// Get terrain elevation under cursor
	h  = get_terrain_height(tx, ty);

	// Cursor posn in M
	rect 		= main_rect;
	rect.top		= rect.bottom - 18;
	rect.bottom	= rect.top    + 15;
	rect.left		= ofst;
	rect.right	= rect.left + 210;
	FillRect(hdc, &rect, GetStockObject(LTGRAY_BRUSH));

	if (show_latlong) {
		calc_latlong(get_mx(wx), get_my(wy), &lat, &lng);
		deg1 = (int)(abs(lat) / 60.0);
		if (lat > 0) {
			min1 = lat - (deg1 * 60.0);
		}
		else {
			min1 = -lat - (deg1 * 60.0);
		}
		deg2 = (int)(abs(lng) / 60.0);
		if (lng > 0) {
			min2 = lng - (deg2 * 60.0);
		}
		else {
			min2 = -lng - (deg2 * 60.0);
		}

		sprintf(msg, "H: %04dm   %s%02d°%05.2f'   %s%03d°%05.2f'", h, (lat>0) ? "N: " : "S: ", deg1, min1, (lng>0) ? "E: " : "W: ", deg2, min2);
	}
	else {
		sprintf(msg, "H: %04dm   X: %06ldm   Y: %06ldm", h, get_mx(wx), get_my(wy));
	}

	SetTextColor(hdc, RGB(128,  0,   0));
	DrawText (hdc, msg, -1, &rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);

	hilight_rectangle(hdc, rect.left - 1, rect.top - 1, rect.right - rect.left + 1, rect.bottom - rect.top + 1, FALSE);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	ReleaseDC(main_window, hdc);
	SelectObject(hdc, hfontold);
	DeleteObject(hfont);

	show_status_line(current_tool);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void clip_tcoords(int *tx, int *ty)
{
	if (*tx < 0) {
		*tx = 0;
	}
	if (*ty < 0) {
		*ty = 0;
	}
	if (*tx >= terrain_x_size) {
		*tx = terrain_x_size - 1;
	}
	if (*ty >= terrain_y_size) {
		*ty = terrain_y_size - 1;
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Set texture value in terrain matrix
static void set_texture_value(int tx, int ty, int tex_val)
{
	int index;

	if ((tx >= 0) && (tx < terrain_x_size) && (ty >= 0) && (ty < terrain_y_size)) {
		if ((tex_val < 0) || (tex_val > 9999)) {
			tex_val = 0;
		}
		index = (ty * terrain_x_size) + tx;
		texture_data[index] = tex_val;
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Get the value of the texture placed in this grid
static int get_texture_value(int tx, int ty)
{
	int index;

	if ((tx >= 0) && (tx < terrain_x_size) && (ty >= 0) && (ty < terrain_y_size)) {

		index = (ty * terrain_x_size) + tx;

		return texture_data[index];
	}
	return 0;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Change terrain texture in terrain matrix
static void change_texture(HWND hwnd, int tx, int ty, BOOL right, BOOL dblclick)
{
	FARPROC lpProc;

	texture_tx = tx;
	texture_ty = ty;

	if (right) {
		if (dblclick) {
			set_texture_value(texture_tx, texture_ty, 0);
		}
		else {
			set_texture_value(texture_tx, texture_ty, default_tex_no);
		}
	}
	else {
		lpProc = MakeProcInstance(db_texture_proc, appInstance);
		DialogBox(appInstance, "DIALOG_23", hwnd, lpProc);
		FreeProcInstance(lpProc);
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Set terrain height in terrain matrix
static void set_terrain_height(int tx, int ty, int height)
{
	int index;

	if ((tx >= 0) && (tx < terrain_x_size) && (ty >= 0) && (ty < terrain_y_size)) {
		if (height < 0) {
			height = 0;
		}
		else if (height > 9999) {
			height = 9999;
		}
		index = (ty * terrain_x_size) + tx;
		terrain_data[index] = height;
		if (height > max_height) {
			max_height = height;
		}
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Get height of the terrain in this grid (lower left corner)
static int get_terrain_height(int tx, int ty)
{
	int index;

	if ((tx >= 0) && (tx < terrain_x_size) && (ty >= 0) && (ty < terrain_y_size)) {
		index = (ty * terrain_x_size) + tx;
		return terrain_data[index];
	}
	return 0;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Get height of the terrain in this grid (lower left corner)
int get_point_height(long mx, long my)
{
	int tx, ty, index;

	tx = get_tx(get_wx_mx(mx));
	ty = get_ty(get_wy_my(my));
	if ((tx >= 0) && (tx < terrain_x_size) && (ty >= 0) && (ty < terrain_y_size)) {
		index = (ty * terrain_x_size) + tx;
		return terrain_data[index];
	}
	return 0;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static int get_terrain_colour(int tx, int ty)
{
	int height, height_range;

	height = get_terrain_height(tx, ty);
	if (height == 0) {
		return 0;
	}
	height_range = max_height - min_height;
	if (height_range == 0) {
		height_range = 1;
	}
	height -= min_height;

	height = (int)(((long)height * (long)(NUM_COLOURS - 1)) / (long)height_range);

	if (height < 1) {
		return 1;
	}
	else if (height >= NUM_COLOURS) {
		return (NUM_COLOURS - 1);
	}
	return height;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void set_point_height(int tx, int ty, int height_change)
{
	if (edit_delta) {
		set_terrain_height(tx, ty, get_terrain_height(tx, ty) + height_change);
	}
	else {
		set_terrain_height(tx, ty, edit_height);
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void change_height(int tx, int ty, int height_change)
{
	int tx1, ty1, tx2, ty2, x, y, radius;

	if (edit_radius == 0) {
		set_point_height(tx, ty, height_change);
		draw_terrain_square(tx, ty);
	}
	else {
		for (radius = edit_radius; radius > 0; radius--) {
			tx1 = tx - radius;
			ty1 = ty - radius;
			tx2 = tx + radius;
			ty2 = ty + radius;
			for (y = ty1; y <= ty2; y++) {
				for (x = tx1; x <= tx2; x++) {
					set_point_height(x, y, height_change);
				}
			}
		}
		set_point_height(tx, ty, height_change);
		tx1 = tx - edit_radius;
		ty1 = ty - edit_radius;
		tx2 = tx + edit_radius;
		ty2 = ty + edit_radius;
		for (y = ty1; y <= ty2; y++) {
			for (x = tx1; x <= tx2; x++) {
				draw_terrain_square(x, y);
			}
		}
	}
	if (view_active) {
		open_view_window();
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void change_area_height(int tx1, int ty1, int tx2, int ty2, int height_change)
{
	int tx, ty;
	HCURSOR hcursor;

	hcursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
	if (tx1 > tx2) {
		tx = tx1; tx1 = tx2; tx2 = tx;
	}
	if (ty1 > ty2) {
		ty = ty1; ty1 = ty2; ty2 = ty;
	}
	for (ty = ty1; ty <= ty2; ty++) {
		for (tx = tx1; tx <= tx2; tx++) {
			set_point_height(tx, ty, height_change);
			draw_terrain_square(tx, ty);
		}
	}
	SetCursor(hcursor);
	if (view_active) {
		open_view_window();
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void change_line_height(int x1, int y1, int x2, int y2, int height_change)
{
	int dx, dy, step, gc, x, y;

	if ((dx = x2 - x1) < 0) {
		dx = -dx;
		x = x2;
		x2 = x1;
		dy = y1 - y2;
		y = y2;
		y2 = y1;
	}
	else {
		x = x1;
		dy = y2 - y1;
		y = y1;
	}
	if (dy < 0) {
		dy = -dy;
		step = -1;
	}
	else {
		step = 1;
	}
	if (dx > dy) {
		gc = dx >> 1;
		while (x <= x2) {
			set_point_height(x, y, height_change);
			draw_terrain_square(x, y);
			x++;
			gc -= dy;
			if (gc < 0) {
				y += step;
				gc += dx;
			}
		}
	}
	else {
		gc = dy >> 1;
		while (y != y2) {
			set_point_height(x, y, height_change);
			draw_terrain_square(x, y);
			y += step;
			gc -= dx;
			if (gc < 0) {
				x++;
				gc += dy;
			}
		}
		set_point_height(x, y, height_change);
		draw_terrain_square(x, y);
	}
	if (view_active) {
		open_view_window();
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void raise_fractal(int x1, int y1, int x2, int y2, int edit_height, int limit)
{
	int dx, dy, step, gc, x, y, tx, ty;

	if ((dx = x2 - x1) < 0) {
		dx = -dx;
		x = x2;
		x2 = x1;
		dy = y1 - y2;
		y = y2;
		y2 = y1;
	}
	else {
		x = x1;
		dy = y2 - y1;
		y = y1;
	}
	if (dy < 0) {
		dy = -dy;
		step = -1;
	}
	else {
		step = 1;
	}
	if (dx > dy) {
		gc = dx >> 1;
		while (x <= x2) {
			if (limit > y) {
				for (ty = y; ty <= limit; ty++) {
					set_terrain_height(x, ty, get_terrain_height(x, ty) + edit_height);
				}
			}
			else {
				for (ty = limit; ty <= y; ty++) {
					set_terrain_height(x, ty, get_terrain_height(x, ty) + edit_height);
				}
			}
			x++;
			gc -= dy;
			if (gc < 0) {
				y += step;
				gc += dx;
			}
		}
	}
	else {
		gc = dy >> 1;
		while (y != y2) {
			if (limit > x) {
				for (tx = x; tx <= limit; tx++) {
					set_terrain_height(tx, y, get_terrain_height(tx, y) + edit_height);
				}
			}
			else {
				for (tx = limit; tx <= x; tx++) {
					set_terrain_height(tx, y, get_terrain_height(tx, y) + edit_height);
				}
			}
			y += step;
			gc -= dx;
			if (gc < 0) {
				x++;
				gc += dy;
			}
		}
		set_terrain_height(x, y, get_terrain_height(x, y) + edit_height);
		draw_terrain_square(x, y);
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void fractalise(int tx1, int ty1, int tx2, int ty2)
{
	int tx, ty, i, t1, t2, step;
	HCURSOR hcursor;

	hcursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
	if (tx1 > tx2) {
		tx = tx1; tx1 = tx2; tx2 = tx;
	}
	if (ty1 > ty2) {
		ty = ty1; ty1 = ty2; ty2 = ty;
	}
	for (i = 0; i < fractal_iters; i++) {
		if (random(100) > 49) {
			step = edit_height;
		}
		else {
			step = -edit_height;
		}
		if (random(100) > 49) {
			t1 = tx1 + random(tx2 - tx1);
			t2 = tx1 + random(tx2 - tx1);
			if (random(100) > 49) {
				raise_fractal(t1, ty1, t2, ty2, step, tx1);
			}
			else {
				raise_fractal(t1, ty1, t2, ty2, step, tx2);
			}
		}
		else {
			t1 = ty1 + random(ty2 - ty1);
			t2 = ty1 + random(ty2 - ty1);
			if (random(100) > 49) {
				raise_fractal(tx1, t1, tx2, t2, step, ty1);
			}
			else {
				raise_fractal(tx1, t1, tx2, t2, step, ty2);
			}
		}
	}
	for (ty = ty1; ty <= ty2; ty++) {
		for (tx = tx1; tx <= tx2; tx++) {
			draw_terrain_square(tx, ty);
		}
	}
	SetCursor(hcursor);
	if (view_active) {
		open_view_window();
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void zoom_in(int tx1, int ty1, int tx2, int ty2)
{
	int temp, dx, factor;
	Zoom_def *zd;
	RECT rect;

	if (tx1 > tx2) {
		temp = tx1; tx1 = tx2; tx2 = temp;
	}
	if (ty1 > ty2) {
		temp = ty1; ty1 = ty2; ty2 = temp;
	}
	dx = (tx2 - tx1) + 1;
	if (dx < 1) {
		dx = 1;
	}
	GetClientRect(edit_window, &rect);
	factor = terrain_x_size / dx;
	if (factor == 0) {
		factor = 1;
	}
	if (factor < MAX_ZOOM) {
		zd = (Zoom_def *)malloc(sizeof(Zoom_def));
		if (zd != NULL) {
			zd->factor = zoom_factor;
			zd->tx = tx_start;
			zd->ty = ty_start;
			zd->size = square_size;
			list_add(zd, zoom_list);
		}
		zoom_factor = factor;
		set_square_size();
		visible_tx = terrain_x_size / zoom_factor;
		visible_ty = terrain_y_size / zoom_factor;
		set_window_pos(tx1, ty1);
	}
	redraw_terrain();
	if (view_active) {
		open_view_window();
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void zoom_out(void)
{
	Zoom_def *zd;

	zd = list_remove_first(zoom_list);
	if (zd == NULL) {
		zoom_factor = 1;
		set_square_size();
		visible_tx = terrain_x_size;
		visible_ty = terrain_y_size;
		set_window_pos(0, 0);
	}
	else {
		zoom_factor = zd->factor;
		square_size = zd->size;
		visible_tx = terrain_x_size / zoom_factor;
		visible_ty = terrain_y_size / zoom_factor;
		set_window_pos(zd->tx, zd->ty);
		free(zd);
	}
	redraw_terrain();
	if (view_active) {
		open_view_window();
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static BOOL terrain_action(HWND hwnd, int wx, int wy, RECT *rc, BOOL right, BOOL dblclick)
{
	int down_tx, down_ty, tx, ty;

	down_tx = get_tx(down_wx);
	down_ty = get_ty(down_wy);
	tx = get_tx(wx);
	ty = get_ty(wy);
	clip_tcoords(&tx, &ty);

	switch (current_tool) {

		case 3:
			zoom_in(down_tx, down_ty, tx, ty);
			change_tool(last_tool);
			return TRUE;

		case 7:
			change_texture(hwnd, tx, ty, right, dblclick);
			return TRUE;

		case 9:
			change_height(tx, ty, edit_height);
			get_terrain_rect(tx - edit_radius, ty - edit_radius, tx + edit_radius, ty + edit_radius, rc);
			return TRUE;

		case 10:
			change_height(tx, ty, -edit_height);
			get_terrain_rect(tx - edit_radius, ty - edit_radius, tx + edit_radius, ty + edit_radius, rc);
			return TRUE;

		case 11:
			change_area_height(down_tx, down_ty, tx, ty, edit_height);
			get_terrain_rect(down_tx, down_ty, tx, ty, rc);
			return TRUE;

		case 12:
			change_area_height(down_tx, down_ty, tx, ty, -edit_height);
			get_terrain_rect(down_tx, down_ty, tx, ty, rc);
			return TRUE;

		case 13:
			change_line_height(down_tx, down_ty, tx, ty, edit_height);
			get_terrain_rect(down_tx, down_ty, tx, ty, rc);
			return TRUE;

		case 14:
			change_line_height(down_tx, down_ty, tx, ty, -edit_height);
			get_terrain_rect(down_tx, down_ty, tx, ty, rc);
			return TRUE;

		case 15:
			fractalise(down_tx, down_ty, tx, ty);
			get_terrain_rect(down_tx, down_ty, tx, ty, rc);
			return TRUE;

		default:
			break;
	}
	return FALSE;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static BOOL world_action(int wx, int wy, RECT *rc, BOOL dblclk, BOOL right)
{
	int down_tx, down_ty, tx, ty;

	down_tx = get_tx(down_wx);
	down_ty = get_ty(down_wy);

	tx = get_tx(wx);
	ty = get_ty(wy);

	if ((tx >= 0) && (tx < terrain_x_size) && (ty >= 0) && (ty < terrain_y_size)) {

		switch (current_tool) {

			case 3:
				zoom_in(down_tx, down_ty, tx, ty);
				change_tool(last_tool);
				return TRUE;

			case TOOL_OBJSEL:
				select_object(down_wx, down_wy, rc);
				if (dragging) {
					move_selected_object(wx, wy, rc);
				}
				if (dblclk) {
					set_selected_properties(rc);
				}
				return TRUE;

			case TOOL_OBJNEW:
				add_object(wx, wy);
				select_object(wx, wy, rc);
				if (!right) {
					change_tool(TOOL_OBJSEL); // change tool to select object
				}
				return TRUE;

			case TOOL_OBJPST:
				if (is_copy_object()) {
					paste_object(wx, wy);
					redraw_terrain();
					select_object(wx, wy, rc);
				}
				if (!right) {
					change_tool(TOOL_OBJSEL); // change tool to select object
				}
				return TRUE;

			case TOOL_PTH:
				if (dragging) {
					move_path(get_mx(wx), get_my(wy), rc);
				}
				else {
					select_path(get_mx(wx), get_my(wy), rc);
				}
				if (dblclk) {
					set_path_properties(rc);
				}
				return TRUE;

			case TOOL_PTHNEW:
				add_path(get_mx(wx), get_my(wy), right, rc);
				return TRUE;

			case TOOL_OBJLNK:
				associate_object_path(wx, wy, rc);
				return TRUE;

			case TOOL_PTHADD:
				insert_path(get_mx(wx), get_my(wy), rc);
				change_tool(TOOL_PTH);
				return TRUE;

			case TOOL_OBJBRK:
				break_object_path(wx, wy, rc);
				return TRUE;

			default:
				break;
		}
	}
	return FALSE;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void draw_box(HDC hdc, int x1, int y1, int x2, int y2)
{
	int old_rop;
	HPEN pen, old_pen;

	pen = CreatePen(PS_SOLID, 0, RGB(255, 255, 255));
	old_pen = SelectObject(hdc, pen);
	old_rop = SetROP2(hdc, R2_XORPEN);
	MoveTo(hdc, x1, y1);
	LineTo(hdc, x2, y1);
	LineTo(hdc, x2, y2);
	LineTo(hdc, x1, y2);
	LineTo(hdc, x1, y1);
	SetROP2(hdc, old_rop);
	SelectObject(hdc, old_pen);
	DeleteObject(pen);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void draw_x(HDC hdc, int x1, int y1, int x2, int y2)
{
	HPEN pen, old_pen;

	// If user has textures turned 'ON'
	if (show_texture) {
		// Setup pen
		pen = CreatePen(PS_SOLID, 0, RGB(255, 255, 0));
		old_pen = SelectObject(hdc, pen);
		// Adjust coords
		x1 = x1 + 1;
		y1 = y1 + 1;
		x2 = x2 - 1;
		y2 = y2 - 1;
		// Draw 'X'
		MoveTo(hdc, x1    , y1    );
		LineTo(hdc, x2 + 1, y2 + 1);
		MoveTo(hdc, x1    , y2    );
		LineTo(hdc, x2 + 1, y1 - 1);
		// Tidyup pen
		SelectObject(hdc, old_pen);
		DeleteObject(pen);
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void draw_link(HDC hdc, int x1, int y1, int x2, int y2)
{
	int old_rop;
	HPEN pen, old_pen;

	//pen = CreatePen(PS_SOLID, 0, RGB(128, 255, 128));
	pen = CreatePen(PS_SOLID, 0, RGB(255, 255, 255));
	old_pen = SelectObject(hdc, pen);
	old_rop = SetROP2(hdc, R2_XORPEN);
	MoveTo(hdc, x1, y1);
	LineTo(hdc, x2, y2);
	SetROP2(hdc, old_rop);
	SelectObject(hdc, old_pen);
	DeleteObject(pen);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void mouse_pressed(HWND hwnd, int wx, int wy, BOOL right)
{
	right = right;

	down_time = biostime(0, 0L);
	drag_wx1 = down_wx = wx;
	drag_wy1 = down_wy = wy;
	dragging = FALSE;
	//link_wx  = wx; link_wy = wy;
	linking  = FALSE;
	mouse_down = TRUE;
	SetCapture(hwnd);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void mouse_released(HWND hwnd, int wx, int wy, BOOL right)
{
  //	int tx, ty;
	RECT rect;
	HDC hdc;

	if (mouse_down) {

		ReleaseCapture();
		mouse_down = FALSE;
		hdc = GetDC(hwnd);

		//undraw drag box
		if (dragging) {
			draw_box(hdc, drag_wx1, drag_wy1, drag_wx2, drag_wy2);
		}
		if (linking) {
			draw_link(hdc, drag_wx1, drag_wy1, drag_wx2, drag_wy2);
		}
		ReleaseDC(hwnd, hdc);
		GetClientRect (hwnd, &rect);
		if ((wx >= rect.left) && (wx <= rect.right) && (wy >= rect.top) && (wy <= rect.bottom)) {
			if (edit_mode == TERRAIN_MODE) {
				if (terrain_action(hwnd, wx, wy, &rect, right, FALSE)) {
					InvalidateRect(hwnd, &rect, FALSE);
				}
			}
			else if (edit_mode == OBJECT_MODE) {
				if (world_action(wx, wy, &rect, FALSE, right)) {
					InvalidateRect(hwnd, &rect, FALSE);
				}
				force_redraw_toolbar();
			}
		}
		update_menu();
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void mouse_double_pressed(HWND hwnd, int wx, int wy, BOOL right)
{
	RECT rect;

	GetClientRect (hwnd, &rect);
	if ((wx >= rect.left) && (wx <= rect.right) && (wy >= rect.top) && (wy <= rect.bottom)) {
		if (edit_mode == TERRAIN_MODE) {
			if (terrain_action(hwnd, wx, wy, &rect, right, TRUE)) {
				InvalidateRect(hwnd, &rect, FALSE);
			}
		}
		if (edit_mode == OBJECT_MODE) {
			if (world_action(wx, wy, &rect, TRUE, FALSE)) {
				InvalidateRect(hwnd, &rect, FALSE);
			}
			force_redraw_toolbar();
		}
	}
}


//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Returns 0 = no drag action associated with tool
// Returns 1 = drag box - rubber band
// Returns 2 = drag fixed size square
// Returns 3 = mouse move link selected_object to mouse posn
// Returns 4 = drag line - rubber band
//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static int drag_type_tool(int mode, int tool)
{
	static int terrain_tools[] = {0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 4, 4, 1, 0};
	static int object_tools[]  = {0, 0, 0, 1, 0, 0, 0, 0, 0, 2, 0, 3, 3, 2, 3, 3, 0};

	switch (mode) {

		case TERRAIN_MODE:
			return terrain_tools[tool];

		case OBJECT_MODE:
			return object_tools[tool];
	}
	return FALSE;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static BOOL set_link_start(void)
{
	if (get_selected_start() == FALSE) {
		return FALSE;
	}

	drag_wx1 = get_selected_wx();
	drag_wy1 = get_selected_wy();
	return TRUE;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void mouse_moved(HWND hwnd, int wx, int wy)
{
	int drag_type;
	HDC hdc;

	if (mouse_down) {
		hdc = GetDC(hwnd);
		drag_type = drag_type_tool(edit_mode, current_tool);
		if (drag_type == 1) {
			// undraw drag box
			if (dragging) {
				draw_box(hdc, drag_wx1, drag_wy1, drag_wx2, drag_wy2);
			}
			draw_box(hdc, down_wx, down_wy, wx, wy);
			dragging = TRUE;
			drag_wx2 = wx;
			drag_wy2 = wy;
		}
		else if (drag_type == 2) {
			if (dragging || ((biostime(0, 0L) - down_time) > 4)) {
				if (dragging) {
					draw_box(hdc, drag_wx1, drag_wy1, drag_wx2, drag_wy2);
				}
				drag_wx1 = wx - 4;
				drag_wy1 = wy - 4;
				drag_wx2 = wx + 4;
				drag_wy2 = wy + 4;
				draw_box(hdc, drag_wx1, drag_wy1, drag_wx2, drag_wy2);
				dragging = TRUE;
			}
		}
		else if (drag_type == 4) {
			// undraw drag box
			if (linking) {
				draw_link(hdc, drag_wx1, drag_wy1, drag_wx2, drag_wy2);
			}
			draw_link(hdc, down_wx, down_wy, wx, wy);
			linking = TRUE;
			drag_wx2 = wx;
			drag_wy2 = wy;
		}
		ReleaseDC(hwnd, hdc);
	}
	else {
		drag_type = drag_type_tool(edit_mode, current_tool);
		// if select link draw line to mouse from current object
		if (drag_type == 3) {
			if (!linking) {
				// setup start of link line at selected object
				// if not possible break out of function
				if (set_link_start() == FALSE) {
					return;
				}
			}
			// undraw last link line
			hdc = GetDC(hwnd);
			if (linking) {
				draw_link(hdc, drag_wx1, drag_wy1, drag_wx2, drag_wy2);
			}
			draw_link(hdc, drag_wx1, drag_wy1, wx, wy);
			linking = TRUE;
			drag_wx2 = wx;
			drag_wy2 = wy;
			ReleaseDC(hwnd, hdc);
		}
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void redraw_terrain(void)
{
	analyse_terrain();
	update_required = TRUE;
	set_square_size();
	InvalidateRect(edit_window, NULL, FALSE);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// DIB has origin at bottom left
static void draw_terrain_square(int tx, int ty)
{
	int col, line_length;
	uchar *p;

	if ((tx >= 0) && (tx < terrain_x_size) && (ty >= 0) && (ty < terrain_y_size)) {

		col = get_terrain_colour(tx, ty);

		line_length = (terrain_x_size + (sizeof(long) - 1)) & ~(sizeof(long) - 1);

		p = source_pixels + (ty * line_length) + tx;

		*p = col;
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// DIB has origin at bottom left
static void draw_terrain(void)
{
	int tx, ty, col, line_length;
	uchar *p, *line;

	line_length = (terrain_x_size + (sizeof(long) - 1)) & ~(sizeof(long) - 1);
	line = source_pixels;
	for (ty = 0; ty < terrain_y_size; ty++, line += line_length) {
		for (tx = 0, p = line; tx < terrain_x_size; tx++) {
			col = get_terrain_colour(tx, ty);
			*p++ = col;
		}
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static int get_pixel_col(int tx, int ty)
{
	int line_length;
	uchar *p;

	if ((tx >= 0 && tx < terrain_x_size) && (ty >= 0 && ty < terrain_y_size)) {

		line_length = (terrain_x_size + (sizeof(long) - 1)) & ~(sizeof(long) - 1);

		p = source_pixels + (ty * line_length) + tx;

		return *p;
	}
	return 0;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void draw_triangle(HDC hdc, int x1, int y1, int x2, int y2, int x3, int y3, int col)
{
	HBRUSH brush, old_brush;
	POINT pts[3];

	pts[0].x = x1;
	pts[0].y = y1;
	pts[1].x = x2;
	pts[1].y = y2;
	pts[2].x = x3;
	pts[2].y = y3;
	brush = CreateSolidBrush(PALETTEINDEX(col));
	old_brush = SelectObject(hdc, brush);

	Polygon(hdc, (LPPOINT)pts, 3);

	SelectObject(hdc, old_brush);
	DeleteObject(brush);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void draw_square(HDC hdc, int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4, int col)
{
	HBRUSH brush, old_brush;
	POINT pts[4];

	pts[0].x = x1;
	pts[0].y = y1;
	pts[1].x = x2;
	pts[1].y = y2;
	pts[2].x = x3;
	pts[2].y = y3;
	pts[3].x = x4;
	pts[3].y = y4;
	brush = CreateSolidBrush(PALETTEINDEX(col));
	old_brush = SelectObject(hdc, brush);

	Polygon(hdc, (LPPOINT)pts, 4);

	SelectObject(hdc, old_brush);
	DeleteObject(brush);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void expand_pixels(HDC hdc, int left, int top, int width, int height, int tx1, int ty1, int dtx, int dty)
{
	int i, j, x, y, tx, ty, dw, dh;
	int c1, c2, c3, c4, zero_count;
	HBRUSH brush, old_brush;

	dw = (width  / dtx);
	dh = (height / dty);

	y = top + height - dh;

	for (j = dty - 1, ty = ty1; j >= 0; j--, ty++, y -= dh) {

		y = top + ((height * j) / dty);

		for (i = 0, x = left, tx = tx1; i < dtx; i++, tx++, x += dw) {

			x = left + ((width * i) / dtx);

			zero_count = 0;

			c1 = get_pixel_col(tx    , ty    );
			c2 = get_pixel_col(tx + 1, ty    );
			c3 = get_pixel_col(tx + 1, ty + 1);
			c4 = get_pixel_col(tx    , ty + 1);

			if (c1 == 0) {
				zero_count++;
			}
			if (c2 == 0) {
				zero_count++;
			}
			if (c3 == 0) {
				zero_count++;
			}
			if (c4 == 0) {
				zero_count++;
			}
			if (zero_count == 3) {
				if (c1 > 0) {
					draw_triangle(hdc, x, y, x + dw, y + dh, x     , y + dh, c1);
					draw_triangle(hdc, x, y, x + dw, y     , x + dw, y + dh, 0);
				}
				else if (c2 > 0) {
					draw_triangle(hdc, x     , y, x + dw, y     , x, y + dh, 0);
					draw_triangle(hdc, x + dw, y, x + dw, y + dh, x, y + dh, c2);
				}
				else if (c3 > 0) {
					draw_triangle(hdc, x, y, x + dw, y + dh, x     , y + dh, 0);
					draw_triangle(hdc, x, y, x + dw, y     , x + dw, y + dh, c3);
				}
				else {
					draw_triangle(hdc, x     , y, x + dw, y     , x, y + dh, c4);
					draw_triangle(hdc, x + dw, y, x + dw, y + dh, x, y + dh, 0);
				}
				// Search for terrain texture & draw if present
				if (get_texture_value(tx, ty) > 0) {
					draw_x(hdc, x, y, x + dw, y + dh);
				}

			}
			else {
				if (zero_count == 4) {
					brush = CreateSolidBrush(PALETTEINDEX(0));
				}
				else {
					if (c1 > 0) {
						brush = c1;	//CreateSolidBrush(PALETTEINDEX(c1));
					}
					else if (c2 > 0) {
						brush = c2;	//CreateSolidBrush(PALETTEINDEX(c2));
					}
					else if (c3 > 0) {
						brush = c3;	//CreateSolidBrush(PALETTEINDEX(c3));
					}
					else {
						brush = c4;	//CreateSolidBrush(PALETTEINDEX(c4));
					}
				}
				old_brush = SelectObject(hdc, brush);

				//Rectangle(hdc, x, y, x + dw, y + dh);
				draw_square(hdc, x , y, x + dw, y, x + dw, y + dh, x, y + dh, brush);

				SelectObject(hdc, old_brush);
				DeleteObject(brush);

				// Search for terrain texture & draw if present
				if (get_texture_value(tx, ty) > 0) {
					draw_x(hdc, x, y, x + dw, y + dh);
				}
			}
		}
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void redraw_edit(HDC hdc, RECT *rc)
{
	int width, height, x1, y1, x2, y2, dx, dy;
	HCURSOR hcursor;
	HBRUSH hbrush;

	if (update_required) {
		hcursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
		draw_terrain();
		SetCursor(hcursor);
		update_required = FALSE;
	}

	//hbrush = GetStockObject(WHITE_BRUSH);
	hbrush = CreateSolidBrush(RGB(0, 128, 128));
	FillRect(hdc, rc, hbrush);
	SelectPalette(hdc, terrain_palette, 0);
	RealizePalette(hdc);

	x1 = get_tx(rc->left);
	rc->left = get_wx(x1);

	x2 = get_tx(rc->right - 1);
	rc->right = get_wx(x2 + 1);

	y1 = get_ty(rc->bottom - 1);
	rc->bottom = get_wy(y1);

	y2 = get_ty(rc->top);
	rc->top = get_wy(y2 + 1);

	dx = x2 - x1 + 1;
	if (dx > terrain_x_size) {
		dx = terrain_x_size;
	}

	dy = y2 - y1 + 1;
	if (dy > terrain_y_size) {
		dy = terrain_y_size;
	}

	width  = dx * square_size;
	height = dy * square_size;

	if (square_size > 8) {
		expand_pixels(hdc, rc->left, rc->top, width, height, x1, y1, dx, dy);
	}
	else {
		StretchDIBits(hdc, rc->left, rc->top, width, height, x1, y1, dx, dy, source_pixels, source_bm_info, DIB_PAL_COLORS, SRCCOPY);
	}
	draw_objects(hdc, rc);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static int set_start_pos(int tx, int ty)
{
	int x_max, y_max, update;

	update = FALSE;
	if (tx < 0) {
		tx = 0;
	}
	x_max = terrain_x_size - visible_tx;
	if (tx > x_max) {
		tx = x_max;
	}
	if (tx_start != tx) {
		tx_start = tx;
		update = TRUE;
	}
	if (ty < 0)
	ty = 0;
	y_max = terrain_y_size - visible_ty;
	if (ty > y_max) {
		ty = y_max;
	}
	if (ty_start != ty) {
		ty_start = ty;
		update = TRUE;
	}
	return update;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void set_window_pos(int tx, int ty)
{
	static int last_zoom_factor = 1;

	if (set_start_pos(tx, ty) || (zoom_factor != last_zoom_factor)) {
		last_zoom_factor = zoom_factor;
		InvalidateRect(edit_window, NULL, FALSE);
	}
	hscroll_pos = tx_start;
	SetScrollPos(edit_window, SB_HORZ, hscroll_pos, TRUE);
	vscroll_pos = terrain_y_size - visible_ty - ty_start;
	SetScrollPos(edit_window, SB_VERT, vscroll_pos, TRUE);
	//redraw_terrain();
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Redraws the terrain area when screen resized
void resize_edit(int width, int height)
{
	// Set terarin window size based on form size
	tedit_width  = width  - left_edge - 4;
	tedit_height = height - 25;

	MoveWindow(edit_window, left_edge, top_edge, tedit_width, tedit_height, TRUE);

	show_mouse_coords(0, 0);
	set_square_size();
	redraw_terrain();
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void create_edit(HWND hwnd)
{
	hwnd = hwnd;

	update_required = TRUE;
	setup_colours();

	gry_pen		= CreatePen(PS_SOLID, 0, RGB(160,160,160));
	pur_pen		= CreatePen(PS_SOLID, 0, RGB(255,  0,255));
	blu_pen		= CreatePen(PS_SOLID, 0, RGB(  0,128,255));
	red_pen		= CreatePen(PS_SOLID, 0, RGB(255,  0,  0));
	dash_pen		= CreatePen(PS_DASH , 1, RGB(  0,  0,  0));
	dot_pen		= CreatePen(PS_DOT  , 1, RGB(  0,  0,  0));

	white_pen		= GetStockObject(WHITE_PEN);
	grey_brush	= CreateSolidBrush(RGB(192, 192, 192));
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void analyse_terrain(void)
{
	int tx, ty, height;

	min_height = 0;
	max_height = NUM_COLOURS - 1;
	for (ty = 0; ty < terrain_y_size; ty++) {
		for (tx = 0; tx < terrain_x_size; tx++) {
			height = get_terrain_height(tx, ty);
			if (height < min_height) {
				min_height = height;
			}
			if (height > max_height) {
				max_height = height;
			}
		}
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// file name is full project or tmp pathname
static int save_terrain_file(char *file_name, short *data, short *texdat, int x_size, int y_size, int flag)
{
	FILE *fp;
	int i;
	char header[HEADER_SIZE];

	if (flag) {
		fp = fopen(proj_filename(file_name), "wb");
	}
	else {
		fp = fopen(file_name, "wb");
	}

	if (fp != NULL) {
		for (i = 0; i < HEADER_SIZE; i++) header[i] = 0; {
			sprintf(header, "'FST-98' Terrain Elevation Data V02. SIZE=%03d GAP=%03d", x_size, grid_size);
		}
		fwrite(header, 1, HEADER_SIZE, fp);
		fwrite(data, 2, y_size * x_size, fp);
		fclose(fp);

		if (flag) {
			replace_extension(file_name, "FTX");
			fp = fopen(proj_filename(file_name), "wb");
			if (fp != NULL) {
				for (i = 0; i < HEADER_SIZE; i++) header[i] = 0; {
					sprintf(header, "'FST-98' Terrain Textures Data V02. SIZE=%03d", x_size);
				}
				fwrite(header, 1, HEADER_SIZE, fp);
				fwrite(texdat, 2, y_size * x_size, fp);
				fclose(fp);
			}
		}
		return TRUE;
	}
	return FALSE;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void zero_terrain_data(short *data)
{
	unsigned size;

	size = (unsigned)terrain_x_size * (unsigned)terrain_y_size;
	while (size--) {
		*data++ = 0;
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static int load_terrain_file(char *file_name, short *data, short *texdat)
{
	FILE *fp;
	int x_size;
	unsigned size;
	char header[HEADER_SIZE], *p;

	size = (unsigned)terrain_x_size * (unsigned)terrain_y_size;

	fp = fopen(proj_filename(file_name), "rb");

	if (fp != NULL) {

		// Load terrain elevation file (FTD)
		fread(header, 1, HEADER_SIZE, fp);
		p = strstr(header, "SIZE=") + 5;
		x_size = atoi(p);
		if ((x_size == terrain_x_size) && (x_size == terrain_y_size)) {
			// Read terrain grid size from header
			p = strstr(header, "GAP=");
			if (p != NULL) {
				grid_size = atoi(p + 4);
			}
			fread(data, 2, size, fp);
		}
		else {
			zero_terrain_data(data);
			sprintf(msg, "Terrain elevation file '%s' is not %d by %d", file_name, terrain_x_size, terrain_y_size);
			warning(msg);
		}
		fclose(fp);

		// Load terrain texture file (FTX)
		replace_extension(file_name, "FTX");
		fp	= fopen(proj_filename(file_name), "rb");
		if (fp != NULL) {

			fread(header, 1, HEADER_SIZE, fp);
			p = strstr(header, "SIZE=") + 5;
			x_size = atoi(p);
			if ((x_size == terrain_x_size) && (x_size == terrain_y_size)) {
				fread(texdat, 2, size, fp);
			}
			else {
				zero_terrain_data(texdat);
				sprintf(msg, "Terrain texture file '%s' is not %d by %d", file_name, terrain_x_size, terrain_y_size);
				warning(msg);
			}
		}
		else {
			zero_terrain_data(texdat);
		}
		fclose(fp);
		return TRUE;
	}
	zero_terrain_data(data);
	zero_terrain_data(texdat);
	return FALSE;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void copy_terrain_data(short *source, int sx, int sy, short *dest, int dx, int dy, int width, int height)
{
	int x, y;
	short *p1, *p2;

	for (y = 0; y < height; y++) {
		p1 = source + ((sy + y) * terrain_x_size) + sx;
		p2 = dest + ((dy + y) * terrain_x_size) + dx;
		for (x = 0; x < width; x++) {
			*p2++ = *p1++;
		}
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void get_file_name(char *name, int fx, int fy)
{
	char xs[8],ys[8];

	if (fx >= 0) {
		sprintf(xs, "0%02d", fx);
	}
	else {
		sprintf(xs, "_%02d", -fx);
	}

	if (fy >= 0) {
		sprintf(ys, "0%02d", fy);
	}
	else {
		sprintf(ys, "_%02d", -fy);
	}

	sprintf(name, "T%s%s.FTD", xs, ys);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static int save_terrain(void)
{
	int status, fx, fy, tx, ty;
	long xsize, ysize;
	unsigned size;
	short *buffer, *texbuf;
	char file_name[16];

	status = TRUE;
	size = (unsigned)terrain_x_size * (unsigned)terrain_y_size;

	buffer = (short *)calloc(size, sizeof(short));
	if (buffer == NULL) {
		stop("No memory for terrain save buffer");
	}

	texbuf = (short *)calloc(size, sizeof(short));
	if (texbuf == NULL) {
		stop("No memory for texture save buffer");
	}

	xsize = (long)terrain_x_size * grid_size;
	ysize = (long)terrain_y_size * grid_size;

	if (x_base >= 0) {
		fx = (int)(x_base / xsize);
	}
	else {
		fx = (int)((x_base - xsize + 1) / xsize);
	}
	if (y_base >= 0) {
		fy = (int)(y_base / ysize);
	}
	else {
		fy = (int)((y_base - ysize + 1) / ysize);
	}

	tx = (int)((x_base % xsize) / grid_size);
	ty = (int)((y_base % ysize) / grid_size);

	if (tx < 0) {
		tx = terrain_x_size + tx;
	}
	if (ty < 0) {
		ty = terrain_y_size + ty;
	}

	// Builds terrain filename
	get_file_name(file_name, fx, fy);

	if ((tx == 0) && (ty == 0)) {

		if (!save_terrain_file(file_name, terrain_data, texture_data, terrain_x_size, terrain_y_size, TRUE)) {
			status = FALSE;
		}
	}
	else {
		load_terrain_file(file_name, buffer, texbuf);
		copy_terrain_data(terrain_data, 0, 0, buffer, tx, ty, terrain_x_size - tx, terrain_y_size - ty);
		copy_terrain_data(texture_data, 0, 0, texbuf, tx, ty, terrain_x_size - tx, terrain_y_size - ty);

		if (!save_terrain_file(file_name, buffer, texbuf, terrain_x_size, terrain_y_size, TRUE)) {
			status = FALSE;
		}
	}
	if (tx > 0) {
		get_file_name(file_name, fx + 1, fy);
		load_terrain_file(file_name, buffer, texbuf);
		copy_terrain_data(terrain_data, terrain_x_size - tx, 0, buffer, 0, ty, tx, terrain_y_size - ty);
		copy_terrain_data(texture_data, terrain_x_size - tx, 0, texbuf, 0, ty, tx, terrain_y_size - ty);
		if (!save_terrain_file(file_name, buffer, texbuf, terrain_x_size, terrain_y_size, TRUE)) {
			status = FALSE;
		}
	}
	if (ty > 0) {
		get_file_name(file_name, fx, fy + 1);
		load_terrain_file(file_name, buffer, texbuf);
		copy_terrain_data(terrain_data, 0, terrain_y_size - ty, buffer, tx, 0, terrain_x_size - tx, ty);
		copy_terrain_data(texture_data, 0, terrain_y_size - ty, texbuf, tx, 0, terrain_x_size - tx, ty);
		if (!save_terrain_file(file_name, buffer, texbuf, terrain_x_size, terrain_y_size, TRUE)) {
			status = FALSE;
		}
	}
	if ((tx > 0) && (ty > 0)) {
		get_file_name(file_name, fx + 1, fy + 1);
		load_terrain_file(file_name, buffer, texbuf);
		copy_terrain_data(terrain_data, terrain_x_size - tx, terrain_y_size - ty, buffer, 0, 0, tx, ty);
		copy_terrain_data(texture_data, terrain_x_size - tx, terrain_y_size - ty, texbuf, 0, 0, tx, ty);
		if (!save_terrain_file(file_name, buffer, texbuf, terrain_x_size, terrain_y_size, TRUE)) {
			status = FALSE;
		}
	}
	free(buffer);
	free(texbuf);
	return status;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
extern int add_textures(Textures *t)
{
	int 		i;
	Textures	**old_textures;

	if (used_textures >= total_textures) {
		old_textures = textures;
		total_textures += 100;
		textures = (Textures **)calloc(total_textures, sizeof(Textures *));
		for (i = 0; i < used_textures; i++) {
			textures[i] = old_textures[i];
		}
		free(old_textures);
	}
	textures[used_textures] = t;
	return (used_textures++);

}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void load_texture_list(void)
{
	FILE		*fp;
	char		s[40];
	int		n, val, t_no, t_height, t_width;
	Textures	*tx;

	fp = fopen(proj_filename("TEXTURES.FTX"), "rt");

	if (fp != NULL) {
		// Read Header
		fscanf(fp, "%s %s %s %s %s\n", s, s, s, s, s);
		fscanf(fp, "%s %d\n", s, &val);
		tx = malloc(sizeof(Textures));
		// Step through texture file
		for (n = 0; n < val; n++) {
			// Read in a texture
			tx = malloc(sizeof(Textures));
			fscanf(fp, "%d %d %d %s\n", &t_no, &t_height, &t_width, tx->tex_image);
			tx->tex_no	= t_no;
			tx->tex_height	= t_height;
			tx->tex_width	= t_width;
			add_textures(tx);
		}
	}
	fclose(fp);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static int load_terrain(void)
{
	int status, fx, fy, tx, ty;
	long xsize, ysize;
	unsigned size;
	short *buffer, *texbuf;
	char file_name[16];

	load_texture_list();

	status = TRUE;

	new_terrain(100, 100);

	size		= (unsigned)terrain_x_size * (unsigned)terrain_y_size;

	buffer	= (short *)calloc(size, sizeof(short));
	if (buffer == NULL) {
		stop("No memory for terrain load buffer");
	}

	texbuf	= (short *)calloc(size, sizeof(short));
	if (texbuf == NULL) {
		stop("No memory for texture load buffer");
	}

	xsize = (long)terrain_x_size * grid_size;
	ysize = (long)terrain_y_size * grid_size;

	if (x_base >= 0) {
		fx = (int)(x_base / xsize);
	}
	else {
		fx = (int)((x_base - xsize + 1) / xsize);
	}

	if (y_base >= 0) {
		fy = (int)(y_base / ysize);
	}
	else {
		fy = (int)((y_base - ysize + 1) / ysize);
	}

	tx = (int)((x_base % xsize) / grid_size);
	ty = (int)((y_base % ysize) / grid_size);

	if (tx < 0) {
		tx = terrain_x_size + tx;
	}

	if (ty < 0) {
		ty = terrain_y_size + ty;
	}

	// Load LOWER LEFT QUAD
	get_file_name(file_name, fx, fy);
	if (!load_terrain_file(file_name, buffer, texbuf)) {
		status = FALSE;
	}
	copy_terrain_data(buffer, tx, ty, terrain_data, 0, 0, terrain_x_size - tx, terrain_y_size - ty);
	copy_terrain_data(texbuf, tx, ty, texture_data, 0, 0, terrain_x_size - tx, terrain_y_size - ty);

	// Load UPPER LEFT QUAD
	if (tx > 0) {
		get_file_name(file_name, fx + 1, fy);
		if (!load_terrain_file(file_name, buffer, texbuf)) {
			status = FALSE;
		}
		copy_terrain_data(buffer, 0, ty, terrain_data, terrain_x_size - tx, 0, tx, terrain_y_size - ty);
		copy_terrain_data(texbuf, 0, ty, texture_data, terrain_x_size - tx, 0, tx, terrain_y_size - ty);
	}

	// Load LOWER RIGHT QUAD
	if (ty > 0) {
		get_file_name(file_name, fx, fy + 1);
		if (!load_terrain_file(file_name, buffer, texbuf)) {
			status = FALSE;
		}
		copy_terrain_data(buffer, tx, 0, terrain_data, 0, terrain_y_size - ty, terrain_x_size - tx, ty);
		copy_terrain_data(texbuf, tx, 0, texture_data, 0, terrain_y_size - ty, terrain_x_size - tx, ty);
	}

	// Load UPPER RIGHT QUAD
	if ((tx > 0) && (ty > 0)) {
		get_file_name(file_name, fx + 1, fy + 1);
		if (!load_terrain_file(file_name, buffer, texbuf)) {
			status = FALSE;
		}
		copy_terrain_data(buffer, 0, 0, terrain_data, terrain_x_size - tx, terrain_y_size - ty, tx, ty);
		copy_terrain_data(texbuf, 0, 0, texture_data, terrain_x_size - tx, terrain_y_size - ty, tx, ty);
	}
	free(buffer);
	free(texbuf);
	analyse_terrain();
	return status;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void init_terrain(void)
{
	tx_start = 0;
	ty_start = 0;
	zoom_factor = 1;
	square_size = 4;
	visible_tx = terrain_x_size;
	visible_ty = terrain_y_size;
	min_height = 0;
	max_height = NUM_COLOURS - 1;
	zero_terrain_data(terrain_data);
	zero_terrain_data(texture_data);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Builds basic color palette set for sea & terrain
static void setup_palette(void)
{
	int i, base_col;
	LOGPALETTE *paldef;

	paldef = (LOGPALETTE *)malloc(sizeof(LOGPALETTE) + (NUM_COLOURS * sizeof(PALETTEENTRY)));
	paldef->palVersion = 0x300;
	paldef->palNumEntries = NUM_COLOURS;
	// Sea color (#0)
	paldef->palPalEntry[0].peRed = 0;
	paldef->palPalEntry[0].peGreen = 0;
	paldef->palPalEntry[0].peBlue = 160;
	paldef->palPalEntry[0].peFlags = NULL;
     base_col = 69;
     //Terrain colors (#1~31)
	for (i = 1; i < NUM_COLOURS; i++) {
		paldef->palPalEntry[i].peRed = 0;
		paldef->palPalEntry[i].peGreen = base_col + (i * 6);		//((i * (255 - base_col)) / (NUM_COLOURS - 1));
		paldef->palPalEntry[i].peBlue = 0;
		paldef->palPalEntry[i].peFlags = NULL;
	}
	terrain_palette = CreatePalette(paldef);
	free(paldef);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void setup_colours(void)
{
  //	int i;
	HDC hdc;

	setup_palette();
	hdc = GetDC(edit_window);
	SelectPalette(hdc, terrain_palette, 0);
	RealizePalette(hdc);
	SelectPalette(hdc_source, terrain_palette, 0);
	ReleaseDC(edit_window, hdc);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void create_bm_info(int width, int height)
{
	int i;
	BITMAPINFOHEADER *bmih;
	short *ind;

	if (source_bm_info == NULL) {
		source_bm_info = malloc(sizeof(BITMAPINFOHEADER) + (256 * sizeof(RGBQUAD)));
	}
	bmih = &source_bm_info->bmiHeader;
	bmih->biSize = sizeof(BITMAPINFOHEADER);
	bmih->biWidth = width;
	bmih->biHeight = height;
	bmih->biPlanes = 1;
	bmih->biBitCount = 8;
	bmih->biCompression = BI_RGB;
	bmih->biSizeImage = (long)width * (long)height;
	bmih->biXPelsPerMeter = 4000L;
	bmih->biYPelsPerMeter = 4000L;
	bmih->biClrUsed = NUM_COLOURS;
	bmih->biClrImportant = 0;
	ind = (short *)source_bm_info->bmiColors;
	for (i = 0; i < NUM_COLOURS; i++) {
		ind[i] = i;
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void create_source_bitmap(int width, int height)
{
	HDC hdc;
	int line_length;

	if (edit_window != NULL) {
		hdc = GetDC(edit_window);
		hdc_source = CreateCompatibleDC(hdc);
		hrgn_source = CreateRectRgn(0, 0, width - 1, height - 1);
		SelectClipRgn(hdc_source, hrgn_source);
		ReleaseDC(edit_window, hdc);
	}
	create_bm_info(width, height);
	if (source_pixels != NULL) {
		free(source_pixels);
	}
	line_length = (width + (sizeof(long) - 1)) & ~(sizeof(long) - 1);
	source_pixels = (uchar *)calloc(line_length * height, 1);
	if (source_pixels == NULL) {
		stop("No memory for source pixels");
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void new_terrain(int width, int height)
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

		if (texture_data != NULL) {
			free(texture_data);
		}
		texture_data = (short *)calloc(terrain_x_size * terrain_y_size, sizeof(short));
		if (texture_data == NULL) {
			stop("No memory for texture data");
		}
	}

	init_terrain();

	create_source_bitmap(width, height);

	if (edit_window != NULL) {
		SetScrollRange(edit_window, SB_VERT, 0, terrain_y_size, FALSE);
		SetScrollRange(edit_window, SB_HORZ, 0, terrain_x_size, FALSE);
		set_window_pos(0, 0);
	}
	update_required = TRUE;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
BOOL load_symbols(FILE *fp)
{
	char symbol_name[32];
	char symbol_tag[32];

	// Weather data
	fscanf(fp, "%s\n", symbol_tag);
	if (strcmp(symbol_tag, "Symbols") == 0) {
		fscanf(fp, "%s %d\n", symbol_name, &wind_speed);
		fscanf(fp, "%s %d\n", symbol_name, &wind_dirn);
		fscanf(fp, "%s %d\n", symbol_name, &time_of_day);
		fscanf(fp, "%s %d\n", symbol_name, &automatic_colours);
		return TRUE;
	}
	return FALSE;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
int load_world(char *file_name)
{
	FILE *fp;
	int  status;
	char s[60];

	status = FALSE;
	destroy_objects();
	fp = fopen(proj_filename(file_name), "r");
	if (fp != NULL) {
		fscanf(fp, "%s %d\n", s, &world_file_version);

		switch (world_file_version) {

			case 1:
				max_ord = 6;
				break;

			case 2:
			case 3:
			case 4:
			case 5:
				max_ord = 7;
				break;

			default:
				stop("World version incorrect");
				break;
		}
		load_objects(fp);
		load_symbols(fp);
		fclose(fp);
		status = TRUE;
		world_file_version = 5;
	}
	load_terrain();
	return status;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
BOOL save_symbols(FILE *fp)
{
	// Weather data
	fprintf(fp, "Symbols\n");
	fprintf(fp, "WINDSPEED %d\n", wind_speed);
	fprintf(fp, "WINDDIR   %d\n", wind_dirn);
	fprintf(fp, "TIMEOFDAY %d\n", time_of_day);
	fprintf(fp, "AUTOCOLS  %d\n", automatic_colours);

	return TRUE;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
int save_world(char *file_name)
{
	FILE *fp;
	int  status;

	fp = fopen(proj_filename(file_name), "w");
	if (fp != NULL) {
		fprintf(fp, "FSTPC 5\n");
		save_objects(fp);
		save_symbols(fp);
		fclose(fp);
		status = save_terrain();
		return status;
	}
	return FALSE;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Saves terrain data to tmp file for terrview application file name is full path name
void save_view_data(char *file_name)
{
	int x, y;
	unsigned size;
	short *buffer, *texbuf, *p1, *p2;

	size = (unsigned)visible_tx * (unsigned)visible_tx;

	buffer = (short *)calloc(size, sizeof(short));
	if (buffer == NULL) {
		stop("No memory for terrain save buffer");
	}
	for (y = 0; y < visible_tx; y++) {
		p1 = terrain_data + ((ty_start + y) * terrain_x_size) + tx_start;
		p2 = buffer + (y * visible_tx);
		for (x = 0; x < visible_tx; x++) {
			*p2++ = *p1++;
		}
	}

	texbuf = (short *)calloc(size, sizeof(short));
	if (texbuf == NULL) {
		stop("No memory for texture save buffer");
	}
	for (y = 0; y < visible_tx; y++) {
		p1 = texture_data + ((ty_start + y) * terrain_x_size) + tx_start;
		p2 = texbuf + (y * visible_tx);
		for (x = 0; x < visible_tx; x++) {
			*p2++ = *p1++;
		}
	}

	save_terrain_file(file_name, buffer, texbuf, visible_tx, visible_tx, FALSE);

	free(buffer);
	free(texbuf);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void setup_terrain(void)
{
	//int w, h;
	FILE *fp;
	char buff[128];

	x_base = start_x;
	y_base = start_z;
	randomize();
	fp = fopen("project.dat", "r");
	if (fp != NULL) {
		fscanf(fp, "%s\n", proj_path);
		fclose(fp);
	}
	else {
		if (getcwd(buff, 128)) {
			sprintf(proj_path, "%s\\", buff);
		}
	}
	setup_shape();
	setup_objects();
	zoom_list = new_list();
	load_world("WORLD.FST");
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void tidyup_terrain(void)
{
	tidyup_shape();
	DeleteObject(grey_brush);
	DeleteObject(red_pen);
	DeleteObject(dash_pen);
	DeleteObject(dot_pen);
	DeleteObject(hrgn_source);
	DeleteDC(hdc_source);
	DeleteObject(terrain_palette);
	free(source_bm_info);
	free(source_pixels);
	free(terrain_data);
	free(texture_data);
}

