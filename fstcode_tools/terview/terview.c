//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
// TERVIEW.C -- 'FST-98' Terrain Viewer: Entry point
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Allows for viewing of terrain data, called from WORLD.EXE
//
// Changes:
//
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "defs.h"
#include "terview.h"

#define  SKY_BRUSH  0x2f

char prog_version[] = "Version 1.4D Beta";

extern void load_colours(char *path);

static UINT viewload_mssg = NULL;
static UINT viewexit_mssg = NULL;

int current_tool = 1;

extern int grid_on = TRUE;
extern int 	update_required = TRUE;
long FAR PASCAL _export WndProc (HWND, UINT, UINT, LONG);

static BOOL FAR PASCAL db_about_proc(HWND hAbout, unsigned msg, WORD wParam, LONG lParam);

static void tidyup_program(void);

static char szAppName[] = "'FST-98' Terrain Viewer";

HWND main_window;

static HANDLE hInst;

extern int filled = TRUE;

static char proj_path[128] = "";

extern int solid_polys = TRUE;
extern int white_polys = FALSE;

// Windows event handler
extern int grid = FALSE;
extern int white = FALSE;
extern int blue  = FALSE;
extern int black = TRUE;

extern int back_color = 1
;
extern int skycolor = FALSE;

extern int BACK_BRUSH = BLACK_BRUSH;

