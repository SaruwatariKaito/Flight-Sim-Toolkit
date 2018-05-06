//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
// TERRAIN.C -- FST Terrain Editor: Entry point
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Changes:
//
// 25/09/93	- Added time of day menu item and dialog box + time_of_day and automatic_colours globals (+ref in .h)
// 12/10/93	- Amalgamated edit_height & edit_radius editing into single box
// 			- Save_before_quit()
// v1.04		- Added new target type, BLUE_TEAM objects.c
// 			- Added variable grid size, loading and menu fnc.
// v1.10		- Added bomber, rear gun, fixed AAgun
// v1.40		- Added Animated class, Military object chaff & flare
// v1.50		- Added Object position X/Z to dialog, Texture placement
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————


//#include <windows.h>
//#include <stdarg.h>
//#include <dir.h>
//#include <math.h>
//#include <lzexpand.h>
//#include <commdlg.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>

#include "defs.h"
#include "terrain.h"

#define FORM_MIN_WIDTH  447	// Set minimum form width
#define FORM_MIN_HEIGHT 447	// Set minimum form height
#define FORM_MAX_WIDTH  482	// Set starting form width
#define FORM_MAX_HEIGHT 489	// Set starting form height
MINMAXINFO FAR *lpMinMaxInfo;

char prog_version[] = "Version 1.5f Beta";

extern int box_size = 2;

int latitude		= 0;
int longitude 		= 0;
int changed		= FALSE;
int show_latlong	= FALSE;

extern int redraw 		= TRUE;

extern int show_paths 	= TRUE;
extern int show_selpath 	= TRUE;
extern int show_objects 	= TRUE;
extern int show_texture 	= TRUE;
extern int show_teams 	= FALSE;

long int start_x = 0L, start_z = 0L;
long FAR PASCAL _export WndProc (HWND, UINT, UINT, LONG);

char cmd_text[150] = "";

char szAppName[] = "FST Terrain";
char szEditName[] = "FST Terrain Edit";
char szControlName[] = "FST Terrain Control";

HANDLE appInstance = NULL;
HWND main_window = NULL;

static BOOL FAR PASCAL db_about_proc(HWND hAbout, unsigned msg, WORD wParam, LONG lParam);
static BOOL FAR PASCAL db_height_proc(HWND hAbout, unsigned msg, WORD wParam, LONG lParam);
static BOOL FAR PASCAL db_value_proc(HWND hdlg, unsigned msg, WORD wParam, LONG lParam);
static BOOL FAR PASCAL db_weather_proc(HWND hdlg, unsigned msg, WORD wParam, LONG lParam);
static BOOL FAR PASCAL db_tod_proc(HWND hdlg, unsigned msg, WORD wParam, LONG lParam);
static BOOL FAR PASCAL db_grid_proc(HWND hdlg, unsigned msg, WORD wParam, LONG lParam);
static BOOL FAR PASCAL db_grid_size_proc(HWND hdlg, unsigned msg, WORD wParam, LONG lParam);
static BOOL FAR PASCAL db_getfile_proc(HWND hdlg, unsigned msg, WORD wParam, LONG lParam);

//static BOOL FAR PASCAL db_new_proc(HWND hAbout, unsigned msg, WORD wParam, LONG lParam);

static int request_value(HWND hwnd, char *message, int *pval);

static void tidyup_program(void);

//static char message[80];
//static char warning_text[80];

//static char world_cols[32] = "cols.fcd";

int wind_speed = 0;
int wind_dirn  = 0;

int time_of_day = 840; // in minutes 0 - midnight, 720 is noon
int automatic_colours = TRUE; // use automatic pallete change in fly.exe

//static int new_dialog_x = 20, new_dialog_y = 20;
//static int terrain_size = 0;

static char dialog_text[128];
static int dialog_value = 0;
static char dialog_file[128];

