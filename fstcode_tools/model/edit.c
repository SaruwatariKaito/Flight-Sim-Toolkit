//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// FST Model Editor & Aircraft outline editing functions
// Copyright SIMIS Holdings, Ltd. 1995, 1996, 1997, 1998
//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Version:  1.14
//
// Updatad for 'FST-98' May 1998 by W.C. Digiacomo
//
//	 1. If main form is resized, grid autosizes & acft shape recenters
//  2. Enabled Poly fill routines
//  3. Corrected & enhansed speed calculation routines
//  4. Added coordinate display while moving vertex
//  5. Added color change to selected vertex
//  6. Added on/off controls for grid, vertex & poly fill
//  7. Removed 'Blueprint' dialog, vars now fixed
//  8. Added 'Right' double-click to launch engine dialog
//  9. Added new modle definition parameters
// 10. Added menu driven zoom system
//——————————————————————————————————————————————————————————————————————————————————————————————————————————————

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <bios.h>

#include "model.h"
#include "defs.h"

#ifndef ABS
	#define ABS(X) (((X) < 0) ? (-(X)) : (X))
#endif

#define FIN_BASE		180
#define POINT_SIZE	2
#define NUM_POINTS	21
#define MAX_ENGINE	8
#define PROP			1
#define JET				2
#define TURBO			3
#define STD_WEIGHT	1024

#define M(X) ((X) * 100)

#define Bounds(X, MIN, MAX) ((X) = ((X) <= (MIN) ? (MIN) : ((X) >= (MAX) ? (MAX) : (X))))

#define NORMALISE_FORCE(F,W) F = (F * 1024.0) / W

typedef struct {
	int x, y;
} Vector2;

typedef struct {
	int x, y, type;
} Engine;

static int CX1 = 4;
static int CY1 = 4;
static int CX2 = 304;
static int CY2 = 364;
static int SY1	= 368;
static int SY2	= 388;

extern int model_x_size	= 100;
extern int model_y_size	= 100;
extern int changed;

static float zoom_factor	= 1.0;

static int grid_height		= 360;
static int grid_width		= 300;
static int grid_space		= 5;
static int grid_scale		= 5;
static int x_centre			= 200;
static int y_centre			= 150;
static int x_start 			= 0;
static int y_start 			= 0;
static int selected_point	= 0;
static int double_clicke1	= FALSE;
static int double_clicke2	= FALSE;
static int dragging			= FALSE;
static int mouse_down		= FALSE;
static int filled_acft		= TRUE;
static int show_vertex		= TRUE;
static int show_grids 		= TRUE;

static long down_time = 0L;

static Vector2 scaled_points[NUM_POINTS];
static Vector2 last_selected_pos = {0, 0};

static Engine engines[MAX_ENGINE];

static HWND edit_window = NULL;

static HDC draw_hdc = NULL;

static HPEN red_pen, white_pen, black_pen, gray_pen, lgray_pen, dgray_pen, blue_pen;
static HBRUSH blue_brush, gray_brush, lgray_brush, dgray_brush, black_brush;

static void select_point(int wx, int wy);
static void draw_dragged_point(HDC hdc);
static void check_point(int n);
static void move_point(int n, int wx, int wy);
static void check_params(void);

