/* FST Terrain Editor */
/* Terrain control panel */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "defs.h"

int edit_mode = OBJECT_MODE;
int current_tool = 9, last_tool = 9;

static HWND control_window = NULL;
static HBITMAP hbm_buttons, hbm_old_buttons;
static HBITMAP hbm_buttons1;
static HDC hdc_buttons;

long FAR PASCAL _export WndProc_control(HWND hwnd, UINT message, UINT wParam, LONG lParam);

static void mouse_down(HWND hwnd, int wx, int wy);
static void redraw_toolbar(HDC hdc, RECT *rect);

extern void show_mouse_coords(int wx, int wy);

//static int potential_tool(int wx, int wy);

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static BOOL init_control_window(HANDLE hInstance)
{
	WNDCLASS wndclass;

	wndclass.style			= 0;
	wndclass.lpfnWndProc	= WndProc_control;
	wndclass.cbClsExtra		= 0;
	wndclass.cbWndExtra		= 0;
	wndclass.hInstance		= hInstance;
	wndclass.hIcon			= LoadIcon (NULL, IDI_APPLICATION);
	wndclass.hCursor		= LoadCursor (NULL, IDC_ARROW);
	wndclass.lpszMenuName	= NULL;
	wndclass.lpszClassName	= szControlName;

	return RegisterClass (&wndclass);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
HWND create_control_window(HANDLE hInstance, HWND parent)
{
	if (!init_control_window(hInstance)) {
		return NULL;
	}
	control_window = CreateWindow (
		szControlName,				// window class name
		"",           				// window caption
		WS_CHILD | WS_VISIBLE,		// window style
		3,						// initial x position
		1,						// initial y position
		45,						// initial x size
		181,						// initial y size
		parent,					// parent window handle
		2002,					// window menu handle
		hInstance,				// program instance handle
		NULL);					// creation parameters
	return control_window;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Control window event handler
long FAR PASCAL _export WndProc_control(HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
	HDC hdc;
	PAINTSTRUCT ps;
	RECT rect;

	switch (message) {

		case WM_PAINT:
			hdc = BeginPaint (hwnd, &ps);
			GetClientRect (hwnd, &rect);
			redraw_toolbar(hdc, &rect);
			EndPaint (hwnd, &ps);
			return 0;

		case WM_LBUTTONDOWN:
			mouse_down(hwnd, LOWORD(lParam), HIWORD(lParam));
			return 0;

		case WM_MOUSEMOVE:
			show_mouse_coords(LOWORD(lParam), HIWORD(lParam));
			//show_status_line(potential_tool(LOWORD(lParam), HIWORD(lParam)));
			return 0;

		case WM_CREATE:
			hbm_buttons		= LoadBitmap(appInstance, "BITMAP_1");
			hbm_buttons1		= LoadBitmap(appInstance, "BITMAP_2");
			hdc				= GetDC(hwnd);
			hdc_buttons		= CreateCompatibleDC(hdc);
			hbm_old_buttons	= SelectObject(hdc_buttons, hbm_buttons1);
			ReleaseDC(hwnd, hdc);
			return 0;

		case WM_DESTROY:
			SelectObject(hdc_buttons, hbm_old_buttons);
			DeleteDC(hdc_buttons);
			DeleteObject(hbm_buttons);
			DeleteObject(hbm_buttons1);
			return 0;
	}
	return DefWindowProc (hwnd, message, wParam, lParam);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Inverts button highliting so button looks like it is pressed
static void draw_hilight(HDC hdc, int x, int y)
{
	HPEN old_pen;

	old_pen = SelectObject(hdc, GetStockObject(BLACK_PEN));

	SelectObject(hdc, CreatePen(PS_SOLID, 0, RGB(128, 128, 128)));
	MoveTo(hdc, x +  1, y +  1);
	LineTo(hdc, x + 20, y +  1);
	MoveTo(hdc, x +  1, y +  2);
	LineTo(hdc, x +  1, y + 21);

	SelectObject(hdc, CreatePen(PS_SOLID, 0, RGB(255, 255, 255)));
	MoveTo(hdc, x + 20, y +  2);
	LineTo(hdc, x + 20, y + 21);
	MoveTo(hdc, x +  2, y + 20);
	LineTo(hdc, x + 20, y + 20);

	DeleteObject(SelectObject(hdc, old_pen));
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Covers button face will grey filled poly
static void draw_blank(HDC hdc, int x, int y)
{
	HBRUSH brush;
	RECT rect;

	brush = CreateSolidBrush(RGB(192, 192, 192));
	rect.left   = x +  2;
	rect.right  = x + 20;
	rect.top    = y +  2;
	rect.bottom = y + 20;
	FillRect(hdc, &rect, brush);
	DeleteObject(brush);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void redraw_toolbar(HDC hdc, RECT *rect)
{
	int x, y;

	if (current_tool & 1) {
		x = rect->left;
	}
	else {
		x = rect->left + 21;
	}

	BitBlt(hdc, rect->left, rect->top, 43, 200, hdc_buttons, 0, 0, SRCCOPY);

	if (current_tool <= 8) {
		y = rect->top + (((current_tool - 1) / 2) * 21);
	}
	else {
		y = rect->top + 96 + (((current_tool - 9) / 2) * 21);
	}

	// Draw hilight makes buttons appear to be pressed down
	draw_hilight(hdc, x, y);

	if (edit_mode == TERRAIN_MODE) {
		draw_hilight(hdc, rect->left, 0);
	}
	else if (edit_mode == OBJECT_MODE) {
		draw_hilight(hdc, rect->left + 21, 0);
	}

	if (edit_mode == OBJECT_MODE) {
		if (!selected_object_associable()) {
			draw_blank(hdc, rect->left, rect->top + (5 * 21) + 12);
			draw_blank(hdc, rect->left + 21, rect->top + (5 * 21) + 12);
		}
		if (!is_copy_object()) {
			draw_blank(hdc, rect->left + 21, rect->top + (7 * 21) + 12);
		}
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void set_terrain_mode(void)
{
	edit_mode = TERRAIN_MODE;
	SelectObject(hdc_buttons, hbm_buttons);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void set_object_mode(void)
{
	edit_mode = OBJECT_MODE;
	SelectObject(hdc_buttons, hbm_buttons1);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
char *tool_name(int tool)
{
	static char *share_names[]  = {"-"          , "TERRAIN MODE" , "OBJECT MODE", "Zoom In"   , "Zoom Out"   , "Redraw"    , "View"        , "x", "No Function"};
	static char *terrain_name[] = {"Raise Point", "Depress Point", "Raise Area" , "Lower Area", "Raise Line" , "Lower Line", "Fractalize"  , "Release Button"};
	static char *object_names[] = {"Select Item", "Add Item"     , "Link"       , "Break Link", "Select Path", "New Path"  , "Insert Point", "Paste Active"};

	if (edit_mode == TERRAIN_MODE) {
		share_names[7] = "Ground Texture";
	}
	else {
		share_names[7] = "No Function";
	}

	if (tool > 0 && tool < 9) {
		return share_names[tool];
	}


	if (edit_mode == TERRAIN_MODE) {
		return terrain_name[tool - 9];
	}
	else if (edit_mode == OBJECT_MODE) {
		return object_names[tool - 9];
	}
	return share_names[0];
}

/*
//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static int potential_tool(int wx, int wy)
{
	int button;

	// Groups - first eight are mode independent, next n are mode buttons, gap of 12 pixels
	button = 0;
	if (wx < 43) {
		if (wy < 84) {
			button = ((wy / 21) * 2) + 1;
		}
		else {
			button = (((wy - 96) / 21) * 2) + 9;
		}
		if (wx > 21) {
			button++;
		}
	}
	return button;
}
*/

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Returns new button if it becomes current tool else returns 0
static int select_tool(int button)
{
	int tool;

	tool = 0;
	switch (button) {

		case 1:
			set_terrain_mode();
			tool = 16;
			break;

		case 2:
			set_object_mode();
			tool = 9;
			break;

		case 3: // Zoom in
			return button;

		case 4: // Zoom out
			zoom_out();
			break;

		case 5:
			redraw_terrain();
			break;

          case 6:
			open_view_window();
			break;

		case 7:
			if (edit_mode == TERRAIN_MODE) {
				redraw_terrain();
				return button;
			}
			break;
	}
	return tool;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static int select_terrain_tool(int button)
{
	switch (button) {
		default:
		return button;
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static int select_object_tool(int button)
{
	switch (button) {

		case TOOL_PTHNEW:
			reset_path_selections();
			return button;

		case TOOL_OBJLNK:
		case TOOL_OBJBRK:
			if (selected_object_associable()) {
				reset_path_selections();
				return button;
			}
			break;

		case TOOL_OBJPST:
			if (is_copy_object()) {
				return button;
			}
			break;

		default:
			return button;
	}
	return 0;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void toolbar_pressed(int wx, int wy)	//, RECT *rect)
{
	int button;

	//Groups - first eight are mode independent, next n are mode buttons, gap of 12 pixels
	button = 0;
	if (wx < 43) {
		if (wy < 84) {
			button = ((wy / 21) * 2) + 1;
		}
		else {
			button = (((wy - 96) / 21) * 2) + 9;
		}
		if (wx > 21) {
			button++;
		}
	}

	if (button > 0 && button < 9) {
		button = select_tool(button);
	}
	else {
		if (edit_mode == OBJECT_MODE) {
			button = select_object_tool(button);
		}
		else {
			button = select_terrain_tool(button);
		}
	}
	if (button > 0) {
		if (button != current_tool) {
			last_tool = current_tool;
		}
		current_tool = button;
	}
	update_menu();
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void change_tool(int tool)
{
	RECT rect;
	HWND hwnd;

	current_tool = tool;
	hwnd = control_window;

	GetClientRect (hwnd, &rect);
	InvalidateRect(hwnd, &rect, TRUE);
	UpdateWindow(hwnd);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void force_redraw_toolbar(void)
{
	RECT rect;
	HWND hwnd;

	hwnd = control_window;
	GetClientRect (hwnd, &rect);
	InvalidateRect(hwnd, &rect, TRUE);
	UpdateWindow(hwnd);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void mouse_down(HWND hwnd, int wx, int wy)
{
	RECT rect;

	GetClientRect (hwnd, &rect);
	toolbar_pressed(wx, wy);
	show_status_line(current_tool);

	InvalidateRect(hwnd, &rect, TRUE);
	UpdateWindow(hwnd);
}

