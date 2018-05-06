//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// FST Model Editor & Entry Point
// Copyright SIMIS Holdings, Ltd. 1995, 1996, 1997, 1998
//——————————————————————————————————————————————————————————————————————————————————————————————————————————————

#include <stdio.h>
#include <string.h>
#include <dir.h>

#include "defs.h"
#include "model.h"

HWND main_window;
HANDLE appInstance = NULL;

long FAR PASCAL _export WndProc (HWND, UINT, UINT, LONG);

static BOOL FAR PASCAL db_getfile_proc(HWND hdlg, unsigned msg, WORD wParam, LONG lParam);
static BOOL FAR PASCAL db_save_proc(HWND hAbout, unsigned msg, WORD wParam, LONG lParam);
static BOOL FAR PASCAL db_about_proc(HWND hAbout, unsigned msg, WORD wParam, LONG lParam);

static void tidyup_program(void);

static char szAppName[] = "'FST-98' Model Editor";

//static int vscroll_pos = 0;
//static int hscroll_pos = 0;

static char dialog_file[128];
static char current_file[128] = "*.FMD";

static char proj_path[128];				// full path name for project

int changed = FALSE;

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
static short save_before_quit(HWND hwnd)
{
	char  szBuffer [64] ;
	short nReturn ;

	wsprintf (szBuffer, "Save current changes ?");

	nReturn = MessageBox (hwnd, szBuffer, "FST Model", MB_YESNOCANCEL | MB_ICONQUESTION) ;

	if (nReturn == IDYES) {
		if (!SendMessage (hwnd, WM_COMMAND, IDM_SAVEAS, 0L)) {
			nReturn = IDCANCEL ;
		}
	}
	return nReturn ;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static BOOL check_filename(char *name)
{
	char drive[MAXDRIVE];
	char directory[MAXPATH];
	char file[9];
	char ext[MAXEXT];

	if (name[0] == '*') {
   	return FALSE;
	}

	fnsplit(name, drive, directory, file, ext);
	strcpy(ext, ".fmd");
	fnmerge(name, drive, directory, file, ext);

	return TRUE;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void update_window_title(char *name)
{
	char text[250];

	sprintf(text, "'FST-98' Model:  %s", name);
	SetWindowText(main_window, text);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Windows event handler
long FAR PASCAL _export WndProc (HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
	HDC hdc;
	PAINTSTRUCT ps;
	RECT rect;
	FARPROC lpProc;
	//int temp, scroll_dx, scroll_dy;

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

		case WM_RBUTTONDOWN:
			mouse_pressed(hwnd, LOWORD(lParam), HIWORD(lParam), 0);
			return 0;

		case WM_RBUTTONUP:
			mouse_released(hwnd, LOWORD(lParam), HIWORD(lParam), 0);
			return 0;

		case WM_RBUTTONDBLCLK:
			mouse_double_clicked(hwnd, LOWORD(lParam), HIWORD(lParam), 0);
			return 0;

		case WM_LBUTTONDOWN:
			mouse_pressed(hwnd, LOWORD(lParam), HIWORD(lParam), 1);
			return 0;

		case WM_LBUTTONUP:
			mouse_released(hwnd, LOWORD(lParam), HIWORD(lParam), 1);
			return 0;

		case WM_LBUTTONDBLCLK:
			mouse_double_clicked(hwnd, LOWORD(lParam), HIWORD(lParam), 1);
			return 0;

		case WM_SIZE:
			resize_client(hwnd, LOWORD(lParam), HIWORD(lParam));
			hdc = BeginPaint (hwnd, &ps);
			GetClientRect (hwnd, &rect);
			redraw_client(hdc, &rect);
			EndPaint (hwnd, &ps);
			return 0;

/*
		case WM_HSCROLL:
			scroll_dx = 0;
			switch (wParam) {

				case SB_LINEUP:
            	scroll_dx = -4;
               break;

				case SB_LINEDOWN:
            	scroll_dx = 4;
               break;

				case SB_PAGEUP:
            	scroll_dx = -20;
               break;

				case SB_PAGEDOWN:
            	scroll_dx = 20;
               break;

				case SB_THUMBPOSITION:
					hscroll_pos = LOWORD(lParam);
					SetScrollPos(hwnd, SB_HORZ, hscroll_pos, TRUE);
					scroll_client(hwnd, TRUE, hscroll_pos, vscroll_pos);
					break;

				case SB_THUMBTRACK:
					return DefWindowProc (hwnd, message, wParam, lParam);
			}
			temp = hscroll_pos;
			hscroll_pos += scroll_dx;
			if (hscroll_pos < 0) {
				hscroll_pos = 0;
				scroll_dx = hscroll_pos - temp;
			}
			if (hscroll_pos >= model_x_size) {
				hscroll_pos = model_x_size - 1;
				scroll_dx = hscroll_pos - temp;
			}
			if (scroll_dx != 0) {
				ScrollWindow(hwnd, -scroll_dx, 0, NULL, NULL);
				SetScrollPos(hwnd, SB_HORZ, hscroll_pos, TRUE);
				scroll_client(hwnd, FALSE, hscroll_pos, vscroll_pos);
			}
			return 0;
*/
/*
		case WM_VSCROLL:
			scroll_dy = 0;
			switch (wParam) {
				case SB_LINEUP: scroll_dy = -4; break;
				case SB_LINEDOWN: scroll_dy = 4; break;
				case SB_PAGEUP: scroll_dy = -20; break;
				case SB_PAGEDOWN: scroll_dy = 20; break;
				case SB_THUMBPOSITION:
					vscroll_pos = LOWORD(lParam);
					SetScrollPos(hwnd, SB_VERT, vscroll_pos, TRUE);
					scroll_client(hwnd, TRUE, hscroll_pos, vscroll_pos);
					break;
				case SB_THUMBTRACK:
					return DefWindowProc (hwnd, message, wParam, lParam);
			}
			temp = vscroll_pos;
			vscroll_pos += scroll_dy;
			if (vscroll_pos < 0) {
				vscroll_pos = 0;
				scroll_dy = vscroll_pos - temp;
			}
			if (vscroll_pos >= model_y_size) {
				vscroll_pos = model_y_size - 1;
				scroll_dy = vscroll_pos - temp;
			}
			if (scroll_dy != 0) {
				ScrollWindow(hwnd, 0, -scroll_dy, NULL, NULL);
				SetScrollPos(hwnd, SB_VERT, vscroll_pos, TRUE);
				scroll_client(hwnd, FALSE, hscroll_pos, vscroll_pos);
			}
			return 0;
*/
		case WM_CREATE:
			create_client(hwnd);
			//SetScrollRange(hwnd, SB_VERT, 0, model_y_size, FALSE);
			//setScrollRange(hwnd, SB_HORZ, 0, model_x_size, FALSE);
			return 0;

		case WM_COMMAND:
			switch (wParam) {

				case IDM_NEW:
					return 0;

				case IDM_LOAD:
					if (!changed || IDCANCEL != save_before_quit(hwnd)) {
						lpProc = MakeProcInstance(db_getfile_proc, appInstance);
						strcpy(dialog_file, proj_filename("*.FMD"));
						if (DialogBox(appInstance, "DIALOG_5", hwnd, lpProc)) {
							load_model(dialog_file);
							redraw_client(hdc, &rect);
							strcpy(current_file, dialog_file);
							update_window_title(current_file);
							changed = FALSE;
						}
						FreeProcInstance(lpProc);
						GetClientRect (hwnd, &rect);
						InvalidateRect(hwnd, NULL, FALSE);
						UpdateWindow(hwnd);
					}
					return 0;

				case IDM_SAVE:
					if ((strlen(current_file) <= 0) || current_file[0] == '*') {
						lpProc = MakeProcInstance(db_save_proc, appInstance);
						strcpy(dialog_file, current_file);
						if (DialogBox(appInstance, "DIALOG_14", hwnd, lpProc)) {
							if (check_filename(dialog_file)) {
									strcpy(current_file, dialog_file);
									save_model(dialog_file);
									update_window_title(current_file);
									changed = FALSE;
							}
						}
						FreeProcInstance(lpProc);
					}
					else {
						save_model(current_file);
						changed = FALSE;
					}
					return 0;

				case IDM_SAVEAS:
					lpProc = MakeProcInstance(db_save_proc, appInstance);
					strcpy(dialog_file, current_file);
					if (DialogBox(appInstance, "DIALOG_14", hwnd, lpProc)) {
						if (check_filename(dialog_file)) {
							strcpy(current_file, dialog_file);
							save_model(dialog_file);
							update_window_title(current_file);
							changed = FALSE;
						}
					}
					FreeProcInstance(lpProc);
					return 0;

				case IDM_EXIT:
					if (!changed || IDCANCEL != save_before_quit(hwnd)) {
						PostQuitMessage (0);
               }
					return 0;

				case IDM_HELP:
					WinHelp(hwnd,"FST.HLP", HELP_KEY, "MODEL INDEX");
					return 0;

				case IDM_ABOUT:
					lpProc = MakeProcInstance(db_about_proc, appInstance);
					DialogBox(appInstance, "DIALOG_1", hwnd, lpProc);
					FreeProcInstance(lpProc);
					return 0;

				case IDM_EDIT1:
					edit_properties();
					return 0;

				case IDM_EDIT2:
					edit_engines();
					return 0;

				//case IDM_EDIT3:
				//	edit_canvas();
				//	return 0;

				case IDM_FILLED:
					edit_filled(hwnd);
					return 0;

				case IDM_VERTEX:
					edit_vertex(hwnd);
					return 0;

				case IDM_GRIDS:
					edit_grids(hwnd);
					return 0;

				case IDM_ZOOM1:
					change_zoom(hwnd, 1);
					return 0;

				case IDM_ZOOM2:
					change_zoom(hwnd, 2);
					return 0;

				case IDM_ZOOM3:
					change_zoom(hwnd, 3);
					return 0;

				case IDM_ZOOM4:
					change_zoom(hwnd, 4);
					return 0;

				case IDM_ZOOM5:
					change_zoom(hwnd, 5);
					return 0;
			}
			break;

		case WM_CLOSE:
			if (!changed || IDCANCEL != save_before_quit(hwnd)) {
				DestroyWindow(hwnd);
			}
			return 0;

		case WM_QUERYENDSESSION:
			if (!changed || IDCANCEL != save_before_quit(hwnd)) {
				return 1L;
			}
			return 0;

		case WM_DESTROY:
			PostQuitMessage (0);
			return 0;
	}
	return DefWindowProc (hwnd, message, wParam, lParam);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Get file from
static BOOL FAR PASCAL db_getfile_proc(HWND hdlg, unsigned msg, WORD wParam, LONG lParam)
{
	lParam = lParam;

	switch(msg) {

		case WM_INITDIALOG:
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

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static BOOL FAR PASCAL db_save_proc(HWND hdlg, unsigned msg, WORD wParam, LONG lParam)
{
	char dir_spec[128];

	lParam = lParam;
	switch(msg) {

		case WM_INITDIALOG:
			sprintf(dir_spec, "%s%s", proj_path, "*.fmd");
			SetDlgItemText(hdlg, ID_DB2_FILE, dialog_file);
			DlgDirList(hdlg, ((LPSTR)dir_spec), ID_DB2_LIST, 0, DDL_READWRITE);
			return(TRUE);

		case WM_COMMAND:
			switch (wParam) {

				case ID_DB2_LIST:
					DlgDirSelect(hdlg, dialog_file, ID_DB2_LIST);
					SetDlgItemText(hdlg, ID_DB2_FILE, dialog_file);
					return TRUE;

				case IDOK:
					GetDlgItemText(hdlg,ID_DB2_FILE, dialog_file,80);
					EndDialog(hdlg, TRUE);
					return(TRUE);

				case IDCANCEL:
					EndDialog(hdlg, FALSE);
					return(TRUE);
			}
			break;
	}
	return(FALSE);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static BOOL FAR PASCAL db_about_proc(HWND hAbout, unsigned msg, WORD wParam, LONG lParam)
{
	lParam = lParam;
	switch(msg) {

		case WM_INITDIALOG:
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

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void warning(char *message)
{
	MessageBox(NULL, message, "FST Model:  Warning!", MB_ICONEXCLAMATION);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void stop(char *message)
{
	if (message != NULL) {
		MessageBox(NULL, message, "FST Model:  Program Stopped!", MB_ICONSTOP);
	}
	PostQuitMessage(0);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Window setup routines
static BOOL init_window(HANDLE hInstance)
{
	WNDCLASS wndclass;

	wndclass.style			= CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wndclass.lpfnWndProc	= WndProc;
	wndclass.cbClsExtra		= 0;
	wndclass.cbWndExtra		= 0;
	wndclass.hInstance		= hInstance;
	wndclass.hIcon			= NULL;
	wndclass.hCursor		= LoadCursor (NULL, IDC_CROSS);
	wndclass.hbrBackground	= NULL;
	wndclass.lpszMenuName	= "MENU_1";
	wndclass.lpszClassName	= szAppName;

	return RegisterClass (&wndclass);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static HWND create_window(HANDLE hInstance, char *caption, int width, int height)
{
	HWND hwnd;
	int x, y, xDesktop, yDesktop;

	xDesktop = GetSystemMetrics(SM_CXSCREEN);
	yDesktop = GetSystemMetrics(SM_CYSCREEN);

	x = (xDesktop - width ) / 2;
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

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void setup_program(void)
{
	FILE *fp;
	char buff[128];

	fp = fopen("project.dat", "r");
	if (fp) {
		fscanf(fp, "%s\n", proj_path);
		fclose(fp);
	}
	else {
		if (getcwd(buff, 128)) {
			sprintf(proj_path, "%s\\", buff);
		}
	}
	setup_model();
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void tidyup_program(void)
{
	tidyup_model();
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Entry point: same as normal C main
int PASCAL WinMain (HANDLE hInstance, HANDLE hPrevInstance, LPSTR lpszCmdParam, int nCmdShow)
{
	HWND	hwnd;
	MSG	msg;

	lpszCmdParam = lpszCmdParam;
	if (!hPrevInstance && !init_window(hInstance)) {
		return FALSE;
	}

	appInstance = hInstance;

	setup_program();

	hwnd = create_window(hInstance, "'FST-98' Model Editor", 320, 440);

	main_window = hwnd;

	ShowWindow (hwnd, nCmdShow);
	UpdateWindow (hwnd);

	while (GetMessage (&msg, NULL, 0, 0))
	{
		TranslateMessage (&msg);
		DispatchMessage (&msg);
	}

	tidyup_program();

	return msg.wParam;
}