static int calc_top_speed(void);
static int calc_stall_speed(void);

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
typedef struct {
	long		airframe_weight;
	float	airframe_drag;
	float	uc_drag;
	float	airbrake_drag;
	float	dihedral;
	float	incidence;
	float	stall_angle;
	float	lift;
	float	control_power;
	float	roll_inertia;
	float	pitch_inertia;
	float	stealth;
	int		retracts;
	int		flaps;
	int		airbrakes;
	int		biplane;
	int		tail_dragger;
	int		ejector;
	int		auto_piloted;
	long		fuel_weight;
	int		engines;
	int		engine_type;
	int		afterburner;
	int		vectored_thrust;
	int		vector_min;
	int		vector_max;
	int		droptnk_qty;
	long		droptnk_wt;
	long		thrust;
	long		fuelrt_idle;
	long		fuelrt_crz;
	long		fuelrt_mrt;
	long		fuelrt_ab;
	long		thrust_idle;
	long		thrust_crz;
	long		thrust_mrt;
	long		thrust_ab;
	long		egt_idle;
	long		egt_crz;
	long		egt_mrt;
	long		egt_ab;
	int		fuel_dumps;
	int		dump_rate;
	int		jammers;
	int		hi_fsd_speed;
	long		max_ceiling;
	long		restart_alt;
} Props;

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// This section sets the DEFAULT 'Props' values
static Props params = {
	5000L,	// airframe_weight
	2.0,		// airframe_drag
	0.5,		// uc_drag
	1.0,		// airbrake_drag
	0.0,		// dihedral
	2.0,		// incidence
	15.0,	// stall_angle
	1.5,		// lift
	2.0,		// control_power
	1.5,		// roll_inertia
	1.5,		// pitch_inertia
	10.0,	// stealth
	1,		// retracts
	1,		// flaps
	1,		// airbrakes
	0,		// biplane
	0,		// tail_dragger
	1,		// ejector
	0,		// auto_piloted
	1000L,	// fuel_weight
	1,		// engines
	2,		// engine_type
	0,		// afterburner
	0,		// vectored_thrust
	0,		// vector_min
	0,		// vector_max
	2,		// droptnk_qty
	2000L,	// droptnk_wt
	0,		// thrust
	250L,	// fuelrt_idle
	700L,	// fuelrt_crz
	1000L,	// fuelrt_mrt
	1800L,	// fuelrt_ab
	750L,	// thrust_idle
	2100L,	// thrust_crz
	3000L,	// thrust_mrt
	3750L,	// thrust_ab
	400L,	// egt_idle
	600L,	// egt_crz
	800L,	// egt_mrt
	1000L,	// egt_ab
	0,		// fuel_dumps
	0,		// dump_rate
	0,		// jammers
	300,		// hi_fsd_speed
	64000L,	// max_ceiling
	50000L,	// restart_alt
};

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// This section sets the MINIMUM 'Props' values
static Props min = {
	500L,	// airframe_weight
	0.2,		// airframe_drag
	0.2,		// uc_drag
	0.2,		// airbrake_drag
	-20.0,	// dihedral
	-10.0,	// incidence
	10.0,	// stall_angle
	0.3,		// lift
	0.1,		// control_power
	0.1,		// roll_inertia
	0.1,		// pitch_inertia
	0.1,		// stealth
	0,		// retracts
	0,		// flaps
	0,		// airbrakes
	0,		// biplane
	0,		// tail_dragger
	0,		// ejector
	0,		// auto_piloted
	50L,		// fuel_weight
	1,		// engines
	0,		// engine_type
	0,		// afterburner
	0,		// vectored_thrust
	0,		// vector_min
	0,		// vector_max
	0,		// droptnk_qty
	0L,		// droptnk_wt
	0,		// thrust
	0L,		// fuelrt_idle
	0L,		// fuelrt_crz
	0L,		// fuelrt_mrt
	0L,		// fuelrt_ab
	0L,		// thrust_idle
	0L,		// thrust_crz
	0L,		// thrust_mrt
	0L,		// thrust_ab
	0L,		// egt_idle
	0L,		// egt_crz
	0L,		// egt_mrt
	0L,		// egt_ab
	0,		// fuel_dumps
	0,		// dump_rate
	0,		// jammers
	0,		// hi_fsd_speed
	0L,		// max_ceiling
	0L,		// restart_alt
};

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// This section sets the MAXIMUM 'Props' values
static Props max = {
	100000L,	// airframe_weight
	10.0,	// airframe_drag
	10.0,	// uc_drag
	10.0,	// airbrake_drag
	20.0,	// dihedral
	10.0,	// incidence
	45.0,	// stall_angle
	5.0,		// lift
	10.0,	// control_power
	10.0,	// roll_inertia
	10.0,	// pitch_inertia
	10.0,	// stealth
	1,		//  retracts
	1,		// flaps
	1,		// airbrakes
	1,		// biplane
	1,		// tail_dragger
	1,		// ejector
	1,		// auto_piloted
	60000L,	// fuel_weight
	8,		// engines
	2,		// engine_type
	1,		// afterburner
	1,		// vectored_thrust
	180,		// vector_min
	180,		// vector_min
	8,		// droptnk_qty
	32000L,	// droptnk_wt
	0,		// thrust
	50000L,	// fuelrt_idle
	50000L,	// fuelrt_crz
	50000L,	// fuelrt_mrt
	50000L,	// fuelrt_ab
	500000L,	// thrust_idle
	500000L,	// thrust_crz
	500000L,	// thrust_mrt
	500000L,	// thrust_ab
	50000L,	// egt_idle
	50000L,	// egt_crz
	50000L,	// egt_mrt
	50000L,	// egt_ab
	1,		// fuel_dumps
	5000,	// dump_rate
	1,		// jammers
	10000,	// hi_fsd_speed
	90000L,	// max_ceiling
	90000L,	// restart_alt
};

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static Vector2 points[NUM_POINTS] = {
	{  88,   0},			// 00 - nose
	{  31,  12},			// 01 - R foward wing centre
	{ -45,  12},			// 02 - R rear wing centre
	{  31, -12},			// 03 - L foward wing centre
	{ -45, -12},			// 04 - L rear wing centre
	{ -78,   0},			// 05 - foward tail centre
	{-120,   0},			// 06 - rear tail centre
	{ -15,  74},			// 07 - R wing tip (f)
	{ -45,  74},			// 08 - R wing tip (r)
	{ -15, -74},			// 09 - L wing tip (f)
	{ -45, -74},			// 10 - L wing tip (r)
	{-106,  37},			// 11 - R tail tip (f)
	{-123,  37},			// 12 - R tail tip (r)
	{-106, -37},			// 13 - L tail tip (f)
	{-123, -37},			// 14 - L tail tip (r)
	{-130, FIN_BASE - 45}, 	// 15 - fin top lead
	{-150, FIN_BASE - 45}, 	// 16 - fin top back
	{ -87, FIN_BASE},		// 17 - fin bottom lead
	{-140, FIN_BASE},		// 18 - fin bottom back
	{ -78,  12},			// 19 - Aft Body RT
	{ -78, -12},			// 20 - Aft Body LT
};

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static int get_wx(int nx)
{
	//return (x_start + (nx * zoom_factor));
	return (x_start + (nx * 1));
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static int get_wy(int ny)
{
	//return (y_start + (ny * zoom_factor));
	return (y_start + (ny * 1));
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static int get_cx(int nx)
{
	return (x_start + x_centre + (nx * zoom_factor));
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static int get_cy(int ny)
{
	return (y_start + y_centre + (ny * zoom_factor));
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static int get_nx(int wx)
{
	return ((wx - x_start - x_centre) / zoom_factor);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static int get_ny(int wy)
{
	return ((wy - y_start - y_centre) / zoom_factor);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void refresh_window(void)
{
	RECT rect;

	GetClientRect (edit_window, &rect);
	InvalidateRect(edit_window, &rect, FALSE);
	UpdateWindow(edit_window);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void refresh_points(void)
{
	RECT rect;

	GetClientRect (edit_window, &rect);
	InvalidateRect(edit_window, &rect, FALSE);
	UpdateWindow(edit_window);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void mouse_double_clicked(HWND hwnd, int wx, int wy, int click_side)
{
	hwnd = hwnd;

	wx = wx;
	wy = wy;

	if (click_side == 0) {
		double_clicke1 = FALSE;
		double_clicke2 = TRUE;
	}
	else {
		double_clicke1 = TRUE;
		double_clicke2 = FALSE;
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void mouse_pressed(HWND hwnd, int wx, int wy, int click_side)
{
	long t;

	click_side = click_side;
	t = biostime(0, 0L);
	down_time = t;
	dragging = FALSE;
	mouse_down = TRUE;
	SetCapture(hwnd);
	select_point(wx, wy);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void mouse_released(HWND hwnd, int wx, int wy, int click_side)
{
	HDC hdc;

	wx = wx;
	wy = wy;
	click_side = click_side;

	if (double_clicke1) {
		edit_properties();
		double_clicke1 = FALSE;
	}
	else if (double_clicke2) {
		edit_engines();
		double_clicke2 = FALSE;
	}
	else if (mouse_down) {
		ReleaseCapture();
		mouse_down = FALSE;
		if (dragging) {
			hdc = GetDC(hwnd);
			draw_dragged_point(hdc);
			ReleaseDC(hwnd, hdc);
			check_point(selected_point);
		}
		refresh_points();
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void mouse_moved(HWND hwnd, int wx, int wy)
{
	char	s[40];
	HDC	hdc;
	float	cx, cy;

	sprintf(s, "%s","");

	cx = get_cx(0);
	cy = get_cy(0);

	cx = ((wx - cx) / 15);
	cy = ((wy - cy) / 15);

	if (mouse_down) {
		if (dragging || ((biostime(0, 0L) - down_time) > 2)) {
			hdc = GetDC(hwnd);
			if (dragging) {
				draw_dragged_point(hdc);
			}
			else {
				last_selected_pos = points[selected_point];
			}
			move_point(selected_point, wx, wy);
			draw_dragged_point(hdc);
			dragging = TRUE;
			ReleaseDC(hwnd, hdc);
		}
	}
	SetBkMode(hdc, TRANSPARENT);
	SetTextColor(hdc, RGB(0, 0, 0));
	sprintf(s, "Vertex %d:   %5.2f  x %5.2f     ", selected_point, cx, cy );
	TextOut(hdc, get_wx(CX1 + 10), get_wy(SY1 + 3), s, strlen(s));
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void zoom_view(int factor)
{
	RECT rect;

	zoom_factor = factor;
	if (zoom_factor == 1) {
		x_start = 0;
		y_start = 0;
		SetScrollPos(edit_window, SB_HORZ, 0, TRUE);
		SetScrollPos(edit_window, SB_VERT, 0, TRUE);
	}
	GetClientRect (edit_window, &rect);
	InvalidateRect(edit_window, &rect, TRUE);
	UpdateWindow(edit_window);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static BOOL FAR PASCAL db_properties_proc(HWND hdlg, unsigned msg, WORD wParam, LONG lParam)
{
	char s[40];

	lParam = lParam;

	switch(msg) {

		case WM_INITDIALOG:

			sprintf(s, "%s", "");

			sprintf(s, "%ld", params.airframe_weight);
			SetDlgItemText(hdlg, DB3_WEIGHT, s);

			sprintf(s, "%3.2f", params.airframe_drag);
			SetDlgItemText(hdlg, DB3_DRAG, s);

			sprintf(s, "%3.2f", params.uc_drag);
			SetDlgItemText(hdlg, DB3_UCDRAG, s);

			sprintf(s, "%3.2f", params.airbrake_drag);
			SetDlgItemText(hdlg, DB3_ABDRAG, s);

			sprintf(s, "%3.2f", params.dihedral);
			SetDlgItemText(hdlg, DB3_DIHEDRAL, s);

			sprintf(s, "%3.2f", params.incidence);
			SetDlgItemText(hdlg, DB3_INCIDENCE, s);

			sprintf(s, "%3.2f", params.stall_angle);
			SetDlgItemText(hdlg, DB3_STALL, s);

			sprintf(s, "%3.2f", params.lift);
			SetDlgItemText(hdlg, DB3_LIFT, s);

			sprintf(s, "%3.2f", params.control_power);
			SetDlgItemText(hdlg, DB3_CONTROL, s);

			sprintf(s, "%3.2f", params.roll_inertia);
			SetDlgItemText(hdlg, DB3_ROLLI, s);

			sprintf(s, "%3.2f", params.pitch_inertia);
			SetDlgItemText(hdlg, DB3_PITCHI, s);

			sprintf(s, "%3.2f", params.stealth);
			SetDlgItemText(hdlg, DB3_STEALTH, s);

			sprintf(s, "%d", params.hi_fsd_speed);
			SetDlgItemText(hdlg, DB3_HI_FSD_SPEED, s);

			sprintf(s, "%ld", params.max_ceiling);
			SetDlgItemText(hdlg, DB3_MAX_CEILING, s);

			sprintf(s, "%ld", params.restart_alt);
			SetDlgItemText(hdlg, DB3_RESTART_ALT, s);

			CheckDlgButton(hdlg, DB3_RETRACTS, params.retracts);
			CheckDlgButton(hdlg, DB3_FLAPS, params.flaps);
			CheckDlgButton(hdlg, DB3_AIRBRAKES, params.airbrakes);
			CheckDlgButton(hdlg, DB3_BIPLANE, params.biplane);
			CheckDlgButton(hdlg, DB3_TDRAGGER, params.tail_dragger);
			CheckDlgButton(hdlg, DB3_EJECTOR, params.ejector);
			CheckDlgButton(hdlg, DB3_AUTO_PILOTED, params.auto_piloted);
			CheckDlgButton(hdlg, DB3_JAMMERS, params.jammers);

			return(TRUE);

		case WM_COMMAND:

			switch (wParam) {

				case IDOK:
					GetDlgItemText(hdlg, DB3_WEIGHT, s, 40);
					params.airframe_weight = atol(s);

					GetDlgItemText(hdlg, DB3_DRAG, s, 40);
					params.airframe_drag = atof(s);

					GetDlgItemText(hdlg, DB3_UCDRAG, s, 40);
					params.uc_drag = atof(s);

					GetDlgItemText(hdlg, DB3_ABDRAG, s, 40);
					params.airbrake_drag = atof(s);

					GetDlgItemText(hdlg, DB3_DIHEDRAL, s, 40);
					params.dihedral = atof(s);

					GetDlgItemText(hdlg, DB3_INCIDENCE, s, 40);
					params.incidence = atof(s);

					GetDlgItemText(hdlg, DB3_STALL, s, 40);
					params.stall_angle = atof(s);

					GetDlgItemText(hdlg, DB3_LIFT, s, 40);
					params.lift = atof(s);

					GetDlgItemText(hdlg, DB3_CONTROL, s, 40);
					params.control_power = atof(s);

					GetDlgItemText(hdlg, DB3_ROLLI, s, 40);
					params.roll_inertia = atof(s);

					GetDlgItemText(hdlg, DB3_PITCHI, s, 40);
					params.pitch_inertia = atof(s);

					GetDlgItemText(hdlg, DB3_STEALTH, s, 40);
					params.stealth = atof(s);

					GetDlgItemText(hdlg, DB3_HI_FSD_SPEED, s, 40);
					params.hi_fsd_speed = atof(s);

					GetDlgItemText(hdlg, DB3_MAX_CEILING, s, 40);
					params.max_ceiling = atof(s);

					GetDlgItemText(hdlg, DB3_RESTART_ALT, s, 40);
					params.restart_alt	= atof(s);

					params.retracts		= IsDlgButtonChecked(hdlg, DB3_RETRACTS);
					params.flaps			= IsDlgButtonChecked(hdlg, DB3_FLAPS);
					params.airbrakes		= IsDlgButtonChecked(hdlg, DB3_AIRBRAKES);
					params.biplane			= IsDlgButtonChecked(hdlg, DB3_BIPLANE);
					params.tail_dragger	= IsDlgButtonChecked(hdlg, DB3_TDRAGGER);
					params.ejector			= IsDlgButtonChecked(hdlg, DB3_EJECTOR);
					params.auto_piloted	= IsDlgButtonChecked(hdlg, DB3_AUTO_PILOTED);
					params.jammers			= IsDlgButtonChecked(hdlg, DB3_JAMMERS);

					EndDialog(hdlg, TRUE);
					changed = TRUE;
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
static BOOL FAR PASCAL db_engine_proc(HWND hdlg, unsigned msg, WORD wParam, LONG lParam)
{
	char s[40];

	lParam = lParam;
	switch(msg) {

		case WM_INITDIALOG:

			sprintf(s, "%s", "");

			sprintf(s, "%ld", params.fuel_weight);
			SetDlgItemText(hdlg, DB7_FUEL, s);

			sprintf(s, "%d", params.engines);
			SetDlgItemText(hdlg, DB7_ENGINES, s);

			sprintf(s, "%d", params.vector_min);
			SetDlgItemText(hdlg, DB7_VECTOR_MIN, s);

			sprintf(s, "%d", params.vector_max);
			SetDlgItemText(hdlg, DB7_VECTOR_MAX, s);

			sprintf(s, "%d", params.droptnk_qty);
			SetDlgItemText(hdlg, DB7_DROPTNK_QTY, s);

			sprintf(s, "%d", params.droptnk_wt);
			SetDlgItemText(hdlg, DB7_DROPTNK_WT, s);

			sprintf(s, "%ld", params.fuelrt_idle);
			SetDlgItemText(hdlg, DB7_FUELRT_IDLE, s);

			sprintf(s, "%ld", params.fuelrt_crz);
			SetDlgItemText(hdlg, DB7_FUELRT_CRZ, s);

			sprintf(s, "%ld", params.fuelrt_mrt);
			SetDlgItemText(hdlg, DB7_FUELRT_MRT, s);

			sprintf(s, "%ld", params.fuelrt_ab);
			SetDlgItemText(hdlg, DB7_FUELRT_AB, s);

			sprintf(s, "%ld", params.thrust_idle);
			SetDlgItemText(hdlg, DB7_THRUST_IDLE, s);

			sprintf(s, "%ld", params.thrust_crz);
			SetDlgItemText(hdlg, DB7_THRUST_CRZ, s);

			sprintf(s, "%ld", params.thrust_mrt);
			SetDlgItemText(hdlg, DB7_THRUST_MRT, s);

			sprintf(s, "%ld", params.thrust_ab);
			SetDlgItemText(hdlg, DB7_THRUST_AB, s);

			sprintf(s, "%ld", params.egt_idle);
			SetDlgItemText(hdlg, DB7_EGT_IDLE, s);

			sprintf(s, "%ld", params.egt_crz);
			SetDlgItemText(hdlg, DB7_EGT_CRZ, s);

			sprintf(s, "%ld", params.egt_mrt);
			SetDlgItemText(hdlg, DB7_EGT_MRT, s);

			sprintf(s, "%ld", params.egt_ab);
			SetDlgItemText(hdlg, DB7_EGT_AB, s);

			sprintf(s, "%d", params.dump_rate);
			SetDlgItemText(hdlg, DB7_DUMP_RATE, s);

			CheckDlgButton(hdlg, DB7_FUEL_DUMPS, params.fuel_dumps);

			CheckDlgButton(hdlg, DB7_PROP			, (params.engine_type == 0));
			CheckDlgButton(hdlg, DB7_TURBO		, (params.engine_type == 1));
			CheckDlgButton(hdlg, DB7_JET			, (params.engine_type == 2));
			CheckDlgButton(hdlg, DB7_AFTERBURNER	, params.afterburner);
			CheckDlgButton(hdlg, DB7_VECTOR_THRUST	, params.vectored_thrust);

			EnableWindow(GetDlgItem(hdlg, DB7_DUMP_RATE)	, IsDlgButtonChecked(hdlg, DB7_FUEL_DUMPS));
			EnableWindow(GetDlgItem(hdlg, DB7_FUELRT_AB)	, IsDlgButtonChecked(hdlg, DB7_AFTERBURNER));
			EnableWindow(GetDlgItem(hdlg, DB7_THRUST_AB)	, IsDlgButtonChecked(hdlg, DB7_AFTERBURNER));
			EnableWindow(GetDlgItem(hdlg, DB7_EGT_AB)	, IsDlgButtonChecked(hdlg, DB7_AFTERBURNER));
			EnableWindow(GetDlgItem(hdlg, DB7_VECTOR_MAX), IsDlgButtonChecked(hdlg, DB7_VECTOR_THRUST));
			EnableWindow(GetDlgItem(hdlg, DB7_VECTOR_MIN), IsDlgButtonChecked(hdlg, DB7_VECTOR_THRUST));

			return(TRUE);

		case WM_COMMAND:
			switch (wParam) {

				case DB7_FUEL_DUMPS:
					EnableWindow(GetDlgItem(hdlg, DB7_DUMP_RATE), IsDlgButtonChecked(hdlg, DB7_FUEL_DUMPS));
					break;

				case DB7_AFTERBURNER:
					EnableWindow(GetDlgItem(hdlg, DB7_FUELRT_AB)	, IsDlgButtonChecked(hdlg, DB7_AFTERBURNER));
					EnableWindow(GetDlgItem(hdlg, DB7_THRUST_AB)	, IsDlgButtonChecked(hdlg, DB7_AFTERBURNER));
					EnableWindow(GetDlgItem(hdlg, DB7_EGT_AB)		, IsDlgButtonChecked(hdlg, DB7_AFTERBURNER));
					break;

				case DB7_VECTOR_THRUST:
					EnableWindow(GetDlgItem(hdlg, DB7_VECTOR_MAX)	, IsDlgButtonChecked(hdlg, DB7_VECTOR_THRUST));
					EnableWindow(GetDlgItem(hdlg, DB7_VECTOR_MIN)	, IsDlgButtonChecked(hdlg, DB7_VECTOR_THRUST));
					break;

				case DB7_PROP:
					params.engine_type = 0;
					CheckDlgButton(hdlg, DB7_PROP		, 1);
					CheckDlgButton(hdlg, DB7_TURBO	, 0);
					CheckDlgButton(hdlg, DB7_JET		, 0);
                         EnableWindow(GetDlgItem(hdlg, DB7_AFTERBURNER)	, IsDlgButtonChecked(hdlg, DB7_JET));
					break;

				case DB7_TURBO:
					params.engine_type = 1;
					CheckDlgButton(hdlg, DB7_PROP		, 0);
					CheckDlgButton(hdlg, DB7_TURBO	, 1);
					CheckDlgButton(hdlg, DB7_JET		, 0);
					EnableWindow(GetDlgItem(hdlg, DB7_AFTERBURNER)	, IsDlgButtonChecked(hdlg, DB7_JET));
					break;

				case DB7_JET:
					params.engine_type = 2;
					CheckDlgButton(hdlg, DB7_PROP		, 0);
					CheckDlgButton(hdlg, DB7_TURBO	, 0);
					CheckDlgButton(hdlg, DB7_JET		, 1);
					EnableWindow(GetDlgItem(hdlg, DB7_AFTERBURNER)	, IsDlgButtonChecked(hdlg, DB7_JET));
					break;

				case IDOK:
					GetDlgItemText(hdlg, DB7_FUEL, s, 40);
					params.fuel_weight = atol(s);

					GetDlgItemText(hdlg, DB7_ENGINES, s, 40);
					params.engines = atoi(s);

					GetDlgItemText(hdlg, DB7_VECTOR_MIN, s, 40);
					params.vector_min = atoi(s);

					GetDlgItemText(hdlg, DB7_VECTOR_MAX, s, 40);
					params.vector_max = atoi(s);

					GetDlgItemText(hdlg, DB7_DROPTNK_QTY, s, 40);
					params.droptnk_qty = atoi(s);

					GetDlgItemText(hdlg, DB7_DROPTNK_WT, s, 40);
					params.droptnk_wt = atol(s);

				  	GetDlgItemText(hdlg, DB7_FUELRT_IDLE, s, 40);
					params.fuelrt_idle = atol(s);

					GetDlgItemText(hdlg, DB7_FUELRT_CRZ, s, 40);
					params.fuelrt_crz = atol(s);

					GetDlgItemText(hdlg, DB7_FUELRT_MRT, s, 40);
					params.fuelrt_mrt = atol(s);

					GetDlgItemText(hdlg, DB7_FUELRT_AB, s, 40);
					params.fuelrt_ab = atol(s);

				  	GetDlgItemText(hdlg, DB7_THRUST_IDLE, s, 40);
					params.thrust_idle = atol(s);

					GetDlgItemText(hdlg, DB7_THRUST_CRZ, s, 40);
					params.thrust_crz = atol(s);

					GetDlgItemText(hdlg, DB7_THRUST_MRT, s, 40);
					params.thrust_mrt = atol(s);

					GetDlgItemText(hdlg, DB7_THRUST_AB, s, 40);
					params.thrust_ab = atol(s);

				  	GetDlgItemText(hdlg, DB7_EGT_IDLE, s, 40);
					params.egt_idle = atol(s);

					GetDlgItemText(hdlg, DB7_EGT_CRZ, s, 40);
					params.egt_crz = atol(s);

					GetDlgItemText(hdlg, DB7_EGT_MRT, s, 40);
					params.egt_mrt = atol(s);

					GetDlgItemText(hdlg, DB7_EGT_AB, s, 40);
					params.egt_ab = atol(s);

			 		GetDlgItemText(hdlg, DB7_DUMP_RATE, s, 40);
					params.dump_rate = atoi(s);

					if (IsDlgButtonChecked(hdlg, DB7_PROP)) {
						params.engine_type = 0;
                         }
                         else if (IsDlgButtonChecked(hdlg, DB7_TURBO)) {
                         	params.engine_type = 1;
					}
					else if (IsDlgButtonChecked(hdlg, DB7_JET)) {
                         	params.engine_type = 2;
					}

			 		params.afterburner		= IsDlgButtonChecked(hdlg, DB7_AFTERBURNER);
					params.vectored_thrust	= IsDlgButtonChecked(hdlg, DB7_VECTOR_THRUST);

					//EnableWindow(GetDlgItem(hdlg, DB7_VECTOR_MAX), params.vectored_thrust);
					//EnableWindow(GetDlgItem(hdlg, ID_DB6_PROPS), (dialog_object->type != CLASS_CULTURAL));

					params.fuel_dumps			= IsDlgButtonChecked(hdlg, DB7_FUEL_DUMPS);
					changed = TRUE;
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

/*
//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static BOOL FAR PASCAL db_canvas_proc(HWND hdlg, unsigned msg, WORD wParam, LONG lParam)
{
	//int i;
	//static int size;
	//char s[40];

	lParam = lParam;
	switch(msg) {

		case WM_INITDIALOG:

			//size = grid_width / grid_scale;
			//sprintf(s, "%d", size);
			//SetDlgItemText(hdlg, DB6_SIZE, s);

			//sprintf(s, "%d", grid_space / grid_scale);
			//SetDlgItemText(hdlg, DB6_GRID, s);

			//sprintf(s, "%d", (int)((x_centre - CX1) / grid_scale));
			//SetDlgItemText(hdlg, DB6_CENTRE, s);
			return(TRUE);

		case WM_COMMAND:
			switch (wParam) {

				case DB6_SIZE:
					//GetDlgItemText(hdlg, DB6_SIZE, s, 40);
					//i = atoi(s);
					//if (i > grid_width) {
					//	i = grid_width;
					//}
					//else if (i < 10) {
					//	i = 10;
					//}
					//sprintf(s, "%d", i * 2 / 3);
					//SetDlgItemText(hdlg, DB6_CENTRE, s);
					return TRUE;

				case IDOK:

					//GetDlgItemText(hdlg, DB6_SIZE, s, 40);
					//size = atoi(s);
					//if (size > 100) {
					//	size = 100;
					//}
					//if (size < 1) {
					//	size = 1;
					//}
					//grid_scale = grid_width / size;
					//if (grid_scale < 1) {
					//	grid_scale = 1;
					//}

					//GetDlgItemText(hdlg, DB6_GRID, s, 40);
					//grid_space = atoi(s);
					//if (grid_space < 1) {
					//	grid_space = 1;
					//}
					//grid_space = grid_space * grid_scale;

					//GetDlgItemText(hdlg, DB6_CENTRE, s, 40);
					//x_centre = CX1 + atoi(s) * grid_scale;
					changed = TRUE;
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
*/

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void edit_filled(HWND hwnd)
{
	filled_acft = !filled_acft;

	if (filled_acft) {
		CheckMenuItem(GetMenu(hwnd), IDM_FILLED, MF_BYCOMMAND | MF_CHECKED);
	}
	else {
		CheckMenuItem(GetMenu(hwnd), IDM_FILLED, MF_BYCOMMAND | MF_UNCHECKED);
	}
	refresh_window();
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void edit_vertex(HWND hwnd)
{
	show_vertex = !show_vertex;

	if (show_vertex) {
		CheckMenuItem(GetMenu(hwnd), IDM_VERTEX, MF_BYCOMMAND | MF_CHECKED);
	}
	else {
		CheckMenuItem(GetMenu(hwnd), IDM_VERTEX, MF_BYCOMMAND | MF_UNCHECKED);
	}
	refresh_window();
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void edit_grids(HWND hwnd)
{
	show_grids = !show_grids;

	if (show_grids) {
		CheckMenuItem(GetMenu(hwnd), IDM_GRIDS, MF_BYCOMMAND | MF_CHECKED);
	}
	else {
		CheckMenuItem(GetMenu(hwnd), IDM_GRIDS, MF_BYCOMMAND | MF_UNCHECKED);
	}
	refresh_window();
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void change_zoom(HWND hwnd, int zf)
{
	CheckMenuItem(GetMenu(hwnd), IDM_ZOOM1, MF_BYCOMMAND | MF_UNCHECKED);
	CheckMenuItem(GetMenu(hwnd), IDM_ZOOM2, MF_BYCOMMAND | MF_UNCHECKED);
	CheckMenuItem(GetMenu(hwnd), IDM_ZOOM3, MF_BYCOMMAND | MF_UNCHECKED);
	CheckMenuItem(GetMenu(hwnd), IDM_ZOOM4, MF_BYCOMMAND | MF_UNCHECKED);
	CheckMenuItem(GetMenu(hwnd), IDM_ZOOM5, MF_BYCOMMAND | MF_UNCHECKED);

	switch (zf) {

		case 1:
			CheckMenuItem(GetMenu(hwnd), IDM_ZOOM1, MF_BYCOMMAND | MF_CHECKED);
			zoom_factor = 0.5;
			break;

		case 2:
			CheckMenuItem(GetMenu(hwnd), IDM_ZOOM2, MF_BYCOMMAND | MF_CHECKED);
			zoom_factor = 0.75;
			break;

		case 3:
			CheckMenuItem(GetMenu(hwnd), IDM_ZOOM3, MF_BYCOMMAND | MF_CHECKED);
			zoom_factor = 1.0;
			break;

		case 4:
			CheckMenuItem(GetMenu(hwnd), IDM_ZOOM4, MF_BYCOMMAND | MF_CHECKED);
			zoom_factor = 1.5;
			break;

		case 5:
			CheckMenuItem(GetMenu(hwnd), IDM_ZOOM5, MF_BYCOMMAND | MF_CHECKED);
			zoom_factor = 2.0;
			break;
	}
	refresh_window();
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void edit_properties(void)
{
	FARPROC	lpProc;
	Props		saved_params;

	saved_params = params;
	lpProc = MakeProcInstance(db_properties_proc, appInstance);
	if (!DialogBox(appInstance, "DIALOG_3", edit_window, lpProc)) {
		params = saved_params;
	}
	check_params();
	FreeProcInstance(lpProc);
	refresh_window();
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void edit_engines(void)
{
	FARPROC	lpProc;
	Props		saved_params;

	saved_params = params;
	lpProc = MakeProcInstance(db_engine_proc, appInstance);
	if (!DialogBox(appInstance, "DIALOG_7", edit_window, lpProc)) {
		params = saved_params;
	}
	check_params();
	FreeProcInstance(lpProc);
	refresh_window();
}

/*
//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void edit_canvas(void)
{
	FARPROC lpProc;

	lpProc = MakeProcInstance(db_canvas_proc, appInstance);
	DialogBox(appInstance, "DIALOG_6", edit_window, lpProc);
	FreeProcInstance(lpProc);
	refresh_window();
}
*/

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void draw_box(HDC hdc, int x, int y, int width, int height)
{
	MoveTo(hdc, x - width, y - height);
	LineTo(hdc, x + width, y - height);
	LineTo(hdc, x + width, y + height);
	LineTo(hdc, x - width, y + height);
	LineTo(hdc, x - width, y - height);
}

/*
//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void draw_cross(HDC hdc, int x, int y, int width, int height)
{
	MoveTo(hdc, x - width, y);
	LineTo(hdc, x + width + 1, y);
	MoveTo(hdc, x, y - height);
	LineTo(hdc, x, y + height + 1);
}
*/

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void draw_point(int n, int sz)
{
	draw_box(draw_hdc, get_cx(points[n].x), get_cy(points[n].y), sz, sz);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static int get_distance(int x1, int y1, int x2, int y2)
{
	int dx, dy, dist;

	dx = x2 - x1;
	if (dx < 0) {
		dx = -dx;
	}
	dy = y2 - y1;
	if (dy < 0) {
		dy = -dy;
	}
	if (dx >= dy) {
		dist = dx + dy - (dy >> 1);
	}
	else {
		dist = dx + dy - (dx >> 1);
	}
	return dist;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void move_point(int n, int wx, int wy)
{
	points[n].x = get_nx(wx);
	points[n].y = get_ny(wy);
	changed = TRUE;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void select_point(int wx, int wy)
{
	int i, x, y, dist, min_dist;

	last_selected_pos = points[selected_point];
	x = get_nx(wx);
	y = get_ny(wy);
	min_dist = 640;
	for (i = 0; i < NUM_POINTS - 2; i++) {
		dist = get_distance(x, y, points[i].x, points[i].y);
		if (dist < min_dist) {
			min_dist = dist;
			selected_point = i;
		}
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void draw_dragged_point(HDC hdc)
{
	int old_rop;
	HPEN pen, old_pen;

	pen		= CreatePen(PS_SOLID, 0, RGB(0, 128, 0));
	old_pen	= SelectObject(hdc, pen);
	old_rop	= SetROP2(hdc, R2_XORPEN);
	draw_hdc	= hdc;

	draw_point(selected_point, 3);
	SetROP2(hdc, old_rop);
	SelectObject(hdc, old_pen);
	DeleteObject(pen);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void draw_points(HDC hdc)
{
	int i;

	if (show_vertex) {
		draw_hdc = hdc;
		for (i = 0; i < NUM_POINTS - 2; i++) {
			if (i == selected_point) {
				SelectObject(hdc, red_pen);
				draw_point(i, 3);
			}
			else {
				SelectObject(hdc, blue_pen);
				draw_point(i, 3);
			}
		}
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void draw_polygon(HDC hdc, int p1, int p2, int p3, int p4)
{
	POINT pts[4];

	pts[0].x = get_cx(points[p1].x);
	pts[0].y = get_cy(points[p1].y);
	pts[1].x = get_cx(points[p2].x);
	pts[1].y = get_cy(points[p2].y);
	pts[2].x = get_cx(points[p3].x);
	pts[2].y = get_cy(points[p3].y);
	pts[3].x = get_cx(points[p4].x);
	pts[3].y = get_cy(points[p4].y);

	Polygon(hdc, pts, 4);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void draw_aircraft(HDC hdc)
{
	int cx, cy, tail_y;

	// Set Center point
	cx = get_cx(0);
	cy = get_cy(0);

	// Calc tail offsets
  	if (params.engine_type > 1) {
		tail_y = points[2].y;
	}
	else {
		tail_y = points[2].y / 2;
	}
	points[20].x = points[ 5].x;
	points[20].y = tail_y;
	points[21].x = points[ 5].x;
	points[21].y = -tail_y;

	// Set poly draw colors
  	SelectObject(hdc, gray_pen);
	SelectObject(hdc, gray_brush);

	// Draw Aircraft polys
	if (filled_acft) {
  		draw_polygon(hdc,  1,  0,  3,  1);	// NOSE
		draw_polygon(hdc,  1,  2,  4,  3);	// FDW Body
		draw_polygon(hdc,  4,  2, 20, 21);	// AFT Body
		draw_polygon(hdc,  6, 21,  5, 20);	// Tail Body
		draw_polygon(hdc,  1,  7,  8,  2);	// RT Wing
		draw_polygon(hdc,  4, 10,  9,  3);	// LT Wing
		draw_polygon(hdc, 20, 11, 12,  6);	// RT Stab
		draw_polygon(hdc, 21, 13, 14,  6);	// LT Stap
	}

  	// Center Mark
  	SelectObject(hdc, red_pen);
	Arc(hdc, cx - 4, cy - 4, cx + 5, cy + 5, cx, cy - 4, cx, cy -4);
	Arc(hdc, cx - 6, cy - 6, cx + 7, cy + 7, cx, cy - 6, cx, cy -6);

  	// Set pen color
	if (filled_acft) {
  		SelectObject(hdc, dgray_pen);
	}
	else {
  		SelectObject(hdc, black_pen);
	}

	// Nose
	MoveTo(hdc, get_cx(points[ 1].x), get_cy(points[ 1].y));
	LineTo(hdc, get_cx(points[ 0].x), get_cy(points[ 0].y));
	LineTo(hdc, get_cx(points[ 3].x), get_cy(points[ 3].y));

	// Right wing
	MoveTo(hdc, get_cx(points[ 1].x), get_cy(points[ 1].y));
	LineTo(hdc, get_cx(points[ 7].x), get_cy(points[ 7].y));
	LineTo(hdc, get_cx(points[ 8].x), get_cy(points[ 8].y));
	LineTo(hdc, get_cx(points[ 2].x), get_cy(points[ 2].y));
	LineTo(hdc, get_cx(points[ 1].x), get_cy(points[ 1].y));

	// left wing
	MoveTo(hdc, get_cx(points[ 4].x), get_cy(points[ 4].y));
	LineTo(hdc, get_cx(points[10].x), get_cy(points[10].y));
	LineTo(hdc, get_cx(points[ 9].x), get_cy(points[ 9].y));
	LineTo(hdc, get_cx(points[ 3].x), get_cy(points[ 3].y));
	LineTo(hdc, get_cx(points[ 4].x), get_cy(points[ 4].y));

	// body
	MoveTo(hdc, get_cx(points[ 2].x), get_cy(points[ 2].y));
	LineTo(hdc, get_cx(points[ 5].x), get_cy(points[20].y));
	MoveTo(hdc, get_cx(points[ 4].x), get_cy(points[ 4].y));
	LineTo(hdc, get_cx(points[ 5].x), get_cy(points[21].y));

	// right tail
	MoveTo(hdc, get_cx(points[ 5].x), get_cy(points[20].y));
	LineTo(hdc, get_cx(points[11].x), get_cy(points[11].y));
	LineTo(hdc, get_cx(points[12].x), get_cy(points[12].y));
	LineTo(hdc, get_cx(points[ 6].x), get_cy(points[ 6].y));
	LineTo(hdc, get_cx(points[ 5].x), get_cy(points[20].y));

	// left tail
	MoveTo(hdc, get_cx(points[ 5].x), get_cy(points[21].y));
	LineTo(hdc, get_cx(points[13].x), get_cy(points[13].y));
	LineTo(hdc, get_cx(points[14].x), get_cy(points[14].y));
	LineTo(hdc, get_cx(points[ 6].x), get_cy(points[ 6].y));
	LineTo(hdc, get_cx(points[ 5].x), get_cy(points[21].y));

	// Draw Aircraft polys
	if (filled_acft) {
		draw_polygon(hdc, 15, 16, 18, 17);	// Rudder
	}

	// fin
	MoveTo(hdc, get_cx(points[15].x), get_cy(points[15].y));
	LineTo(hdc, get_cx(points[17].x), get_cy(points[17].y));
	LineTo(hdc, get_cx(points[18].x), get_cy(points[18].y));
	LineTo(hdc, get_cx(points[16].x), get_cy(points[16].y));
	LineTo(hdc, get_cx(points[15].x), get_cy(points[15].y));

	draw_points(hdc);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void draw_grid(HDC hdc)
{
	RECT rect;
	int i, x, y, zoom_space;

	zoom_space = grid_space * zoom_factor;

	// Find max/min
	rect.left		= get_wx(CX1);
	rect.right	= get_wx(CX2);
	rect.top		= get_wy(CY1);
	rect.bottom	= get_wy(CY2);
	FillRect(hdc, &rect, GetStockObject(WHITE_BRUSH));

	if (show_grids) {

		for (x = get_cx(0), i = 0; x <= rect.right; x += zoom_space) {
			if (i == 0) {
				SelectObject(hdc, dgray_pen);
			}
			else {
				SelectObject(hdc, lgray_pen);
			}
			MoveTo(hdc, x, rect.top);
			LineTo(hdc, x, rect.bottom);
			if (++i > 9) {
				i = 0;
			}
		}
		for (x = get_cx(0), i = 0; x >= rect.left; x -= zoom_space) {
			if (i == 0) {
				SelectObject(hdc, dgray_pen);
			}
			else {
				SelectObject(hdc, lgray_pen);
			}
			MoveTo(hdc, x, rect.top);
			LineTo(hdc, x, rect.bottom);
			if (++i > 9) {
				i = 0;
			}
		}
		for (y = get_cy(0), i = 0; y <= rect.bottom; y += zoom_space) {
			if (i == 0) {
				SelectObject(hdc, dgray_pen);
			}
			else {
				SelectObject(hdc, lgray_pen);
			}
			MoveTo(hdc, rect.left, y);
			LineTo(hdc, rect.right, y);
			if (++i > 9) {
				i = 0;
			}
		}
		for (y = get_cy(0), i = 0; y >= rect.top; y -= zoom_space) {
			if (i == 0) {
				SelectObject(hdc, dgray_pen);
			}
			else {
				SelectObject(hdc, lgray_pen);
			}
			MoveTo(hdc, rect.left, y);
			LineTo(hdc, rect.right, y);
			if (++i > 9) {
				i = 0;
			}
		}
		// Draw centerlines
	  	SelectObject(hdc, black_pen);
		MoveTo(hdc, rect.left, get_cy(0));
		LineTo(hdc, rect.right, get_cy(0));

	  	MoveTo(hdc, get_cx(0), rect.top);
		LineTo(hdc, get_cx(0), rect.bottom);
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
extern void draw_scale(HDC hdc)
{
	RECT rect;
	char s[40];

	// Find max/min
	rect.left	= get_wx(CX1);
	rect.right	= get_wx(CX2);
	rect.top		= get_wy(SY1);
	rect.bottom = get_wy(SY2);

	FillRect(hdc, &rect, GetStockObject(WHITE_BRUSH));

	SelectObject(hdc, white_pen);
	MoveTo(hdc, get_wx(CX1), get_wy(SY2));
	LineTo(hdc, get_wx(CX2), get_wy(SY2));
	LineTo(hdc, get_wx(CX2), get_wy(SY1));

 	SelectObject(hdc, black_pen);
	MoveTo(hdc, get_wx(CX1-1), get_wy(SY2+1));
	LineTo(hdc, get_wx(CX1-1), get_wy(SY1-1));
	LineTo(hdc, get_wx(CX2+0), get_wy(SY1-1));

 	SelectObject(hdc, dgray_pen);
	MoveTo(hdc, get_wx(CX2+2), get_wy(SY1-2));
	LineTo(hdc, get_wx(CX1-2), get_wy(SY1-2));
	LineTo(hdc, get_wx(CX1-2), get_wy(SY2+2));

	SelectObject(hdc, lgray_pen);
	MoveTo(hdc, get_wx(CX2+1), get_wy(SY1-1));
	LineTo(hdc, get_wx(CX2+1), get_wy(SY2+1));
	LineTo(hdc, get_wx(CX1-1), get_wy(SY2+1));

	SelectObject(hdc, white_pen);
	LineTo(hdc, get_wx(CX1-2), get_wy(SY2+2));
	LineTo(hdc, get_wx(CX2+2), get_wy(SY2+2));
	LineTo(hdc, get_wx(CX2+2), get_wy(SY1-2));

	SetTextColor(hdc, RGB(0, 0, 0));
	sprintf(s, "Vertex %d:   %5.2f  x %5.2f     ", selected_point, (float)points[selected_point].x/grid_scale, (float)points[selected_point].y/grid_scale);
	TextOut(hdc, get_wx(CX1 + 8), get_wy(SY1 + 3), s, strlen(s));

	SetTextColor(hdc, RGB(128, 128, 128));
	sprintf(s, "%d%", (int)(100.0 * zoom_factor));
	TextOut(hdc, get_wx(CX2 - 40), get_wy(SY1 + 3), s, strlen(s));


	// Draw 3D Frame
	SelectObject(hdc, white_pen);
	MoveTo(hdc, get_wx(CX1), get_wy(CY2));
	LineTo(hdc, get_wx(CX2), get_wy(CY2));
	LineTo(hdc, get_wx(CX2), get_wy(CY1));

 	SelectObject(hdc, black_pen);
	MoveTo(hdc, get_wx(CX1-1), get_wy(CY2+1));
	LineTo(hdc, get_wx(CX1-1), get_wy(CY1-1));
	LineTo(hdc, get_wx(CX2+0), get_wy(CY1-1));

 	SelectObject(hdc, dgray_pen);
	MoveTo(hdc, get_wx(CX2+2), get_wy(CY1-2));
	LineTo(hdc, get_wx(CX1-2), get_wy(CY1-2));
	LineTo(hdc, get_wx(CX1-2), get_wy(CY2+2));

	SelectObject(hdc, lgray_pen);
	MoveTo(hdc, get_wx(CX2+1), get_wy(CY1-1));
	LineTo(hdc, get_wx(CX2+1), get_wy(CY2+1));
	LineTo(hdc, get_wx(CX1-1), get_wy(CY2+1));

	SelectObject(hdc, white_pen);
	LineTo(hdc, get_wx(CX1-2), get_wy(CY2+2));
	LineTo(hdc, get_wx(CX2+2), get_wy(CY2+2));
	LineTo(hdc, get_wx(CX2+2), get_wy(CY1-2));

	SelectObject(hdc, gray_pen);
	MoveTo(hdc, 0, 0);
	LineTo(hdc, 1280, 0);
	LineTo(hdc, 1280, 1);
	LineTo(hdc, 0, 1);

	MoveTo(hdc, 0, 0);
	LineTo(hdc, 0, 1024);
	LineTo(hdc, 1, 1024);
	LineTo(hdc, 1, 0);

  	MoveTo(hdc, CX2 + 3, 0);
	LineTo(hdc, CX2 + 3, 1024);
	LineTo(hdc, CX2 + 4, 1024);
	LineTo(hdc, CX2 + 4, 0);

  	MoveTo(hdc, 0, SY2 + 3);
	LineTo(hdc, 1280, SY2 + 3);

}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void draw_info(HDC hdc)
{
	char s[40];

	SetBkMode(hdc, TRANSPARENT);

	SetTextColor(hdc, RGB(0, 0, 128));
	sprintf(s, "Max speed = %d", calc_top_speed());
	TextOut(hdc, 10, 10, s, strlen(s));

	sprintf(s, "Stall speed = %d", calc_stall_speed());
	TextOut(hdc, 10, 30, s, strlen(s));
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void redraw_client(HDC hdc, RECT *rc)
{
	FillRect(hdc, rc, gray_brush);

	draw_grid(hdc);
	draw_aircraft(hdc);
	draw_info(hdc);
	draw_scale(hdc);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void resize_client(HWND hwnd, int width, int height)
{
	hwnd	 = hwnd;
	height = height;
	width  = width;

	CX2 = width  - 6;
	CY2 = height - 30;
	SY1 = height - 30;
	SY2 = height - 6;

	grid_width	= CX2 - 6;
	grid_height	= CY2 - 6;

	x_centre = (grid_width  * 0.55);
	y_centre = (grid_height * 0.40);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void scroll_client(HWND hwnd, int refresh, int xpos, int ypos)
{
	RECT rect;

	x_start = -xpos;
	y_start = -ypos;

	if (refresh) {
		GetClientRect (hwnd, &rect);
		InvalidateRect(hwnd, &rect, TRUE);
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void create_client(HWND hwnd)
{
	model_x_size	= 360;
	model_y_size	= 360;

	edit_window	= hwnd;

	red_pen		= CreatePen(PS_SOLID, 0, RGB(255,   0,   0));
	blue_pen		= CreatePen(PS_SOLID, 0, RGB(  0,   0, 192));
	white_pen		= GetStockObject(WHITE_PEN);
	black_pen		= GetStockObject(BLACK_PEN);

	lgray_pen		= CreatePen(PS_SOLID, 0, RGB(223, 223, 223));
	gray_pen 		= CreatePen(PS_SOLID, 0, RGB(192, 192, 192));
	dgray_pen		= CreatePen(PS_SOLID, 0, RGB(128, 128, 128));

	lgray_brush	= CreateSolidBrush(RGB(223, 223, 223));
	gray_brush	= CreateSolidBrush(RGB(192, 192, 192));
	dgray_brush	= CreateSolidBrush(RGB(128, 128, 128));

	blue_brush	= CreateSolidBrush(RGB(  0,   0, 192));
	black_brush	= GetStockObject(BLACK_BRUSH);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void check_params(void)
{
	Bounds(params.airframe_weight	, min.airframe_weight	, max.airframe_weight);
	Bounds(params.airframe_drag	, min.airframe_drag		, max.airframe_drag);
	Bounds(params.uc_drag			, min.uc_drag				, max.uc_drag);
	Bounds(params.airbrake_drag	, min.airbrake_drag		, max.airbrake_drag);
	Bounds(params.dihedral			, min.dihedral				, max.dihedral);
	Bounds(params.incidence			, min.incidence			, max.incidence);
	Bounds(params.stall_angle		, min.stall_angle			, max.stall_angle);
	Bounds(params.lift				, min.lift					, max.lift);
	Bounds(params.control_power	, min.control_power		, max.control_power);
	Bounds(params.roll_inertia		, min.roll_inertia		, max.roll_inertia);
	Bounds(params.pitch_inertia	, min.pitch_inertia		, max.pitch_inertia);
	Bounds(params.retracts			, min.retracts				, max.retracts);
	Bounds(params.flaps				, min.flaps					, max.flaps);
	Bounds(params.airbrakes			, min.airbrakes			, max.airbrakes);
	Bounds(params.biplane			, min.biplane				, max.biplane);
	Bounds(params.tail_dragger		, min.tail_dragger		, max.tail_dragger);
	Bounds(params.ejector			, min.ejector				, max.ejector);
	Bounds(params.fuel_weight		, min.fuel_weight			, max.fuel_weight);
	Bounds(params.engines			, min.engines				, max.engines);
	Bounds(params.engine_type		, min.engine_type			, max.engine_type);
	Bounds(params.jammers			, min.jammers				, max.jammers);
	Bounds(params.stealth			, min.stealth				, max.stealth);
	Bounds(params.hi_fsd_speed		, min.hi_fsd_speed		, max.hi_fsd_speed);
	Bounds(params.max_ceiling		, min.max_ceiling			, max.max_ceiling);
	Bounds(params.restart_alt		, min.restart_alt			, max.restart_alt);
	Bounds(params.auto_piloted		, min.auto_piloted 	 	, max.auto_piloted);
	Bounds(params.afterburner		, min.afterburner 	 	, max.afterburner);
	Bounds(params.vectored_thrust	, min.vectored_thrust	, max.vectored_thrust);
	Bounds(params.vector_min		, min.vector_min			, max.vector_min);
	Bounds(params.vector_max		, min.vector_max			, max.vector_max);
	Bounds(params.droptnk_qty		, min.droptnk_qty			, max.droptnk_qty);
	Bounds(params.droptnk_wt		, min.droptnk_wt			, max.droptnk_wt);
	Bounds(params.fuelrt_idle		, min.fuelrt_idle			, max.fuelrt_idle);
	Bounds(params.fuelrt_crz		, min.fuelrt_crz			, max.fuelrt_crz);
	Bounds(params.fuelrt_mrt		, min.fuelrt_mrt			, max.fuelrt_mrt);
	Bounds(params.fuelrt_ab			, min.fuelrt_ab			, max.fuelrt_ab);
	Bounds(params.thrust_idle		, min.thrust_idle			, max.thrust_idle);
	Bounds(params.thrust_crz		, min.thrust_crz			, max.thrust_crz);
	Bounds(params.thrust_mrt		, min.thrust_mrt			, max.thrust_mrt);
	Bounds(params.thrust_ab			, min.thrust_ab			, max.thrust_ab);
	Bounds(params.egt_idle			, min.egt_idle				, max.egt_idle);
	Bounds(params.egt_crz			, min.egt_crz				, max.egt_crz);
	Bounds(params.egt_mrt			, min.egt_mrt				, max.egt_mrt);
	Bounds(params.egt_ab				, min.egt_ab				, max.egt_ab);
	Bounds(params.fuel_dumps		, min.fuel_dumps			, max.fuel_dumps);
	Bounds(params.dump_rate			, min.dump_rate			, max.dump_rate);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void check_point(int n)
{
	switch (n) {

		case 0:
			points[0].y = 0;
			if ((x_centre + points[0].x) > grid_width) {

				points[0].x = grid_width - x_centre;

			}
			if (points[0].x <= points[1].x) {
				points[0].x = points[1].x + 1;
			}
			break;

		case 1:
			Bounds(points[1].y, 1, 40);
			if (points[1].x <= 0) {
				points[1].x = 1;
			}
			if (points[1].x >= points[0].x) {
				points[1].x = points[0].x - 1;
			}
			points[3].x = points[1].x;
			points[3].y = -points[1].y;
			break;

		case 2:
			Bounds(points[2].y, 1, 40);
			if (points[2].x >= 0) {
				points[2].x = -1;
			}
			if (points[2].x <= points[5].x) {
				points[2].x = points[5].x + 1;
			}
			points[4].x = points[2].x;
			points[4].y = -points[2].y;
			break;

		case 3:
			Bounds(points[3].y, -40, -1);
			if (points[3].x <= 0) {
				points[3].x = 1;
			}
			if (points[3].x >= points[0].x) {
				points[3].x = points[0].x - 1;
			}
			points[1].x = points[3].x;
			points[1].y = -points[3].y;
			break;

		case 4:
			Bounds(points[4].y, -40, -1);
			if (points[4].x >= 0) {
				points[4].x = -1;
			}
			if (points[4].x <= points[5].x) {
				points[4].x = points[5].x + 1;
			}
			points[2].x = points[4].x;
			points[2].y = -points[4].y;
			break;

		case 5:
			points[5].y = 0;
			if (points[5].x >= points[2].x) {
				points[5].x = points[2].x - 1;
			}
			if (points[5].x <= points[6].x) {
				points[5].x = points[6].x + 1;
			}
			break;

		case 6:
			points[6].y = 0;
			if (points[6].x >= points[5].x) {
				points[6].x = points[5].x - 1;
			}
			if (points[6].x <= -x_centre) {
				points[6].x = -x_centre;
			}
			break;

		case 7:
			if (points[7].y <= points[1].y) {
				points[7].y = points[1].y + 1;
			}
			if (points[7].x >= points[0].x) {
				points[7].x = points[0].x - 1;
			}
			if (points[7].x <= points[8].x) {
				points[7].x = points[8].x + 1;
			}
			points[9].y = -points[7].y;
			points[9].x = points[7].x;
			break;

		case 8:
			if (points[8].y <= points[2].y) {
				points[8].y = points[2].y + 1;
			}
			if (points[8].x <= points[5].x) {
				points[8].x = points[5].x + 1;
			}
			if (points[8].x >= points[7].x) {
				points[8].x = points[7].x - 1;
			}
			points[10].y = -points[8].y;
			points[10].x = points[8].x;
			break;

		case 9:
			if (points[9].y >= points[3].y) {
				points[9].y = points[3].y - 1;
			}
			if (points[9].x >= points[0].x) {
				points[9].x = points[0].x - 1;
			}
			if (points[9].x <= points[10].x) {
				points[9].x = points[10].x + 1;
			}
			points[7].y = -points[9].y;
			points[7].x = points[9].x;
			break;

		case 10:
			if (points[10].y >= points[4].y) {
				points[10].y = points[4].y - 1;
			}
			if (points[10].x <= points[5].x) {
				points[10].x = points[5].x + 1;
			}
			if (points[10].x >= points[9].x) {
				points[10].x = points[9].x - 1;
			}
			points[8].y = -points[10].y;
			points[8].x = points[10].x;
			break;

		case 11:
			if (points[11].y <= points[2].y) {
				points[11].y = points[2].y + 1;
			}
			if (points[11].x >= points[2].x) {
				points[11].x = points[2].x - 1;
			}
			if (points[11].x <= points[12].x) {
				points[11].x = points[12].x + 1;
			}
			points[13].y = -points[11].y;
			points[13].x = points[11].x;
			break;

		case 12:
			if (points[12].y <= points[2].y) {
				points[12].y = points[2].y + 1;
			}
			if (points[12].x <= -x_centre) {
				points[12].x = -x_centre;
			}
			if (points[12].x >= points[11].x) {
				points[12].x = points[11].x - 1;
			}
			points[14].y = -points[12].y;
			points[14].x = points[12].x;
			break;

		case 13:
			if (points[13].y >= points[4].y) {
				points[13].y = points[4].y - 1;
			}
			if (points[13].x >= points[4].x) {
				points[13].x = points[4].x - 1;
			}
			if (points[13].x <= points[14].x) {
				points[13].x = points[14].x + 1;
			}
			points[11].y = -points[13].y;
			points[11].x = points[13].x;
			break;

		case 14:
			if (points[14].y >= points[4].y) {
				points[14].y = points[4].y - 1;
			}
			if (points[14].x <= -x_centre) {
				points[14].x = -x_centre;
			}
			if (points[14].x >= points[13].x) {
				points[14].x = points[13].x - 1;
			}
			points[12].y = -points[14].y;
			points[12].x = points[14].x;
			break;

		case 15:
			Bounds(points[15].x, points[16].x + 4, 0);
			Bounds(points[15].y, 100, FIN_BASE - 2);
			break;

		case 16:
			Bounds(points[16].x, -x_centre, points[15].x - 4);
			Bounds(points[16].y, 100, FIN_BASE - 2);
			break;

		case 17:
			Bounds(points[17].x, points[18].x + 6, 0);
			points[17].y = FIN_BASE;
			break;

		case 18:
			Bounds(points[18].x, -x_centre, points[17].x - 6);
			points[18].y = FIN_BASE;
			break;

		default:
			break;
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static int triangle_area(Vector2 *pts, int p1, int p2, int p3)
{
	int area;
	double dx, dy, s, a, b, c;

	dx = (double)pts[p2].x - (double)pts[p1].x;
	dy = (double)pts[p2].y - (double)pts[p1].y;
	a = sqrt((dx * dx) + (dy * dy));
	dx = (double)pts[p3].x - (double)pts[p2].x;
	dy = (double)pts[p3].y - (double)pts[p2].y;
	b = sqrt((dx * dx) + (dy * dy));
	dx = (double)pts[p1].x - (double)pts[p3].x;
	dy = (double)pts[p1].y - (double)pts[p3].y;
	c = sqrt((dx * dx) + (dy * dy));
	s = (a + b + c) / 2.0;
	area = (int)(sqrt(s * (s - a) * (s - b) * (s - c)));
	return area;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static int wing_area(Vector2 *pts)
{
	int tri1, tri2;

	tri1 = triangle_area(pts, 1, 7, 2);
	tri2 = triangle_area(pts, 8, 2, 7);

	return(tri1 + tri2);
}

/*
//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static int tail_area(Vector2 *pts)
{
	int tri1, tri2;

	tri1 = triangle_area(pts, 5, 11, 6);
	tri2 = triangle_area(pts, 12, 6, 11);
	return(tri1 + tri2);
}
*/
/*
//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static int fin_area(Vector2 *pts)
{
	int tri1, tri2;

	tri1 = triangle_area(pts, 18, 15, 17);
	tri2 = triangle_area(pts, 15, 18, 16);
	return(tri1 + tri2);
}
*/

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static float fin_height(Vector2 *pts)
{
	if (pts[15].y < pts[16].y) {
		return (pts[18].y - pts[15].y);
	}
	return (pts[18].y - pts[16].y);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static float tail_span(Vector2 *pts)
{
	if (pts[11].y > pts[12].y) {
		return pts[11].y;
	}
	return pts[12].y;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static float wing_span(Vector2 *pts)
{
	if (pts[7].y > pts[8].y) {
		return pts[7].y;
	}
	return pts[8].y;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static float root_chord(Vector2 *pts)
{
	return(pts[1].x - pts[2].x);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static float calc_lift_coeff(Vector2 *pts)
{
	int wa, lc;

	wa = wing_area(pts) / 20;
	lc = (wa * 4) + (wa * wing_span(pts)) / root_chord(pts);

	return (lc / 4);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void scale_points(void)
{
	int i;

	for (i = 0; i < NUM_POINTS; i++) {
		scaled_points[i].x = (points[i].x * 10.0) / 15.0;
		scaled_points[i].y = (points[i].y * 10.0) / 15.0;
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
int calc_stall_speed(void)
{
	int speed;
	double lift_coeff, weight, lift;

	scale_points();
	lift_coeff = (double)(calc_lift_coeff(scaled_points) * params.lift);
	NORMALISE_FORCE(lift_coeff, params.airframe_weight);
	lift = lift_coeff * (double)(params.stall_angle - 1.0);
	weight = STD_WEIGHT * 1.2;
	speed = (int)(sqrt((weight * 1200.0) / lift));
	return (speed);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static int calc_top_speed(void)
{
	int		speed;
	double	thrust, drag;

	scale_points();
	thrust = params.thrust_mrt * params.engines;
	NORMALISE_FORCE(thrust, params.airframe_weight);
	drag = (double)(( wing_span(scaled_points) + tail_span(scaled_points) + fin_height(scaled_points)) * params.airframe_drag);
	NORMALISE_FORCE(drag, params.airframe_weight);
	speed = (int)(sqrt((thrust * 4096.0) / drag));
	return (speed);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
int load_model(char *file_name)
{
	FILE *fp;
	int i, version, dummy;
	Vector2 v;
	char s[64];

	fp = fopen(proj_filename(file_name), "r");
	if (fp != NULL) {
		fscanf(fp, "%s %d\n", s, &version);
		fscanf(fp, "%s %ld\n", s, &params.airframe_weight);
		fscanf(fp, "%s %f\n", s, &params.airframe_drag);
		fscanf(fp, "%s %f\n", s, &params.uc_drag);
		fscanf(fp, "%s %f\n", s, &params.airbrake_drag);
		fscanf(fp, "%s %f\n", s, &params.dihedral);
		fscanf(fp, "%s %f\n", s, &params.incidence);
		fscanf(fp, "%s %f\n", s, &params.stall_angle);
		fscanf(fp, "%s %f\n", s, &params.lift);
		fscanf(fp, "%s %f\n", s, &params.control_power);
		fscanf(fp, "%s %f\n", s, &params.roll_inertia);
		fscanf(fp, "%s %f\n", s, &params.pitch_inertia);
		fscanf(fp, "%s %d\n", s, &params.retracts);
		fscanf(fp, "%s %d\n", s, &params.flaps);
		fscanf(fp, "%s %d\n", s, &params.airbrakes);
		fscanf(fp, "%s %d\n", s, &params.biplane);
		fscanf(fp, "%s %d\n", s, &params.tail_dragger);
		fscanf(fp, "%s %d\n", s, &params.ejector);

		if (version < 2) {
			fscanf(fp, "%s %ld\n", s, &params.thrust);
			params.thrust_idle	= params.thrust * .25;
			params.thrust_crz		= params.thrust * .7;
			params.thrust_mrt		= params.thrust;
			params.thrust_ab		= params.thrust * 1.25;
		}

		fscanf(fp, "%s %ld\n", s, &params.fuel_weight);
		fscanf(fp, "%s %d\n", s, &params.engines);
		fscanf(fp, "%s %d\n", s, &params.engine_type);

		if (version < 2) {
			fscanf(fp, "%s %d\n", s, &dummy);	//grid_scale);
			fscanf(fp, "%s %d\n", s, &dummy);	//grid_space);
			fscanf(fp, "%s %d\n", s, &dummy);	//x_centre);
      }

		// version 2 fmd
		if(version > 1){
			fscanf(fp, "%s %d\n", s, &params.jammers);
			fscanf(fp, "%s %f\n", s, &params.stealth);
			fscanf(fp, "%s %d\n", s, &params.hi_fsd_speed);
			fscanf(fp, "%s %ld\n", s, &params.max_ceiling);
			fscanf(fp, "%s %ld\n", s, &params.restart_alt);
			fscanf(fp, "%s %d\n", s, &params.auto_piloted);
			fscanf(fp, "%s %d\n", s, &params.afterburner);
			fscanf(fp, "%s %d\n", s, &params.vectored_thrust);
			fscanf(fp, "%s %d\n", s, &params.vector_min);
			fscanf(fp, "%s %d\n", s, &params.vector_max);
			fscanf(fp, "%s %d\n", s, &params.droptnk_qty);
			fscanf(fp, "%s %ld\n", s, &params.droptnk_wt);

			fscanf(fp, "%s %ld\n", s, &params.fuelrt_idle);
			fscanf(fp, "%s %ld\n", s, &params.fuelrt_crz);
			fscanf(fp, "%s %ld\n", s, &params.fuelrt_mrt);
			fscanf(fp, "%s %ld\n", s, &params.fuelrt_ab);

			fscanf(fp, "%s %ld\n", s, &params.thrust_idle);
			fscanf(fp, "%s %ld\n", s, &params.thrust_crz);
			fscanf(fp, "%s %ld\n", s, &params.thrust_mrt);
			fscanf(fp, "%s %ld\n", s, &params.thrust_ab);

			fscanf(fp, "%s %ld\n", s, &params.egt_idle);
			fscanf(fp, "%s %ld\n", s, &params.egt_crz);
			fscanf(fp, "%s %ld\n", s, &params.egt_mrt);
			fscanf(fp, "%s %ld\n", s, &params.egt_ab);

			fscanf(fp, "%s %d\n", s, &params.fuel_dumps);
			fscanf(fp, "%s %d\n", s, &params.dump_rate);
		}

		for (i = 0; i < NUM_POINTS; i++) {
			fscanf(fp, "%d %d\n", &v.x, &v.y);
			points[i] = v;
		}

		// Convert JET flag to 'engine_type'
		if (version < 2) {
			if (params.engine_type) {
				params.engine_type = 2;
			}
		}

		grid_space	= 5;
		grid_scale	= 5;
		x_centre		= grid_width  * 0.55;
		y_centre		= grid_height * 0.40;

		fclose(fp);
		return TRUE;
	}
	return FALSE;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
int save_model(char *file_name)
{
	FILE *fp;
	int i;

	fp = fopen(proj_filename(file_name), "w");
	if (fp != NULL) {
		fprintf(fp, "FST_Model %d\n", 2);

		// Version 1 FMD
		fprintf(fp, "airframe_weight %ld\n", params.airframe_weight);
		fprintf(fp, "airframe_drag   %3.2f\n", params.airframe_drag);
		fprintf(fp, "uc_drag         %3.2f\n", params.uc_drag);
		fprintf(fp, "airbrake_drag   %3.2f\n", params.airbrake_drag);
		fprintf(fp, "dihedral        %3.2f\n", params.dihedral);
		fprintf(fp, "incidence       %3.2f\n", params.incidence);
		fprintf(fp, "stall_angle     %3.2f\n", params.stall_angle);
		fprintf(fp, "wing_efficiency %3.2f\n", params.lift);
		fprintf(fp, "control_power   %3.2f\n", params.control_power);
		fprintf(fp, "roll_inertia    %3.2f\n", params.roll_inertia);
		fprintf(fp, "pitch_inertia   %3.2f\n", params.pitch_inertia);
		fprintf(fp, "retracts        %d\n", params.retracts);
		fprintf(fp, "flaps_slats     %d\n", params.flaps);
		fprintf(fp, "airbrakes       %d\n", params.airbrakes);
		fprintf(fp, "biplane         %d\n", params.biplane);
		fprintf(fp, "tail_dragger    %d\n", params.tail_dragger);
		fprintf(fp, "ejector         %d\n", params.ejector);

		fprintf(fp, "fuel_weight     %ld\n", params.fuel_weight);
		fprintf(fp, "num_engines     %d\n", params.engines);
		fprintf(fp, "engine_type     %d\n", params.engine_type);

		//fprintf(fp, "units_per_meter %d\n", grid_scale);
		//fprintf(fp, "grid_space      %d\n", grid_space);
		//fprintf(fp, "x_centre        %d\n", x_centre);

		// Version 2 FMD
		fprintf(fp, "jammers         %d\n", params.jammers);
		fprintf(fp, "stealth         %3.2f\n", params.stealth);
		fprintf(fp, "hi_fsd_speed    %d\n", params.hi_fsd_speed);
		fprintf(fp, "max_ceiling     %ld\n", params.max_ceiling);
		fprintf(fp, "restart_alt     %ld\n", params.restart_alt);

		fprintf(fp, "auto_piloted    %d\n", params.auto_piloted);
		fprintf(fp, "afterburner     %d\n", params.afterburner);
		fprintf(fp, "vectored_thrust %d\n", params.vectored_thrust);
		fprintf(fp, "vector_min      %d\n", params.vector_min);
		fprintf(fp, "vector_max      %d\n", params.vector_max);
		fprintf(fp, "droptnk_qty     %d\n", params.droptnk_qty);
		fprintf(fp, "droptnk_wt      %ld\n", params.droptnk_wt);

		fprintf(fp, "fuelrt_idle     %ld\n", params.fuelrt_idle);
		fprintf(fp, "fuelrt_crz      %ld\n", params.fuelrt_crz);
		fprintf(fp, "fuelrt_mrt      %ld\n", params.fuelrt_mrt);
		fprintf(fp, "fuelrt_ab       %ld\n", params.fuelrt_ab);

		fprintf(fp, "thrust_idle     %ld\n", params.thrust_idle);
		fprintf(fp, "thrust_crz      %ld\n", params.thrust_crz);
		fprintf(fp, "thrust_mrt      %ld\n", params.thrust_mrt);
		fprintf(fp, "thrust_ab       %ld\n", params.thrust_ab);

		fprintf(fp, "egt_idle        %ld\n", params.egt_idle);
		fprintf(fp, "egt_crz         %ld\n", params.egt_crz);
		fprintf(fp, "egt_mrt         %ld\n", params.egt_mrt);
		fprintf(fp, "egt_ab          %ld\n", params.egt_ab);

		fprintf(fp, "fuel_dumps      %d\n", params.fuel_dumps);
		fprintf(fp, "dump_rate       %d\n", params.dump_rate);

		for (i = 0; i < NUM_POINTS; i++) {
			fprintf(fp, "%d %d\n", points[i].x, points[i].y);
		}
		fclose(fp);
		return TRUE;
	}
	return FALSE;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void new_model(void)
{
 	x_start		= 0;
	y_start		= 0;
	zoom_factor	= 1;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void setup_model(void)
{
	int i;

	for (i = 0; i < MAX_ENGINE; i++) {
		engines[i].type = NULL;
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void tidyup_model(void)
{
	DeleteObject(red_pen);
	DeleteObject(gray_pen);
	DeleteObject(dgray_pen);
	DeleteObject(lgray_pen);
	DeleteObject(blue_brush);
	DeleteObject(dgray_brush);
	DeleteObject(gray_brush);
	DeleteObject(lgray_brush);
}