extern void show_mouse_coords(int wx, int wy);

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
static short save_before_quit(HWND hwnd)
{
	char  szBuffer [64 + _MAX_FNAME + _MAX_EXT] ;
	short nReturn ;

	wsprintf (szBuffer, "Save current changes ?");

	nReturn = MessageBox (hwnd, szBuffer, "FST World", MB_YESNOCANCEL | MB_ICONQUESTION) ;

	if (nReturn == IDYES) {
		if (!SendMessage (hwnd, WM_COMMAND, IDM_SAVE, 0L)) {
			nReturn = IDCANCEL ;
		}
	}
	return nReturn ;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
// If environment variable TMP is set up use it to create temp path name, else use current project directory
static char *tmp_filename(char *file)
{
	static char filename[256];
	char *tmp;

	tmp = getenv("TMP");
	if (tmp != NULL) {
		sprintf(filename, "%s\\%s", tmp, file);
	}
	else {
		sprintf(filename, "%s%s", proj_path, file);
	}
	return filename;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void update_menu(void)
{
	HMENU hm;

	hm = GetMenu(main_window);

	// Edit Menu
	EnableMenuItem(hm, IDM_CUT		, is_selected_object() ?  MF_ENABLED : MF_GRAYED);
	EnableMenuItem(hm, IDM_COPY		, is_selected_object() ?  MF_ENABLED : MF_GRAYED);
	EnableMenuItem(hm, IDM_PASTE		, is_copy_object() ?      MF_ENABLED : MF_GRAYED);

	EnableMenuItem(hm, IDM_OBJPROP	, is_selected_object() ?  MF_ENABLED : MF_GRAYED);
	EnableMenuItem(hm, IDM_REMSHAPE	, is_selected_object() ?  MF_ENABLED : MF_GRAYED);

	EnableMenuItem(hm, IDM_HSTEP		, (edit_mode == TERRAIN_MODE) ?  MF_ENABLED : MF_GRAYED);
	EnableMenuItem(hm, IDM_HRADIUS	, (edit_mode == TERRAIN_MODE) ?  MF_ENABLED : MF_GRAYED);
	EnableMenuItem(hm, IDM_FITERS		, (edit_mode == TERRAIN_MODE) ?  MF_ENABLED : MF_GRAYED);

	EnableMenuItem(hm, IDM_DELPATH	, is_selected_path() ?  MF_ENABLED : MF_GRAYED);
	EnableMenuItem(hm, IDM_PATHATTR	, is_selected_path() ?  MF_ENABLED : MF_GRAYED);
	EnableMenuItem(hm, IDM_DELPOINT	, is_selected_posn() ?  MF_ENABLED : MF_GRAYED);

	CheckMenuItem(hm, IDM_PATHS		, show_paths   ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hm, IDM_SELPATH		, show_selpath ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hm, IDM_OBJECTS		, show_objects ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hm, IDM_TEXTURE 	, show_texture ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hm, IDM_LATLONG		, show_latlong ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hm, IDM_TEAMS		, show_teams   ? MF_CHECKED : MF_UNCHECKED);

	CheckMenuItem(hm, IDM_BOX_L		, (box_size == 4) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hm, IDM_BOX_M		, (box_size == 2) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hm, IDM_BOX_S		, (box_size == 1) ? MF_CHECKED : MF_UNCHECKED);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Main window event handler
long FAR PASCAL _export WndProc (HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
	HMENU hm;
	RECT rect;
	FARPROC lpProc;

	switch (message) {

		case WM_GETMINMAXINFO:
			// set the MINMAXINFO structure pointer
			lpMinMaxInfo = (MINMAXINFO FAR *) lParam;
			lpMinMaxInfo->ptMinTrackSize.x = FORM_MIN_WIDTH;
			lpMinMaxInfo->ptMinTrackSize.y = FORM_MIN_HEIGHT;
			//lpMinMaxInfo->ptMaxTrackSize.x = FORM_MAX_WIDTH;
			//lpMinMaxInfo->ptMaxTrackSize.y = FORM_MAX_HEIGHT;
			break;

		case WM_CREATE:
			return 0;

		case WM_QUERYNEWPALETTE:
			return check_edit_palette();

		case WM_SIZE:
			resize_edit(LOWORD(lParam), HIWORD(lParam));
			show_mouse_coords(LOWORD(lParam), HIWORD(lParam));
			return 0;

		case WM_COMMAND:

			switch (wParam) {

				case IDM_ABOUT:
					lpProc = MakeProcInstance(db_about_proc, appInstance);
					DialogBox(appInstance, "DIALOG_1", hwnd, lpProc);
					FreeProcInstance(lpProc);
					return 0;

				case IDM_LOAD:
					lpProc = MakeProcInstance(db_getfile_proc, appInstance);
					strcpy(dialog_file, proj_filename("*.FST"));
					if (DialogBox(appInstance, "DIALOG_5", hwnd, lpProc)) {
						load_world(dialog_file);
						FreeProcInstance(lpProc);
						GetClientRect (hwnd, &rect);
						resize_edit(rect.right, rect.bottom);
						InvalidateRect(hwnd, NULL, FALSE);
						UpdateWindow(hwnd);
					}
					return 0;

				case IDM_SAVE:
					save_world("WORLD.FST");
					changed = FALSE;
					return 1;

				case IDM_EXIT:
					if (!changed || (IDCANCEL != save_before_quit(hwnd))) {
						DestroyWindow(hwnd);
					}
					return 0;

				case IDM_CUT:
					copy_selected_object();
					remove_object();
					update_menu();
					if (is_copy_object()) {
						change_tool(TOOL_OBJPST);
					}
					break;

				case IDM_COPY:
					copy_selected_object();
					update_menu();
					if (is_copy_object()) {
						change_tool(TOOL_OBJPST);
					}
					break;

				case IDM_PASTE:
					if (is_copy_object()) {
						change_tool(TOOL_OBJPST);
					}
					break;

				case IDM_TOD:
					lpProc = MakeProcInstance(db_tod_proc, appInstance);
					DialogBox(appInstance, "DIALOG_22", hwnd, lpProc);
					FreeProcInstance(lpProc);
					changed = TRUE;
					break;

				case IDM_WEATHER:
					lpProc = MakeProcInstance(db_weather_proc, appInstance);
					DialogBox(appInstance, "DIALOG_9", hwnd, lpProc);
					FreeProcInstance(lpProc);
					changed = TRUE;
					break;

				case IDM_GRID_SIZE:
					lpProc = MakeProcInstance(db_grid_size_proc, appInstance);
					DialogBox(appInstance, "DIALOG_16", hwnd, lpProc);
					FreeProcInstance(lpProc);
					return 0;

				case IDM_HSTEP:
					lpProc = MakeProcInstance(db_height_proc, appInstance);
					DialogBox(appInstance, "DIALOG_7", hwnd, lpProc);
					FreeProcInstance(lpProc);
					return 0;

				case IDM_FITERS:
					request_value(hwnd, "Enter fractal iterations", &fractal_iters);
					return 0;

				case IDM_OBJPROP:
					set_selected_properties(NULL);
					return 0;

				case IDM_LSHAPE:
					choose_default_object(hwnd);
					return 0;

				case IDM_GRID:
					lpProc = MakeProcInstance(db_grid_proc, appInstance);
					DialogBox(appInstance, "DIALOG_16", hwnd, lpProc);
					FreeProcInstance(lpProc);
					return 0;

				case IDM_LATLONG:
					hm = GetMenu(main_window);
					show_latlong = !show_latlong;
					CheckMenuItem(hm, IDM_LATLONG, show_latlong ? MF_CHECKED : MF_UNCHECKED);
					return 0;

				case IDM_PATHS:
					hm = GetMenu(main_window);
					show_paths = !show_paths;
					CheckMenuItem(hm, IDM_PATHS, show_paths ? MF_CHECKED : MF_UNCHECKED);
					redraw_terrain();
					return 0;

				case IDM_SELPATH:
					hm = GetMenu(main_window);
					show_selpath = !show_selpath;
					CheckMenuItem(hm, IDM_SELPATH, show_selpath ? MF_CHECKED : MF_UNCHECKED);
					redraw_terrain();
					return 0;

				case IDM_OBJECTS:
					hm = GetMenu(main_window);
					show_objects = !show_objects;
					CheckMenuItem(hm, IDM_OBJECTS, show_objects ? MF_CHECKED : MF_UNCHECKED);
					redraw_terrain();
					return 0;

				case IDM_TEAMS:
					hm = GetMenu(main_window);
					show_teams = !show_teams;
					CheckMenuItem(hm, IDM_TEAMS, show_teams ? MF_CHECKED : MF_UNCHECKED);
					redraw_terrain();
					return 0;

				case IDM_BOX_L:
					hm = GetMenu(main_window);
					box_size = 4;
					CheckMenuItem(hm, IDM_BOX_L, (box_size == 4) ? MF_CHECKED : MF_UNCHECKED);
					CheckMenuItem(hm, IDM_BOX_M, (box_size == 2) ? MF_CHECKED : MF_UNCHECKED);
					CheckMenuItem(hm, IDM_BOX_S, (box_size == 1) ? MF_CHECKED : MF_UNCHECKED);
					redraw_terrain();
					return 0;

				case IDM_BOX_M:
					hm = GetMenu(main_window);
					box_size = 2;
					CheckMenuItem(hm, IDM_BOX_L, (box_size == 4) ? MF_CHECKED : MF_UNCHECKED);
					CheckMenuItem(hm, IDM_BOX_M, (box_size == 2) ? MF_CHECKED : MF_UNCHECKED);
					CheckMenuItem(hm, IDM_BOX_S, (box_size == 1) ? MF_CHECKED : MF_UNCHECKED);
					redraw_terrain();
					return 0;

				case IDM_BOX_S:
					hm = GetMenu(main_window);
					box_size = 1;
					CheckMenuItem(hm, IDM_BOX_L, (box_size == 4) ? MF_CHECKED : MF_UNCHECKED);
					CheckMenuItem(hm, IDM_BOX_M, (box_size == 2) ? MF_CHECKED : MF_UNCHECKED);
					CheckMenuItem(hm, IDM_BOX_S, (box_size == 1) ? MF_CHECKED : MF_UNCHECKED);
					redraw_terrain();
					return 0;


				case IDM_TEXTURE:
					hm = GetMenu(main_window);
					show_texture = !show_texture;
					CheckMenuItem(hm, IDM_TEXTURE, show_texture ? MF_CHECKED : MF_UNCHECKED);
					redraw_terrain();
					return 0;

				case IDM_REMSHAPE:
					remove_object();
					redraw_terrain();
					update_menu();
					return 0;

				case IDM_PATHATTR:
					set_path_properties(NULL);
					return 0;

				case IDM_ADDPOINT:
					change_tool(14);
					return 0;

				case IDM_DELPOINT:
					path_delete_point();
					redraw_terrain();
					update_menu();
					return 0;

				case IDM_DELPATH:
					path_delete();
					redraw_terrain();
					update_menu();
					return 0;

				case IDM_HELP:
					WinHelp((HWND)hwnd, (LPCSTR)"FST.HLP", (UINT)HELP_KEY, (DWORD)"World Index");
					return 0;
			}
			break;

		case WM_CLOSE:
			if (!changed || (IDCANCEL != save_before_quit(hwnd))) {
				DestroyWindow(hwnd);
			}
			return 0;

		case WM_QUERYENDSESSION:
			if (!changed || (IDCANCEL != save_before_quit(hwnd))) {
				return 1L;
			}
			return 0;

		case WM_DESTROY:
			DeleteObject (
			SetClassWord (hwnd, GCW_HBRBACKGROUND,
			GetStockObject (WHITE_BRUSH))) ;
			PostQuitMessage (0);
			return 0;
	}
	return DefWindowProc (hwnd, message, wParam, lParam);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
static int request_value(HWND hwnd, char *message, int *pval)
{
	FARPROC lpProc;
	int ok;

	ok = FALSE;
	lpProc = MakeProcInstance(db_value_proc, appInstance);
	strcpy(dialog_text, message);
	dialog_value = *pval;
	if (DialogBox(appInstance, "DIALOG_4", hwnd, lpProc)) {
		*pval = dialog_value;
		ok = TRUE;
	}
	FreeProcInstance(lpProc);
	return ok;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
static BOOL FAR PASCAL db_value_proc(HWND hdlg, unsigned msg, WORD wParam, LONG lParam)
{
	char s[80];

	lParam = lParam;
	switch(msg) {

		case WM_INITDIALOG:
			SetDlgItemText(hdlg, ID_DB4_TEXT, dialog_text);
			sprintf(s, "%d", dialog_value);
			SetDlgItemText(hdlg, ID_DB4_VALUE, s);
			return(TRUE);

		case WM_COMMAND:
			switch (wParam) {

				case IDOK:
					EndDialog(hdlg, TRUE);
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

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
static BOOL FAR PASCAL db_tod_proc(HWND hdlg, unsigned msg, WORD wParam, LONG lParam)
{
	HWND hCtrl;
	char s[80];
	static int old_tod, old_auto;

	lParam = lParam;
	switch(msg) {

		case WM_INITDIALOG:
			old_tod  = time_of_day;
			old_auto = automatic_colours;
			sprintf(s, "%02d:%02d", time_of_day/60, time_of_day%60);
			SetDlgItemText(hdlg, ID_DB_VALUE2, s);
			CheckDlgButton(hdlg, ID_DB_VALUE1, automatic_colours);
			hCtrl = GetDlgItem(hdlg, ID_DB_VALUE);
			SetScrollRange(hCtrl, SB_CTL, 0, 144, FALSE);
			SetScrollPos(hCtrl, SB_CTL, time_of_day/10, TRUE);
			return TRUE;

		case WM_HSCROLL:
			switch (wParam) {

				case SB_LINEUP:
					time_of_day -= 10;
					break;

				case SB_LINEDOWN:
					time_of_day += 10;
					break;

				case SB_PAGEUP:
					time_of_day -= 60;
					break;

				case SB_PAGEDOWN:
					time_of_day += 60;
					break;

				case SB_THUMBPOSITION:
					time_of_day = LOWORD(lParam) * 10;
					break;

				//case SB_THUMBTRACK:
					//return DefWindowProc ((HWND)hdlg, (UINT)msg, (WPARAM)wParam, (LPARAM)lParam);
			}
			hCtrl = GetDlgItem(hdlg, ID_DB_VALUE);
			if (time_of_day > 1439) {
				time_of_day = 1439;
			}
			if (time_of_day < 0) {
				time_of_day = 0;
			}
			SetScrollPos(hCtrl, SB_CTL, time_of_day/10, TRUE);
			sprintf(s, "%02d : %02d", time_of_day/60, time_of_day%60);
			SetDlgItemText(hdlg, ID_DB_VALUE2, s);
			return 0;

		case WM_COMMAND:
			switch (wParam) {

				case ID_DB_VALUE1:
					automatic_colours = !automatic_colours;
					CheckDlgButton(hdlg, ID_DB_VALUE1, automatic_colours);
					break;

				case IDOK:
					EndDialog(hdlg, TRUE);
					return TRUE;

				case IDCANCEL:
					time_of_day = old_tod;
					automatic_colours = old_auto;
					EndDialog(hdlg, FALSE);
					return TRUE;
			}
			break;
	}
	return FALSE;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
static BOOL FAR PASCAL db_weather_proc(HWND hdlg, unsigned msg, WORD wParam, LONG lParam)
{
	BOOL translated;
	int spd, drn;

	lParam = lParam;
	switch(msg) {
		case WM_INITDIALOG:
			SetDlgItemInt(hdlg, ID_DB_VALUE,  wind_speed, TRUE);
			SetDlgItemInt(hdlg, ID_DB_VALUE1, wind_dirn,  TRUE);
			return(TRUE);

		case WM_COMMAND:
			switch (wParam) {

				case IDOK:
					spd = GetDlgItemInt(hdlg, ID_DB_VALUE, &translated, TRUE);
					if (spd >= 0 && spd <= 100) {
						wind_speed = spd;
					}
					drn = GetDlgItemInt(hdlg, ID_DB_VALUE1, &translated, TRUE);
					if (drn >= 0 && drn <= 360) {
						wind_dirn  = drn;
					}
					EndDialog(hdlg, TRUE);
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

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Get file from
static BOOL FAR PASCAL db_getfile_proc(HWND hdlg, unsigned msg, WORD wParam, LONG lParam)
{
	//char s[80];

	lParam = lParam;
	switch(msg) {

		case WM_INITDIALOG:
			SetDlgItemText(hdlg, ID_DB4_TEXT, dialog_text);
			DlgDirList(hdlg, dialog_file, ID_DB5_LIST, 0, DDL_READWRITE);
			return(TRUE);

		case WM_COMMAND:
			switch (wParam) {

				case IDOK:
					DlgDirSelect(hdlg, dialog_file, ID_DB5_LIST);
					EndDialog(hdlg, TRUE);
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

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
static BOOL FAR PASCAL db_height_proc(HWND hdlg, unsigned msg, WORD wParam, LONG lParam)
{
	int r;
	char s[80];

	lParam = lParam;
	switch(msg) {

		case WM_INITDIALOG:
			sprintf(s, "%d", edit_height);
			SetDlgItemText(hdlg, ID_DB7_HEIGHT, s);
			if (edit_delta) {
				CheckRadioButton(hdlg, ID_DB7_ADD, ID_DB7_SET, ID_DB7_ADD);
			}
			else {
				CheckRadioButton(hdlg, ID_DB7_ADD, ID_DB7_SET, ID_DB7_SET);
			}
			sprintf(s, "%d", edit_radius);
			SetDlgItemText(hdlg, ID_DB7_RADIUS, s);
			return(TRUE);

		case WM_COMMAND:
			switch (wParam) {

				case IDOK:
					GetDlgItemText(hdlg,ID_DB7_HEIGHT,s,80);
					edit_height = atoi(s);
					GetDlgItemText(hdlg,ID_DB7_RADIUS,s,80);
					r = atoi(s);
					edit_radius = (r > 50) ? 50 : (r < 0) ? 0 : r;
					if (IsDlgButtonChecked(hdlg, ID_DB7_ADD)) {
						edit_delta = TRUE;
					}
					else {
						edit_delta = FALSE;
					}
					EndDialog(hdlg, TRUE);
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

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Dialog proc for default values used when creating a new object
static BOOL FAR PASCAL db_grid_proc(HWND hdlg, unsigned msg, WORD wParam, LONG lParam)
{
	int   grid;
	long lr;
	char s[40];

	lParam = lParam;
	switch(msg) {

		case WM_INITDIALOG:
			SendDlgItemMessage(hdlg, ID_DB_VALUE, CB_ADDSTRING, 0, (LPARAM)((LPSTR)"1 M"));
			SendDlgItemMessage(hdlg, ID_DB_VALUE, CB_ADDSTRING, 0, (LPARAM)((LPSTR)"10 M"));
			SendDlgItemMessage(hdlg, ID_DB_VALUE, CB_ADDSTRING, 0, (LPARAM)((LPSTR)"100 M"));
			sprintf(s, "%d M", edit_grid_size);
			lr = SendDlgItemMessage(hdlg, ID_DB_VALUE, CB_SELECTSTRING, -1, (LPARAM)s);
			if (lr == CB_ERR) {
				// current grid size is not one of defaults
				SendDlgItemMessage(hdlg, ID_DB_VALUE, CB_ADDSTRING, 0, (LPARAM)((LPSTR)s));
				SendDlgItemMessage(hdlg, ID_DB_VALUE, CB_SELECTSTRING, -1, (LPARAM)s);
			}
			return(TRUE);

		case WM_COMMAND:
			switch (wParam) {

				case IDOK:
					GetDlgItemText(hdlg, ID_DB_VALUE, s, 16);
					sscanf(s, "%d M", &grid);
					if (grid >= 1 && grid <= 1000) {
						edit_grid_size = grid;
					}
					EndDialog(hdlg, TRUE);
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

extern long grid_size;

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
static BOOL FAR PASCAL db_grid_size_proc(HWND hdlg, unsigned msg, WORD wParam, LONG lParam)
{
	int  grid;
	long lr;
	char s[40];

	lParam = lParam;
	switch(msg) {

		case WM_INITDIALOG:
			SendDlgItemMessage(hdlg, ID_DB_VALUE, CB_ADDSTRING, 0, (LPARAM)((LPSTR)"500 M"));
			SendDlgItemMessage(hdlg, ID_DB_VALUE, CB_ADDSTRING, 0, (LPARAM)((LPSTR)"250 M"));
			SendDlgItemMessage(hdlg, ID_DB_VALUE, CB_ADDSTRING, 0, (LPARAM)((LPSTR)"100 M"));
			sprintf(s, "%d M", grid_size);
			lr = SendDlgItemMessage(hdlg, ID_DB_VALUE, CB_SELECTSTRING, -1, (LPARAM)s);
			if (lr == CB_ERR) {
				//current grid size is not one of defaults
				SendDlgItemMessage(hdlg, ID_DB_VALUE, CB_ADDSTRING, 0, (LPARAM)((LPSTR)s));
				SendDlgItemMessage(hdlg, ID_DB_VALUE, CB_SELECTSTRING, -1, (LPARAM)s);
			}
			return(TRUE);

		case WM_COMMAND:
			switch (wParam) {

				case IDOK:
					GetDlgItemText(hdlg, ID_DB_VALUE, s, 16);
					sscanf(s, "%d M", &grid);
					if (grid >= 100 && grid <= 1000) {
						grid_size = grid;
					}
					EndDialog(hdlg, TRUE);
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

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
static BOOL FAR PASCAL db_about_proc(HWND hAbout, unsigned msg, WORD wParam, LONG lParam)
{
	lParam = lParam;
	switch(msg) {

		case WM_INITDIALOG:
			// Updates 'Adout' dialog to current version number
			SetDlgItemText(hAbout, ID_DB_VALUE, prog_version);
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
void warning(char *message)
{
	MessageBox(NULL, "FST WORLD", message, MB_ICONEXCLAMATION);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void stop(char *message)
{
	if (message != NULL) {
		warning(message);
	}
	PostQuitMessage(0);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
//  View application interface
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

extern BOOL view_active = FALSE;

static UINT viewload_mssg  = 0;
static UINT viewexit_mssg  = 0;

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void open_view_window(void)
{
	if (!view_active) {
		save_view_data(tmp_filename("~TERVIEW.TMP"));
		WinExec("terview", SW_SHOW);
		view_active = TRUE;
	}
	else {
		save_view_data(tmp_filename("~TERVIEW.TMP"));
		PostMessage(HWND_BROADCAST, viewload_mssg, NULL, NULL);
	}
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Window setup routines
static BOOL init_window(HANDLE hInstance)
{
	WNDCLASS wndclass;

	wndclass.style		   = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc   = WndProc;
	wndclass.cbClsExtra	   = 0;
	wndclass.cbWndExtra	   = 0;
	wndclass.hInstance	   = hInstance;
	wndclass.hIcon		   = NULL;	//LoadIcon (NULL, IDI_APPLICATION);
	wndclass.hCursor	   = LoadCursor (NULL, IDC_ARROW);
	wndclass.hbrBackground = CreateSolidBrush (RGB(192, 192, 192));
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
	if (_argc == 3) {
		sscanf(_argv[1], "%ld", &start_x);
		sscanf(_argv[2], "%ld", &start_z);
	}
	else if (_argc == 5) {
		sscanf(_argv[1], "%ld", &start_x);
		sscanf(_argv[2], "%ld", &start_z);
		sscanf(_argv[3], "%d", &longitude);
		sscanf(_argv[4], "%d", &latitude);
	}
	setup_terrain();
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void tidyup_program(void)
{
	tidyup_terrain();
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Entry point: same as normal C main
int PASCAL WinMain (HANDLE hInstance, HANDLE hPrevInstance, LPSTR lpszCmdParam, int nCmdShow)
{
	MSG	msg;
	//char cmd[80];
	char main_title[256];

	lpszCmdParam = lpszCmdParam;

	if (hPrevInstance) {
		MessageBox(NULL, "Can not start more than one copy of World!", "Cannot run program!", MB_ICONEXCLAMATION);
		return FALSE;
	}

	if (!init_window(hInstance)) {
		return FALSE;
	}

	appInstance = hInstance;

	setup_program();

	sprintf(main_title, "FST World - %s", proj_path);

	main_window = create_window(hInstance, main_title, FORM_MAX_WIDTH, FORM_MAX_HEIGHT);

	create_control_window(hInstance, main_window);
	create_edit_window(hInstance, main_window);

	ShowWindow (main_window, nCmdShow);
	UpdateWindow (main_window);

	update_menu();

	// View application Symbolic messages
	viewload_mssg = RegisterWindowMessage("FST_TERVIEWLOAD");	// Tell view to reload
	viewexit_mssg = RegisterWindowMessage("FST_TERVIEWEXIT");	// view has closed

	while (GetMessage (&msg, NULL, 0, 0)) {
		if (msg.message == viewexit_mssg) {
			view_active = FALSE;
			//force_redraw_control();
		}
		else {
			TranslateMessage (&msg);
			DispatchMessage (&msg);
		}
	}

	tidyup_program();

	if (UnregisterClass(szControlName, appInstance) == 0) {
		warning("Could not unregister 'CONTROL' class");
	}
	if (UnregisterClass(szEditName, appInstance) == 0) {
		warning("Could not unregister 'EDIT' class");
	}
	if (UnregisterClass(szAppName, appInstance) == 0) {
		warning("Could not unregister 'MAIN' class");
	}
	return msg.wParam;
}