extern int zoom100 = TRUE;
extern int zoom200 = FALSE;
extern int zoom300 = FALSE;
extern int zoom400 = FALSE;

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Use temporary dir. specified by TMP environment variable if set, else use project directory, (found at startup)
static char *tmp_filename(char *file)
{
	static char filename[256];
	char *tmp;

     tmp = getenv("TMP");
	if (tmp) {
		sprintf(filename, "%s\\%s", tmp, file);
	}
     else {
		sprintf(filename, "%s%s", proj_path, file);
	}
	return filename;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
long FAR PASCAL _export WndProc (HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
	HDC hdc;
	PAINTSTRUCT ps;
	RECT rect;
	FARPROC lpProc;
	HMENU hm;
	WNDCLASS wndclass;
	//int scroll_dx, scroll_dy, col, wx, wy;
	//HANDLE hInstance;

	hm = GetMenu(main_window);

	CheckMenuItem(hm, IDM_ZOOM100	, zoom100     ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hm, IDM_ZOOM200	, zoom200     ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hm, IDM_ZOOM300	, zoom300     ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hm, IDM_ZOOM400	, zoom400     ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hm, IDM_WHITE	, white       ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hm, IDM_SKY	, blue        ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hm, IDM_BLACK	, black       ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hm, IDM_GRID	, grid        ? MF_CHECKED : MF_UNCHECKED);

	CheckMenuItem(hm, IDM_FILLED	, white_polys ? MF_UNCHECKED : MF_CHECKED);

	switch (message) {

		case WM_PAINT:
			hdc = BeginPaint (hwnd, &ps);
               GetClientRect (hwnd, &rect);
			redraw_client(hdc, &rect);
			EndPaint (hwnd, &ps);
			return 0;

		case WM_MOUSEMOVE:
			if (wParam & MK_LBUTTON) {
				mouse_moved(hwnd, LOWORD(lParam), HIWORD(lParam));
			}
			return 0;

		case WM_LBUTTONDOWN:
			mouse_down(hwnd, LOWORD(lParam), HIWORD(lParam), FALSE);
			return 0;

		case WM_LBUTTONUP:
			mouse_up(hwnd, LOWORD(lParam), HIWORD(lParam), FALSE, wParam);
			return 0;

		case WM_LBUTTONDBLCLK:
			mouse_double_pressed(hwnd, LOWORD(lParam), HIWORD(lParam), FALSE);
			return 0;

		case WM_RBUTTONDOWN:
			mouse_down(hwnd, LOWORD(lParam), HIWORD(lParam), TRUE);
			return 0;

		case WM_RBUTTONUP:
			mouse_up(hwnd, LOWORD(lParam), HIWORD(lParam), TRUE, wParam);
			return 0;

		case WM_RBUTTONDBLCLK:
			mouse_double_pressed(hwnd, LOWORD(lParam), HIWORD(lParam), TRUE);
			return 0;

		case WM_SIZE:
			resize_client(hwnd, LOWORD(lParam), HIWORD(lParam));
			return 0;

		case WM_CREATE:
			create_client(hwnd);
			return 0;

		case WM_COMMAND:
			switch (wParam) {

				case IDM_EXIT:
					PostMessage(HWND_BROADCAST, viewexit_mssg, NULL, NULL);
					PostQuitMessage (0);
					return 0;

				case IDM_ABOUT:
					lpProc = MakeProcInstance(db_about_proc, hInst);
					DialogBox(hInst, "DIALOG_1", hwnd, lpProc);
					FreeProcInstance(lpProc);
					return 0;

				case IDM_FILLED:
					hm = GetMenu(main_window);
					white_polys = !white_polys;
					CheckMenuItem(hm, IDM_FILLED, white_polys ? MF_UNCHECKED : MF_CHECKED);
					update_required = TRUE;
					mouse_up(hwnd, LOWORD(0), HIWORD(0), FALSE, 0);
					return 0;

				case IDM_ZOOM100:
					hm = GetMenu(main_window);
					zoom100 = TRUE;
					zoom200 = FALSE;
					zoom300 = FALSE;
					zoom400 = FALSE;
					CheckMenuItem(hm, IDM_ZOOM100, zoom100 ? MF_CHECKED : MF_UNCHECKED);
					zoom_view(1);
					update_required = TRUE;
					mouse_up(hwnd, LOWORD(0), HIWORD(0), FALSE, 0);
					return 0;

				case IDM_ZOOM200:
					hm = GetMenu(main_window);
					zoom100 = FALSE;
					zoom200 = TRUE;
					zoom300 = FALSE;
					zoom400 = FALSE;
					CheckMenuItem(hm, IDM_ZOOM200, zoom200 ? MF_CHECKED : MF_UNCHECKED);
					zoom_view(2);
					update_required = TRUE;
					mouse_up(hwnd, LOWORD(0), HIWORD(0), FALSE, 0);
					return 0;

				case IDM_ZOOM300:
					hm = GetMenu(main_window);
					zoom100 = FALSE;
					zoom200 = FALSE;
					zoom300 = TRUE;
					zoom400 = FALSE;
					CheckMenuItem(hm, IDM_ZOOM300, zoom300 ? MF_CHECKED : MF_UNCHECKED);
					zoom_view(4);
					update_required = TRUE;
					mouse_up(hwnd, LOWORD(0),HIWORD(0), FALSE, 0);
					return 0;

				case IDM_ZOOM400:
					hm = GetMenu(main_window);
					zoom100 = FALSE;
					zoom200 = FALSE;
					zoom300 = FALSE;
					zoom400 = TRUE;
					CheckMenuItem(hm, IDM_ZOOM400, zoom400 ? MF_CHECKED : MF_UNCHECKED);
					zoom_view(8);
					update_required = TRUE;
					mouse_up(hwnd, LOWORD(0), HIWORD(0), FALSE, 0);
					return 0;

				case IDM_GRID:
					hm = GetMenu(main_window);
					grid = !grid;
					CheckMenuItem(hm, IDM_GRID, grid ? MF_CHECKED : MF_UNCHECKED);
					grid_on = !grid_on;
					return 0;

				case IDM_WHITE:
					hm = GetMenu(main_window);
					black = FALSE;
					blue  = FALSE;
					white = TRUE;
					skycolor = FALSE;
					CheckMenuItem(hm, IDM_WHITE, white ? MF_CHECKED : MF_UNCHECKED);
					BACK_BRUSH = WHITE_BRUSH;
                         back_color = 2;
					wndclass.hbrBackground = GetStockObject (WHITE_BRUSH);
					mouse_up(hwnd, LOWORD(0), HIWORD(0), FALSE, 0);
					return 0;

				case IDM_SKY:
					hm = GetMenu(main_window);
					black = FALSE;
					blue  = TRUE;
					white = FALSE;
					skycolor = FALSE;
					CheckMenuItem(hm, IDM_SKY, blue ? MF_CHECKED : MF_UNCHECKED);
					BACK_BRUSH = DKGRAY_BRUSH;
                         back_color = 1;
					wndclass.hbrBackground = GetStockObject (DKGRAY_BRUSH);
					mouse_up(hwnd, LOWORD(0), HIWORD(0), FALSE, 0);
					return 0;

				case IDM_BLACK:
					hm = GetMenu(main_window);
					black = TRUE;
					blue  = FALSE;
					white = FALSE;
					skycolor = FALSE;
					CheckMenuItem(hm, IDM_BLACK, black ? MF_CHECKED : MF_UNCHECKED);
					BACK_BRUSH = BLACK_BRUSH;
					back_color = 0;
					wndclass.hbrBackground = GetStockObject (BLACK_BRUSH);
					mouse_up(hwnd, LOWORD(0), HIWORD(0), FALSE, 0);
					return 0;

				case IDM_ITEM1:
					return 0;
			}
			break;

		case WM_DESTROY:
			PostMessage(HWND_BROADCAST, viewexit_mssg, NULL, NULL);
			PostQuitMessage (0);
			return 0;
	}
	return DefWindowProc (hwnd, message, wParam, lParam);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
static BOOL FAR PASCAL db_about_proc(HWND hAbout, unsigned msg, WORD wParam, LONG lParam)
{
	lParam = lParam;
	switch(msg) {

		case WM_INITDIALOG:
			// Updates 'Adout' dialog to current version number
			SetDlgItemText(hAbout, IDM_VERSION, prog_version);
			return(TRUE);

		case WM_COMMAND:
			if (wParam == IDOK) {
				EndDialog(hAbout, TRUE);
				return(TRUE);
			}
			break;
	}
	return(FALSE);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void warning(char *message, ...)
{
	MessageBox(NULL, message, "FST Terrain Viewer", MB_ICONEXCLAMATION);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void stop(char *message, ...)
{
	if (message != NULL) {
		MessageBox(NULL, message, "FST Terrain Viewer", MB_ICONSTOP);
	}
	PostMessage(HWND_BROADCAST, viewexit_mssg, NULL, NULL);
	PostQuitMessage(0);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Window setup routines
static BOOL init_window(HANDLE hInstance)
{
	WNDCLASS wndclass;

	wndclass.style		   = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wndclass.lpfnWndProc   = WndProc;
	wndclass.cbClsExtra	   = 0;
	wndclass.cbWndExtra	   = 0;
	wndclass.hInstance	   = hInstance;
	wndclass.hIcon		   = NULL;
	wndclass.hCursor	   = LoadCursor (NULL, IDC_ARROW);
	wndclass.hbrBackground = GetStockObject (LTGRAY_BRUSH);
	wndclass.lpszMenuName  = "MENU_1";
	wndclass.lpszClassName = szAppName;

	return RegisterClass (&wndclass);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
static HWND create_window(HANDLE hInstance, char *caption, int width, int height)
{
	HWND hwnd;
	int x, y, xDesktop, yDesktop;

	xDesktop = GetSystemMetrics(SM_CXSCREEN);
	yDesktop = GetSystemMetrics(SM_CYSCREEN);
	x = (xDesktop - width) / 2;
	y = (yDesktop - height) / 2;

	hwnd = CreateWindow (
		szAppName,			// window class name
		caption,				// window caption
		WS_OVERLAPPEDWINDOW,	// window style
		x,					// initial x position
		y,					// initial y position
		width,				// initial x size
		height,				// initial y size
		NULL,				// parent window handle
		NULL,				// window menu handle
		hInstance,			// program instance handle
		NULL);				// creation parameters

	return hwnd;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void setup_program(void)
{
	FILE *fp;

	//Setup project path
	fp = fopen("project.dat", "r");
	if (fp) {
		fscanf(fp, "%s", proj_path);
		fclose(fp);
	}
	setup_ground();
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void tidyup_program(void)
{
	tidyup_ground();
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Entry point: same as normal C main
static void reload(HWND hwnd)
{
	RECT rect;

	load_terrain(tmp_filename("~TERVIEW.TMP"), TRUE);

	GetClientRect (hwnd, &rect);
	resize_client(hwnd, rect.right, rect.bottom);
	InvalidateRect(hwnd, NULL, FALSE);
	UpdateWindow(hwnd);
	BringWindowToTop(hwnd);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
int PASCAL WinMain (HANDLE hInstance, HANDLE hPrevInstance, LPSTR lpszCmdParam, int nCmdShow)
{
	HWND	hwnd;
	MSG	msg;

	lpszCmdParam = lpszCmdParam;
	if (!hPrevInstance && !init_window(hInstance)) {
		return FALSE;
	}
	hInst = hInstance;
	main_window = hwnd = create_window(hInstance, "'FST-98' Terrain Viewer", 400, 300);
	setup_program();

	load_terrain(tmp_filename("~TERVIEW.TMP"), TRUE);
	load_colours(proj_path);

	ShowWindow (hwnd, nCmdShow);
	UpdateWindow (hwnd);
	viewload_mssg = RegisterWindowMessage("FST_TERVIEWLOAD");
	viewexit_mssg = RegisterWindowMessage("FST_TERVIEWEXIT");

	while (GetMessage (&msg, NULL, 0, 0)) {
		if (msg.message == viewload_mssg) {
			reload(hwnd);
		}
		else {
			TranslateMessage (&msg);
			DispatchMessage (&msg);
		}
	}

	tidyup_program();

	return msg.wParam;
}

