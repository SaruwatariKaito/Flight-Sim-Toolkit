/* FST Project Editor */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <dir.h>
#include <bios.h>

#include <windows.h>
#include "windowsx.h"
#include "project.h"

extern char* proj_filename(char *name);
extern char colours_file[32];

extern void force_top_redraw(void);
extern void set_start(void);
extern void draw_status(int flag);

static HPEN grey192 = NULL;
static HPEN grey128 = NULL;

static BOOL mouse_down = FALSE;

static HDC        hdc_ftc;
static HRGN       hrgn_ftc;
static BITMAPINFO *ftc_bm_info = NULL;
//static BYTE huge  *ftc_pixels  = NULL;
static int ftc_width  = 100;
static int ftc_height = 100;
HPALETTE ftc_palette = NULL;

#define MAX_PEN 8
static HPEN pens[MAX_PEN];

// origin at 3,3
#define MAX_DATA 8

static BYTE huge *ftc_data[MAX_DATA][MAX_DATA];

extern void setup_pens(void);

// Application Globals
extern HANDLE hInst;
extern void exec_tool(char *tool);
extern int width, height;
extern int grid_type;
extern int show_map;


// Edit.c Locals
static char szTileName[] = "FST Start Tile"; // Colours Window Class Name
static HWND tile_window = NULL;

static int grid_size = 10;

static int x_start = 0, y_start = 0;

static int x_centre = 120, y_centre = 120;

static int wx_sel = 0, wy_sel = 0;

long int x_sel = 0, z_sel = 0;

static void force_main_redraw(void);

static int vscroll_radius = 0, hscroll_radius = 0;
static int vscroll_pos = 0,    hscroll_pos = 0;
static int mouse_state = 0; // MB_STATE_... mouse button state

static int  drag_timer  = 0;
static int mouse_x = 0;
static int mouse_y = 0;

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

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
// get_wx() - given screen x, return window x
static int get_wx(long int sx)
{
	return (sx + x_centre);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
// get_wy() - given screen y, return window y
static int get_wy(long int sy)
{
	return (y_centre - sy);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
// get_sx() - given window x, return screen x
static long int get_sx(int wx)
{
	return (wx - x_centre);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
//  get_sy() - given window y, return screen y
static long int get_sy(int wy)
{
	return (y_centre - wy);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
static long int get_world_x(int wx)
{
	return (((long int)(wx - x_centre)) * 50000L) / (long int)grid_size;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
static long int get_world_z(int wy)
{
	return (((long int)(y_centre - wy)) * 50000L) / (long int)grid_size;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void draw_box(HDC hdc, BOOL redraw)
{
	int			wx, wy;
	static int	wx_old = 0, wy_old = 0;
	int			last_mode;

	wx = get_wx(x_sel * grid_size / 50000L);
	wy = get_wy(z_sel * grid_size / 50000L);

	wx += grid_size / 2;
	wy -= grid_size / 2;

	if (!redraw) {
		// SelectObject(hdc, get_pen(BLUE));
		SelectObject(hdc, GetStockObject(WHITE_PEN));
		last_mode = SetROP2(hdc, R2_XORPEN);
		// Undraw last position
		MoveTo(hdc, wx_old - grid_size / 2, wy_old - grid_size / 2);
		LineTo(hdc, wx_old - grid_size / 2, wy_old + grid_size / 2);
		LineTo(hdc, wx_old + grid_size / 2, wy_old + grid_size / 2);
		LineTo(hdc, wx_old + grid_size / 2, wy_old - grid_size / 2);
		LineTo(hdc, wx_old - grid_size / 2, wy_old - grid_size / 2);
	}
	else {
		// SelectObject(hdc, GetStockObject(RED_PEN));
		SelectObject(hdc, get_pen(RED));
	}
	MoveTo(hdc, wx - grid_size / 2, wy - grid_size / 2);
	LineTo(hdc, wx - grid_size / 2, wy + grid_size / 2);
	LineTo(hdc, wx + grid_size / 2, wy + grid_size / 2);
	LineTo(hdc, wx + grid_size / 2, wy - grid_size / 2);
	LineTo(hdc, wx - grid_size / 2, wy - grid_size / 2);

	if (!redraw) {
		SetROP2(hdc, last_mode);
	}
	wx_old = wx;
	wy_old = wy;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void draw_grid(HDC hdc)
{
	int	i, j, x, y, max, left, right, top, bot;
	HPEN	pen, oldpen;

	oldpen = SelectObject(hdc, grey192);

	left   = -width  / 2;
	right  =  width  / 2 + (width  % 2);
	top    = +height / 2 + (height % 2);
	bot    = -height / 2;

     // Draw Grid Lines
     if (grid_type == 1) {

		for (i = left + 1; i < right; i++) {
			SelectObject(hdc, grey128);
			if (i == 0) {
				SelectObject(hdc, grey192);
			}
			MoveTo(hdc, get_wx(i * grid_size), get_wy(top * grid_size));
			LineTo(hdc, get_wx(i * grid_size), get_wy(bot * grid_size));
		}
		for (i = bot + 1; i < top; i++) {
			SelectObject(hdc, grey128);
			if (i == 0) {
				SelectObject(hdc, grey192);
			}
			MoveTo(hdc, get_wx(left  * grid_size), get_wy(i * grid_size));
			LineTo(hdc, get_wx(right * grid_size), get_wy(i * grid_size));
		}
     }

     // Draw Grid Dots
     if (grid_type == 2) {
		SelectObject(hdc, grey192);
		for (i = left + 1; i < right; i++) {
			for (j = bot + 1; j < top; j++) {
				MoveTo(hdc, get_wx(i * grid_size) - 1, get_wy(j * grid_size)   );
				LineTo(hdc, get_wx(i * grid_size) + 2, get_wy(j * grid_size)   );
				MoveTo(hdc, get_wx(i * grid_size)    , get_wy(j * grid_size) - 1);
				LineTo(hdc, get_wx(i * grid_size)    , get_wy(j * grid_size) + 2);
			}
		}
     }

     // Outline world area
	SelectObject(hdc, grey192);
	MoveTo(hdc, get_wx(left  * grid_size), get_wy(top * grid_size));
	LineTo(hdc, get_wx(right * grid_size), get_wy(top * grid_size));
	LineTo(hdc, get_wx(right * grid_size), get_wy(bot * grid_size));
	LineTo(hdc, get_wx(left  * grid_size), get_wy(bot * grid_size));
	LineTo(hdc, get_wx(left  * grid_size), get_wy(top * grid_size));

	SelectObject(hdc, oldpen);
}

/*
//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void draw_tile_window(HDC hdc)
{
	draw_grid(hdc);
	draw_box(hdc, TRUE);
}
*/

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Action functions
//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Client level interface
// redraw, resize, ...
//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Called from window event handler: hdc - device context, rect - rectangle to redraw in window co-ords, draw_shape() - draws into window bitmap
void redraw_client(HDC hdc, RECT *rect)
{
	int dx, dy, i, j, shft;
	int left, right, top, bot;
	HBRUSH hbrush;

	SelectPalette(hdc, ftc_palette, 0);
	RealizePalette(hdc);

	// Fill view area with specified color
	hbrush = CreateSolidBrush(RGB(0, 0, 128));
	FillRect(hdc, rect, hbrush);

	// Set extents
	dx    = rect->left;
	dy    = rect->top;
     shft  = MAX_DATA / 2;
	left  = -width  / 2 + shft;
	right =  width  / 2 + (width  % 2) + shft;
	top   = +height / 2 + (height % 2) + shft;
	bot   = -height / 2 + shft;

	// If user wants to see terrain then draw each parcel's bitmap
	if (show_map) {
		for (i = 0; i < MAX_DATA; i++) {
			for (j = 0; j < MAX_DATA; j++) {
				if (i >= left && i < right && j >= bot && j < top) {
					StretchDIBits(hdc, get_wx(dx + grid_size * (i - shft)), get_wy(dy + grid_size * (j - shft)) - grid_size, grid_size, grid_size, 0, 0, ftc_width, ftc_height, (LPSTR)ftc_data[i][j], ftc_bm_info, DIB_PAL_COLORS, SRCCOPY);
				}
			}
		}
     }
	// Redraw grid & selection box
    	draw_grid(hdc);
	draw_box(hdc, TRUE);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
// x & y are number of tiles from left/bottom of world
void scroll_client(HWND hwnd, int x, int y)
{
	hwnd = hwnd;

	x_centre = 120 - ((width  % 2) * grid_size / 2);
	y_centre = 120 + ((height % 2) * grid_size / 2);

	x_centre -= x * grid_size;
	y_centre -= y * grid_size;

	force_main_redraw();
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void limit_sel(void)
{
	// Check east/west limits
	x_sel = x_sel / 1000;
	if (x_sel <  -width / 2 * 50L) {
		x_sel = -width / 2 * 50L;
	}

	if (x_sel >  +(width / 2 + (width %2 ) - 1) * 50L) {
		x_sel = +(width / 2 + (width % 2) - 1) * 50L;
	}
	x_sel = x_sel * 1000L;

	// Check north/south limits
	z_sel = z_sel / 1000;
	if (z_sel <  -height / 2 * 50L) {
		z_sel = -height / 2 * 50L;
	}
	if (z_sel >  + (height / 2 + (height % 2) - 1) * 50L) {
		z_sel = + (height / 2 + (height % 2) - 1) * 50L;
	}
	z_sel = z_sel * 1000L;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void resize(void)
{
	int max, grid_step;
	HDC hdc;

	max = (width >= height) ? width : height;

	grid_size = 0;
     grid_step = 4;

	// Determine screen size of grid based on wirld size
	while (((grid_size + grid_step) * max) < 240) {
		grid_size += grid_step;
	}

	x_centre = 120 - ((width  % 2) * grid_size / 2);
	y_centre = 120 + ((height % 2) * grid_size / 2);

	hdc = GetDC(tile_window);

	// Set scroll ranges
	SetScrollRange(tile_window, SB_VERT, 0, height, FALSE);
	SetScrollRange(tile_window, SB_HORZ, 0, width, FALSE);

	// Set scroll bars to be centred
	hscroll_pos = hscroll_radius = height / 2;
	vscroll_pos = vscroll_radius = width  / 2;

	SetScrollPos(tile_window, SB_HORZ, hscroll_pos, TRUE);
	SetScrollPos(tile_window, SB_VERT, vscroll_pos, TRUE);

	ReleaseDC(tile_window, hdc);

	limit_sel();
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Client mouse functions
void mouse_click(HWND hwnd, int wx, int wy, int left)
{
	hwnd = hwnd;
	left = left;

	x_sel = get_world_x(wx);
	z_sel = get_world_z(wy);

	limit_sel();

	force_main_redraw();

	draw_status(FALSE);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void mouse_move(HWND hwnd, int wx, int wy)
{
	int x, y;
	HDC hdc;

	x_sel = get_world_x(wx);
	z_sel = get_world_z(wy);

	limit_sel();

	hdc = GetDC(hwnd);
	draw_box(hdc, FALSE);

	ReleaseDC(hwnd, hdc);

	draw_status(FALSE);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
static BITMAPINFO *create_bm_info(int width, int height)
{
	BITMAPINFO *bm_info = NULL;
	int i;
	BITMAPINFOHEADER *bmih;
	short *ind;

	bm_info = malloc(sizeof(BITMAPINFOHEADER) + (256 * sizeof(RGBQUAD)));
	bmih = &bm_info->bmiHeader;

	bmih->biSize			= sizeof(BITMAPINFOHEADER);
	bmih->biWidth			= width;
	bmih->biHeight			= height;
	bmih->biPlanes			= 1;
	bmih->biBitCount		= 8;
	bmih->biCompression		= BI_RGB;
	bmih->biSizeImage		= (long)width * (long)height;
	bmih->biXPelsPerMeter	= 4000L;
	bmih->biYPelsPerMeter	= 4000L;
	bmih->biClrUsed		= 256;
	bmih->biClrImportant	= 0;

	ind = (short *)bm_info->bmiColors;

	for (i = 0; i < 256; i++) {
		ind[i] = i;
	}
	return bm_info;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
static BYTE huge *create_bitmap(int width, int height, HDC *hdc_ptr, HRGN *hrgn_ptr)
{
	HDC hdc;
	long int line_length, i, j;
	unsigned char far* ptr;
	BYTE huge *pixels;

	if (tile_window != NULL) {
		hdc = GetDC(tile_window);
		*hdc_ptr = CreateCompatibleDC(hdc);
		*hrgn_ptr = CreateRectRgn(0, 0, width - 1, height - 1);
		SelectClipRgn(*hdc_ptr, *hrgn_ptr);
		ReleaseDC(tile_window, hdc);
	}
	create_bm_info(width, height);

	line_length = (width + (sizeof(long) - 1)) & ~(sizeof(long) - 1);
	pixels = GlobalAllocPtr(GMEM_MOVEABLE, (long int)line_length * (long int)height) ;

	for (i = 0; i < line_length * height; i++) {
		*(pixels + i) = (char)0;
	}
	return pixels;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void reset_bitmap(int width, int height, BYTE huge *pixels)
{
	long int line_length, i;
	unsigned char far* ptr;

	line_length = (width + (sizeof(long) - 1)) & ~(sizeof(long) - 1);
	for (i = 0; i < line_length * height; i++) {
		*(pixels + i) = (char)0;
	}
}

static LOGPALETTE *paldef = NULL;

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void start_palette(void)
{
	if (paldef == NULL) {
		paldef = (LOGPALETTE *)malloc(sizeof(LOGPALETTE) + (256 * sizeof(PALETTEENTRY)));
		paldef->palVersion = 0x300;
		paldef->palNumEntries = 256;
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void set_palette_entry(int n, int r, int g, int b)
{
	if ((paldef != NULL) && (n < 256)) {
		paldef->palPalEntry[n].peRed = r;
		paldef->palPalEntry[n].peGreen = g;
		paldef->palPalEntry[n].peBlue = b;
		paldef->palPalEntry[n].peFlags = NULL;
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void use_palette(void)
{
	HDC hdc;

	if (paldef != NULL) {
		if (ftc_palette != NULL) {
			DeleteObject(ftc_palette);
		}
		ftc_palette = CreatePalette(paldef);
		free(paldef);
		paldef = NULL;

		SelectPalette(hdc_ftc, ftc_palette, 0);
		RealizePalette(hdc_ftc);
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void setup_palette(void)
{
	int i, c;

	start_palette();
	for (i = 0; i < 256; i++) {
		set_palette_entry(i, i, i, i);
	}
	use_palette();
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void create_client(void)
{
	int i,j;

	for (i = 0; i < MAX_DATA; i++) {
		for (j = 0; j < MAX_DATA; j++) {
			ftc_data[i][j] = create_bitmap(100, 100, &hdc_ftc, &hrgn_ftc);
		}
	}
	//ftc_pixels  = create_bitmap(100, 100, &hdc_ftc, &hrgn_ftc);
	ftc_bm_info = create_bm_info(100, 100);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void init_tile(void)
{
	x_start = 0;
	y_start = 0;
}

/*
//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void set_pixel_line(int y, int w, unsigned char *source_pixels, int width, int height, BYTE huge *pixels)
{
	HDC hdc;
	int i, line_length;
	DWORD offset;

	y = height - y - 1;
	if (y < height) {
		line_length = (width + (sizeof(long) - 1)) & ~(sizeof(long) - 1);
		offset = (long int)line_length * (long int)y;
		for (i = 0; i < w; i++, offset++) {
			*(pixels + offset) = source_pixels[i];
		}
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
static int load_ftc_file(int x, int y)
{
	FILE *fp;
	int i, tx, ty;
	unsigned char data[100];
	char file_name[256];

	tx = x-3;
	ty = y-3;
	sprintf(file_name, "t%c%02d%c%02d.ftc", (tx < 0) ? '_' : '0', (tx < 0) ? -tx : tx , (ty < 0) ? '_' : '0', (ty < 0) ? -ty : ty);
	fp = fopen(proj_filename(file_name), "rb");
	if (fp != NULL) {
		for (i = 0; i < 100; i++) {
			fread(data, sizeof(unsigned char), 100, fp);
			set_pixel_line(99-i, 100, data, ftc_width, ftc_height, ftc_data[x][y]);
		}
		fclose(fp);
		return TRUE;
	}
	else {

	}
	return FALSE;
}
*/

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void set_pixel_line(short *source_pixels, int width, int height, BYTE huge *pixels)
{
	HDC	hdc;
	int	i, j, elev, col, offset;

	for (j = 0; j < height; j++) {

		for (i = 0; i < width; i++) {

	     	offset = (j * 100) + i;

			elev = *(source_pixels + offset);

			if (elev == 0) {
				col = 0;
			}
			else {
				col = 1;	//(int)((float)(elev) / 25.6);
			}

			*(pixels + offset) = (char)(col);
		}
	}
}

#define HEADER_SIZE 128

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
static int load_ftc_file(int x, int y)
{
	FILE		*fp;
	int		i, tx, ty, x_size;
     short	*data;
	char		file_name[256], text[80];
	char		header[HEADER_SIZE], *p;
     unsigned	size;

    	size = (unsigned)ftc_width * (unsigned)ftc_height;

	tx = x - (MAX_DATA / 2);
	ty = y - (MAX_DATA / 2);

	sprintf(file_name, "T%c%02d%c%02d.FTD", (tx < 0) ? '_' : '0', (tx < 0) ? -tx : tx , (ty < 0) ? '_' : '0', (ty < 0) ? -ty : ty);

	fp = fopen(proj_filename(file_name), "rb");

	if (fp != NULL) {

		// Load terrain elevation file (FTD) header
		fread(header, 1, HEADER_SIZE, fp);
		p = strstr(header, "SIZE=") + 5;
		x_size = atoi(p);

		if (x_size == ftc_width) {
			//fread(data, 2, size, fp);
			//set_pixel_line(data, ftc_width, ftc_height, ftc_data[x][y]);
		}
		else {
			//zero_terrain_data(data);
			sprintf(text, "Terrain elevation file '%s' is not %d by %d", file_name, 100, 100);
			MessageBox(NULL, text, "FST Project Error!", MB_ICONEXCLAMATION);
			//PostQuitMessage (0);
		}
		fclose(fp);
		return TRUE;
	}
	return FALSE;
}


//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

#define DRAG_DELAY 3
/* Mouse control in edit window
 * Mouse click     - mouse button has been depressed and released without a drag
 *                   starting
 * Mouse DragStart - mouse button has been depressed for > DRAG_TIME 1/10's
 *                   of a second - returns whether drag is initiated
 * Mouse DragMove  - drag is happening and mouse has been moved
 * Mouse DragEnd   - mouse button has been released after drag operation
 * Mouse Move      - mouse has been moved - no drag is happening
 */

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
// total redraw includes background and grid - partial indicates a colour change or selection
static void force_main_redraw(void)
{
	RECT rect;

	GetClientRect(tile_window, &rect);

	InvalidateRect(tile_window, &rect, FALSE);

	UpdateWindow(tile_window);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Windows 3.x creation and Event loop code - Windows event handler
long FAR PASCAL _export WndProcE (HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
	HDC hdc;
	PAINTSTRUCT ps;
	RECT rect;
	FARPROC lpProc;
	clock_t c;
	char mssg[32];

	switch (message) {

		case WM_PAINT:
			hdc = BeginPaint (hwnd, &ps);
			GetClientRect (hwnd, &rect);
			redraw_client(hdc, &rect);
			EndPaint (hwnd, &ps);
			return 0;

		case WM_LBUTTONDOWN:
			mouse_down = TRUE;
			return 0;

		case WM_LBUTTONUP:
			mouse_down = FALSE;
			mouse_click(hwnd, LOWORD(lParam), HIWORD(lParam), (wParam & MK_SHIFT) ? 1 : 0);
			return 0;

		case WM_MOUSEMOVE:
			if (mouse_down) {
				mouse_move(hwnd, LOWORD(lParam), HIWORD(lParam));
			}
			draw_status(FALSE);
			return 0;

		case WM_SIZE:
			//resize_client(LOWORD(lParam), HIWORD(lParam)); // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
			return 0;

		case WM_HSCROLL:

			switch (wParam) {

				case SB_LINEUP:
					hscroll_pos -= 1;
					break;

				case SB_LINEDOWN:
					hscroll_pos += 1;
					break;

				case SB_PAGEUP:
					hscroll_pos -= 5;
					break;

				case SB_PAGEDOWN:
					hscroll_pos += 5;
					break;

				case SB_THUMBPOSITION:
					hscroll_pos = LOWORD(lParam);
					break;

				case SB_THUMBTRACK:
					return DefWindowProc (hwnd, message, wParam, lParam);
			}
			if (hscroll_pos < 0) {
				hscroll_pos = 0;
			}
			else if (hscroll_pos > width) {
				hscroll_pos = width;
			}
			SetScrollPos(hwnd, SB_HORZ, hscroll_pos, TRUE);
			scroll_client(hwnd, hscroll_pos-hscroll_radius, vscroll_pos-vscroll_radius);
			return 0;

		case WM_VSCROLL:

			switch (wParam) {

				case SB_LINEUP:
					vscroll_pos -= 1;
					break;

				case SB_LINEDOWN:
					vscroll_pos += 1;
					break;

				case SB_PAGEUP:
					vscroll_pos -= 5;
					break;

				case SB_PAGEDOWN:
					vscroll_pos += 5;
					break;

				case SB_THUMBPOSITION:
					vscroll_pos = LOWORD(lParam);
					break;

				case SB_THUMBTRACK:
					return DefWindowProc (hwnd, message, wParam, lParam);
			}
			if (vscroll_pos < 0) {
				vscroll_pos = 0;
			}
			else if (vscroll_pos > height) {
				vscroll_pos = height;
			}
			SetScrollPos(hwnd, SB_VERT, vscroll_pos, TRUE);
			scroll_client(hwnd, hscroll_pos-hscroll_radius, vscroll_pos-vscroll_radius);
			return 0;

		case WM_CREATE:
			hdc = GetDC(hwnd);
			create_client();
			// Set scroll range to be same as window size
			GetClientRect (hwnd, &rect);
			SetScrollRange(hwnd, SB_VERT, 0, 100, FALSE);
			SetScrollRange(hwnd, SB_HORZ, 0, 100, FALSE);
			//Set scroll bars to be centred
			hscroll_radius = 50;
			vscroll_radius = 50;
			hscroll_pos = hscroll_radius;
			vscroll_pos = vscroll_radius;
			SetScrollPos(hwnd, SB_HORZ, hscroll_pos, TRUE);
			SetScrollPos(hwnd, SB_VERT, vscroll_pos, TRUE);
			ReleaseDC(hwnd, hdc);
			return 0;

		case WM_DESTROY:
			PostQuitMessage (0);
			return 0;
	}
	return DefWindowProc (hwnd, message, wParam, lParam);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void move_tilewindow(int x1, int y1, int x2, int y2)
{
	MoveWindow(tile_window, x1, y1, x2, y2, TRUE);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
static BOOL init_tile_window(HANDLE hInstance)
{
	WNDCLASS wndclass;

	wndclass.style			= CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc	= WndProcE;
	wndclass.cbClsExtra		= 0;
	wndclass.cbWndExtra		= 0;
	wndclass.hInstance		= hInstance;
	wndclass.hIcon			= NULL;
	wndclass.hCursor		= LoadCursor (NULL, IDC_ARROW);
	wndclass.hbrBackground	= GetStockObject (WHITE_BRUSH);
	wndclass.lpszMenuName	= NULL;
	wndclass.lpszClassName	= szTileName;

	return RegisterClass (&wndclass);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
HWND create_tile_window(HWND parent, HANDLE hInstance, int x, int y, int width, int height)
{
	int xDesktop, yDesktop;

	if (!init_tile_window(hInstance)) {
		return NULL;
	}

	tile_window = CreateWindow (
		szTileName,								// window class name
		"",										// window caption
		WS_CHILD |WS_HSCROLL | WS_VSCROLL | WS_VISIBLE,	// window style
		x,		    								// initial x position
		y,		    								// initial y position
		width,	    								// initial x size
		height,	    								// initial y size
		parent,	    								// parent window handle
		2005,	    								// window menu handle
		hInstance,    								// program instance handle
		NULL);	    								// creation parameters

	return tile_window;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
static int load_terrain_fcd(void)
{
	FILE *fp;
	int n,i,r,g,b;
	unsigned char data[100];
	char comma, s[16];
	char temp_colours[32];

	sprintf(temp_colours, colours_file);

	replace_extension(temp_colours, "FPL");

	fp = fopen(proj_filename(temp_colours), "rb");
	if (fp == NULL) {
		sprintf(s, "Can not locate '%s'", proj_filename(temp_colours));
		MessageBox(NULL, s, "FST Project Error!", MB_ICONEXCLAMATION);
		return FALSE;
	}
	else {
		start_palette();
		fscanf(fp, "%d", &n);
		while (n--) {
			fscanf(fp, "%d%c%d%c%d%c%d", &i, &comma, &r, &comma, &g, &comma, &b);
			set_palette_entry(i, r, g, b);
		}
		fclose(fp);
		use_palette();
		return TRUE;
	}
	return FALSE;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Pens
void setup_pens(void)
{
	COLORREF colour;

	pens[GREY1] = CreatePen(PS_SOLID, 1, PALETTERGB(128, 128, 128));
	pens[GREY2] = CreatePen(PS_SOLID, 1, PALETTERGB(192, 192, 192));

	pens[RED]    = CreatePen(PS_SOLID, 1, PALETTERGB(251,  21,   2));
	pens[GREEN]  = CreatePen(PS_SOLID, 1, PALETTERGB(0,   255,   0));
	pens[BLUE]   = CreatePen(PS_SOLID, 1, PALETTERGB(0,     0, 255));
	pens[LBLUE]  = CreatePen(PS_SOLID, 1, PALETTERGB(0,   255, 255));
	pens[PURPLE] = CreatePen(PS_SOLID, 1, PALETTERGB(128,   0, 128));
	pens[YELLOW] = CreatePen(PS_SOLID, 1, PALETTERGB(255, 255,   0));
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void tidyup_pens(void)
{
	int i;

	for (i = 0; i < MAX_PEN; i++) {
		if (pens[i]) {
			DeleteObject(i);
		}
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
HPEN get_pen(int col)
{
	if (col == WHITE) {
		return GetStockObject(WHITE_PEN);
	}
	else if (col == BLACK) {
		return GetStockObject(BLACK_PEN);
	}
	else {
		return pens[col];
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void setup_tile(void)
{
	int i,j;

	resize();

	for (i = 0; i < MAX_DATA; i++) {
		for (j = 0; j < MAX_DATA; j++) {
			reset_bitmap(100, 100, ftc_data[i][j]);
		}
	}

	grey128 = CreatePen(PS_SOLID, 1, RGB(128, 128, 128));
	grey192 = CreatePen(PS_SOLID, 1, RGB(192, 192, 192));
	//red = CreatePen(PS_SOLID, 1, RGB(192, 10, 5));
	setup_palette();
	setup_pens();

	for (i = 0; i < MAX_DATA; i++) {

		for (j = 0; j < MAX_DATA; j++) {

			load_ftc_file(i, j);
		}
	}

	load_terrain_fcd();
	setup_pens();
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void tidyup_tile(void)
{
	int i,j;

	for (i = 0; i < MAX_DATA; i++) {
		for (j = 0; j < MAX_DATA; j++) {
			GlobalUnlockPtr(ftc_data[i][j]);
			GlobalFree(GlobalPtrHandle(ftc_data[i][j]));
		}
	}
	DeleteObject(grey128);
	DeleteObject(grey192);
	UnregisterClass(szTileName, hInst);
}
