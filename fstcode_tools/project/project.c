/*------------------------------------------------
   GSF Project Mananger
 ------------------------------------------------*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <dir.h>
#include <math.h>
#include <lzexpand.h>
#include <commdlg.h>

#include "project.h"

#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))
// #define abs(a)   (((a) > 0) ? (a) : (-a))

char prog_version[] = "Version 2.2c Beta";

#define  FORM_MIN_WIDTH  424
#define  FORM_MIN_HEIGHT 324
#define  FORM_MAX_WIDTH  424
#define  FORM_MAX_HEIGHT 324

MINMAXINFO FAR *lpMinMaxInfo;

/* =====================================================================
 * Specification of all files which comprise a minimum FST project
 * fst.fpj - project file
 * fly.exe - the simulator
 * cols.fcd - default colours
 * box.fsd - default shape
 * ==================================================================== */
#define MAX_TEMPLATE_N 128
static int   template_n = 0;
static char *template_files[MAX_TEMPLATE_N];
//static char *template_files[MAX_TEMPLATE_N] = {"fst.fpj", "cols.fcd", "fly.exe", "box.fsd"};

extern HWND create_tile_window(HWND parent, HANDLE hInstance, int x, int y, int width, int height);

extern void init_tile(void);
extern void setup_tile(void);
extern void tidyup_tile(void);
extern void resize(void);
extern long int x_sel, z_sel;

static HBITMAP hbm_bitmap, hbm_old_bitmap;
static HDC hdc_bitmap;

static int button_window_width  = 0;
static int button_window_height = 0;

static BOOL FAR PASCAL db_about_proc(HWND hwnd,  unsigned msg, WORD wParam, LONG lParam);
static BOOL FAR PASCAL db_size_proc(HWND hwnd,  unsigned msg, WORD wParam, LONG lParam);
static BOOL FAR PASCAL db_origin_proc(HWND hwnd,  unsigned msg, WORD wParam, LONG lParam);
static BOOL FAR PASCAL db_colours_proc(HWND hdlg, unsigned msg, WORD wParam, LONG lParam);

long FAR PASCAL _export WndProc (HWND, UINT, UINT, LONG) ;

HANDLE hInst;      // Program Instance
HWND hDlgModeless;
HWND main_window;

static int  tools_drive = 0;						 // drive containing tool directory
static char tools_path[MAXPATH] = "";    // path to FST tool directory
static char romtools_path[MAXPATH] = ""; // path of tools on read only media
static char path_name[MAXPATH]  = "";    // path to current project
static char new_project[MAXPATH] = "";   // name of new project
static char project_title[32] = "Noname";

extern char colours_file[32] = "cols.fcd";

int width  = 6;
int height = 6;
int grid_type = 1;
int show_map = TRUE;

int latitude	= 0;
int longitude	= 0;

static char *button_names[] = {
	"W O R L D",
	"S H A P E",
	"C O L O R",
	"T E X T U R E",
	"C O C K P I T",
	"M O D E L",
	"V I E W",
	"F O N T",
	"P U R G E",
	"Frontend",
	"F L Y"
};

#define MAX_BUTTON 14
static HWND hwndButton[MAX_BUTTON];

// ----------
static OPENFILENAME ofn ;

static char    szNewFile   [_MAX_PATH] ;
static char    szFileName  [_MAX_PATH] ;
static char    szTitleName [_MAX_FNAME + _MAX_EXT] ;

static char    szCopyFileName  [_MAX_PATH] = "";
static char    szCopyTitleName [_MAX_FNAME + _MAX_EXT];

char szAppName[] = "FST Project";

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void FileInitialize (HWND hwnd)
{
	ofn.lStructSize       = sizeof (OPENFILENAME) ;
	ofn.hwndOwner         = hwnd ;
	ofn.hInstance         = NULL ;
	ofn.lpstrFilter       = NULL ;          // Set in Open, Create
	ofn.lpstrCustomFilter = NULL ;
	ofn.nMaxCustFilter    = 0 ;
	ofn.nFilterIndex      = 0 ;
	ofn.lpstrFile         = NULL ;          // Set in Open and Close functions
	ofn.nMaxFile          = _MAX_PATH ;
	ofn.lpstrFileTitle    = NULL ;          // Set in Open and Close functions
	ofn.nMaxFileTitle     = _MAX_FNAME + _MAX_EXT ;
	ofn.lpstrInitialDir   = NULL ;
	ofn.lpstrTitle        = NULL ;
	ofn.Flags             = 0 ;             // Set in Open and Close functions
	ofn.nFileOffset       = 0 ;
	ofn.nFileExtension    = 0 ;
	ofn.lpstrDefExt       = NULL ;
	ofn.lCustData         = 0L ;
	ofn.lpfnHook          = NULL ;
	ofn.lpTemplateName    = NULL ;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
BOOL FileOpenDlg (HWND hwnd, LPSTR lpstrFileName, LPSTR lpstrTitleName)
{
	static char *str, szInitialDir[MAXPATH];
	static char *szFilter[] = { "Project Files (*.FPJ)",  "*.fpj", "" } ;

	// set initial directory to current project path
	strcpy(szInitialDir, path_name);
	for (str = szInitialDir; *str; str++)	// find end of path
	  ;
	str--;
	if (*str == '\\') {	// remove \ from end of path
		*str = '\0';
	}

	ofn.lpstrFilter		= szFilter[0];
	ofn.lpstrDefExt		= "fpj";
	ofn.hwndOwner			= hwnd;
	ofn.lpstrTitle			= "Open Project";
	ofn.lpstrFile			= lpstrFileName;
	ofn.lpstrFileTitle		= lpstrTitleName;
	ofn.lpstrInitialDir		= szInitialDir;
	ofn.Flags				= OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

	return GetOpenFileName (&ofn) ;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
BOOL FileCreateDlg (HWND hwnd)
{
	static char szInitialDir[MAXPATH];

	strcpy(szInitialDir, "\\FST");

	ofn.lpstrTitle			= "Create Project";
	ofn.lpstrFilter		= NULL ;
	ofn.lpstrDefExt		= NULL ;
	ofn.hwndOwner			= hwnd ;
	ofn.lpstrFile			= szNewFile ;
	ofn.lpstrFileTitle		= NULL ;
	ofn.lpstrInitialDir		= szInitialDir;	// start search with current project
	ofn.Flags				= NULL;

	return GetOpenFileName (&ofn) ;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
BOOL FileCopyDlg (HWND hwnd, LPSTR lpstrFileName, LPSTR lpstrTitleName)
{
	static char *str, szInitialDir[MAXPATH];
	static char *szFilter[] = { "Shape files (*.fsd)", "*.fsd", "Color files (*.fcd)", "*.fcd", "Cockpit files (*.fgd)", "*.fgd", "Model files (*.fmd)", "*.fmd", "Terrain data (*.ftd)", "*.ftd", "Bitmaps (*.pcx)", "*.pcx", "Sound Samples (*.wav)", "*.wav", "All Files (*.*)",  "*.*",	"" } ;
	char drive[MAXDRIVE];
	char dir[MAXDIR];
	char name[MAXFILE];
	char ext[MAXEXT];

	// set initial directory to last copy or current project path if 1st time
	if (strlen(lpstrFileName) > 0) {
		fnsplit(lpstrFileName, drive, dir, name, ext);
		fnmerge(szInitialDir,  drive, dir, NULL, NULL);
		lpstrFileName[0] = '\0';
	}
	else {
		strcpy(szInitialDir, path_name);
	}

	// remove \ from end of path
	for (str = szInitialDir; *str; str++)	// find end of path
		;
	str--;
	if (*str == '\\') {	// remove \
		*str = '\0';
	}

	ofn.lpstrFilter		= szFilter[0] ;
	ofn.lpstrDefExt		= NULL;

	ofn.lpstrTitle			= "Copy File";
	ofn.hwndOwner			= hwnd ;
	ofn.lpstrFile			= lpstrFileName ;
	ofn.lpstrFileTitle		= lpstrTitleName ;
	ofn.lpstrInitialDir		= szInitialDir;	// start search with current project
	ofn.Flags				= OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

	return GetOpenFileName (&ofn) ;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void set_window_title(void)
{
	char text[250];

	sprintf(text, "FST-98: %s", path_name);
	SetWindowText(main_window, text);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
char* proj_filename(char *name)
{
	static char filename[128];

	if (strlen(path_name) > 0) {
		sprintf(filename, "%s%s", path_name, name);
	}
	else {
		sprintf(filename, "%s", name);
	}
	return filename;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Client code
void force_top_redraw(void)
{
	RECT rect;

	GetClientRect (main_window, &rect);
	InvalidateRect(main_window, &rect, TRUE);
	UpdateWindow(main_window);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void draw_rect(HDC hdc, int x, int y, int dx, int dy, HBRUSH brush)
{
	RECT rect;

	rect.left		= x;
	rect.top		= y;
	rect.right	= x + dx;
	rect.bottom	= y + dy;

	FillRect(hdc, &rect, brush);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void calc_latlong(long int x, long int z, double *lat, double *lng)
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

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void draw_status(int flag)	//HDC hdc,
{
	HDC		hdc;
	HFONT	hfont, hfontold;
	RECT		main_rect, rect;
	char		text[32];
	double	lng, lat, min1, min2;
	int		deg1, deg2, ofst, r_bot, r_top;

	hdc		= GetDC(main_window);
	hfont	= CreateFont(-12,0,0,0,0,0,0,0,0,0,0,0,0,"Arial");
	hfontold	= SelectObject(hdc, hfont);

	GetClientRect (main_window, &main_rect);

	SetBkMode(hdc,TRANSPARENT);

	r_top = 16;
	r_bot =  3;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	rect			= main_rect;
	rect.top		= main_rect.bottom - r_top;
	rect.bottom	= main_rect.bottom - r_bot;
	rect.left		= main_rect.left + 3;
	rect.right	= main_rect.left + 151;
	FillRect(hdc, &rect, GetStockObject(LTGRAY_BRUSH));

	SetTextColor(hdc, RGB(  0,  0, 128));
	sprintf(text, "X: %ld   Z: %ld Km", x_sel / 1000, z_sel / 1000);
	DrawText (hdc, text, -1, &rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
	hilight_rectangle(hdc, rect.left - 1, rect.top - 1, rect.right - rect.left + 1, rect.bottom - rect.top + 1, FALSE);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	calc_latlong(x_sel, z_sel, &lat, &lng);
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

	rect			= main_rect;
	rect.top		= main_rect.bottom - r_top;
	rect.bottom	= main_rect.bottom - r_bot;
	rect.left		= main_rect.left + 157;
	rect.right	= main_rect.left + 413;
	FillRect(hdc, &rect, GetStockObject(LTGRAY_BRUSH));

	SetTextColor(hdc, RGB( 128, 0, 0));
	sprintf(text, "%s%02dº %05.2f'  -by-  %s%03dº %05.2f'", (lat > 0) ? "North: " : "South: ", deg1, min1, (lng > 0) ? "East: " : "West: ", deg2, min2);
	DrawText (hdc, text, -1, &rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
	hilight_rectangle(hdc, rect.left - 1, rect.top - 1, rect.right - rect.left + 1, rect.bottom - rect.top + 1, FALSE);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	hilight_rectangle(hdc, 156, 0, 257, 257, FALSE);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	ReleaseDC(main_window, hdc);
	SelectObject(hdc, hfontold);
	DeleteObject(hfont);
}

/*
//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void redraw_start(HDC hdc, RECT *rect, BOOL flag)
{
	char		text[80];
	double	lng, lat, min1, min2;
	int		deg1, deg2, ofst;

	rect = rect;
	ofst = 20;

	if (flag) {
		BitBlt(hdc, 4, 0, 150, 255, hdc_bitmap, 0, 0, SRCCOPY);
	}
	hilight_rectangle(hdc, 157, 0, 254, 254, FALSE);
	hilight_rectangle(hdc, 158, 1, 252, 252, FALSE);

	//calc_latlong(x_sel, z_sel, &lat, &lng);
	//deg1 = (int)(abs(lat) / 60.0);
	//if (lat > 0) {
	//	min1 = lat - (deg1 * 60.0);
	//}
	//else {
	//	min1 = -lat - (deg1 * 60.0);
	//}
	//deg2 = (int)(abs(lng) / 60.0);
	//if (lng > 0) {
	//	min2 = lng - (deg2 * 60.0);
	//}
	//else {
	//	min2 = -lng - (deg2 * 60.0);
	//}
	//SetBkColor(hdc, RGB(192, 192, 192));
	//sprintf(text, "%s%02d°%05.2f'   %s%03d°%05.2f'", (lat>0) ? "N: " : "S: ", deg1, min1, (lng>0) ? "E: " : "W: ", deg2, min2);
	//TextOut(hdc, 8, rect->bottom - ofst,  text, strlen(text));
	//hilight_rectangle(hdc, 4, rect->bottom - ofst, 190, 15, FALSE);

	//sprintf(text, "x = %ld Km, z = %ld Km     ", x_sel/1000, z_sel/1000);
	//TextOut(hdc, 220, rect->bottom - ofst, text, strlen(text));
	//hilight_rectangle(hdc, 208, rect->bottom - ofst, 180, 15, FALSE);

	ReleaseDC(main_window, hdc);
}
*/

/*
//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void draw_status(void)
{

	redraw_start(FALSE);
}
*/

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
static BOOL read_project_data(void)
{
	char text[80];
	FILE *fp;

	// create path_name specifying current project directory Read configuration data for current project
	sprintf(szFileName, "%s%s", path_name, "fst.fpj");
	fp = fopen(szFileName, "r");
	if (fp == NULL) {
		MessageBox(NULL, "Can't locate file:  FST.FPJ", "FST Project Error!", MB_ICONEXCLAMATION);
		fclose(fp);
	}
	else {
		fscanf(fp, "%s", project_title);
		fscanf(fp, "%d %d", &width, &height);
		fscanf(fp, "%s\n", colours_file);
		fscanf(fp, "%d %d\n", &latitude, &longitude);
		fclose(fp);
		resize();
		return TRUE;
	}
	return FALSE;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
// sets current project, used szFileName to specify new project
int set_start(void)
{
	FILE *fp;
	char prj_name[150];
	char drive[MAXDRIVE];
	char dir[MAXDIR];
	char name[MAXFILE];
	char ext[MAXEXT];
	char text[80];

	// create complete path to master FST project file
	sprintf(prj_name, "%s\\%s", tools_path, "project.dat");

	// Extract path_name from szFileName (name of selected project file)
	fnsplit(szFileName, drive, dir, name, ext);
	fnmerge(path_name,  drive, dir, NULL, NULL);

	// open FST master project file and save new path name to file
	fp = fopen(prj_name, "w");
	if (fp == NULL) {
		MessageBox(NULL, "Can't open file:  PROJECT.DAT", "FST Project Error!", MB_ICONEXCLAMATION);
		return FALSE;
	}
	else {
		fprintf(fp, "%s\n", path_name);
		fclose(fp);
	}
	read_project_data();
	set_window_title();
	return TRUE;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
static char *template_file(char *file)
{
	static char tf[256];

	sprintf(tf, "%s\\TEMPLATE\\%s", romtools_path, file);
	return tf;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
static char *dest_file(char *file)
{
	static char df[256];

	sprintf(df, "%s\\%s", new_project, file);
	return df;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void load_template_data(void)
{
	FILE	*fp;
	char	prj_name[150], text [80];
	int	i, n;

	// Files already loaded
	if (template_n > 0) {
		return;
	}

	fp = fopen(template_file("index.dat"), "r");
	if (fp == NULL) {
		MessageBox(NULL, "Can't locate file:  INDEX.DAT", "FST Project Error!", MB_ICONEXCLAMATION);
	}
	else {
		fscanf(fp, "%d", &n);
		template_n = n;
		for (i = 0; i < n; i++) {
			template_files[i] = malloc(32);
			fscanf(fp, "%s", template_files[i]);
		}
		fclose(fp);
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
// uses new_project string to create a new project if sucessful sets current project to new project else return FALSE
BOOL create_new(void)
{
	FILE		*fp;
	char		prj_name[150];
	OFSTRUCT	StrSrc, StrDest;
	HFILE	hSrc, hDest;
	int		i;

	load_template_data();

	if (mkdir(new_project) == 0) {
		LZStart();
		for (i = 0; i < template_n; i++) {
			hSrc  = LZOpenFile(template_file(template_files[i]), &StrSrc, OF_READ);
			hDest = LZOpenFile(dest_file(template_files[i]), &StrDest, OF_CREATE);
			CopyLZFile(hSrc, hDest);
			LZClose(hSrc);
			LZClose(hDest);
		}
		LZDone();
		sprintf(szFileName, "%s\\FST.FPJ", new_project);
		if (!set_start()) {
			return FALSE;
		}
		return TRUE;
	}
	return FALSE;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Copy file "source" into current dir
BOOL copy_file(char *source, char *title)
{
	OFSTRUCT StrSrc, StrDest;
	HFILE    hSrc, hDest;
	long     status;

	LZStart();

	hSrc  = LZOpenFile(source, &StrSrc, OF_READ);
	if (hSrc == -1) {
		LZDone();
		return FALSE;
	}

	hDest = LZOpenFile(proj_filename(title), &StrDest, OF_CREATE);
	if (hDest == -1) {
		LZClose(hSrc);
		LZDone();
		return FALSE;
	}

	status = CopyLZFile(hSrc, hDest);

	LZClose(hSrc);
	LZClose(hDest);

	LZDone();

	return (status > 0) ? TRUE : FALSE;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void write_fst_fpj(void)
{
	FILE *fp;
	char prj_name[150], text[80];

	sprintf(prj_name, "%s\\%s", path_name, "fst.fpj");

	fp = fopen(prj_name, "w");
	if (fp == NULL) {
		MessageBox(NULL, "Can't open file:  FST.FPJ", "FST Project Error!", MB_ICONEXCLAMATION);
	}
	else {
		fprintf(fp, "%s\n", project_title);
		fprintf(fp, "%d %d\n", width, height);
		fprintf(fp, "%s\n", colours_file);
		fprintf(fp, "%d %d\n", latitude, longitude);
		fclose(fp);
	}
	set_window_title();
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void exec_tool(char *tool)
{
	char name[150];

	sprintf(name, "%s\\%s", romtools_path, tool);
	WinExec(name, SW_SHOW);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void exec_tool2(char *tool)
{
	char name[150];

	sprintf(name, "%s\\%s", path_name, tool);
	WinExec(name, SW_SHOW);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void exec_world(void)
{
	char name[150];

	sprintf(name, "%s\\%s %ld %ld %d %d", romtools_path, "WORLD.EXE", x_sel, z_sel, longitude, latitude);
	//MessageBox(NULL, name, "FST Project:  WORLD Start", MB_ICONINFORMATION);
	WinExec(name, SW_SHOW);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Project manager expects to find a file called project.dat in current directory, this contains path of current project and prefs data (if any)
int setup_start(void)
{
	FILE *fp;
	char master_prj_file[MAXPATH];
	char romtools_file[MAXPATH];
	char text[80];

	width  = 6;
	height = 6;

	// set tools directory to CWD (assumes that project.exe run from FST tool directory),
	getcwd(tools_path, MAXPATH);
	tools_drive = getdisk();

	// read current FST project path
	sprintf(master_prj_file, "%s\\%s", tools_path, "project.dat");
	fp = fopen(master_prj_file, "r");
	if (fp == NULL) {
		MessageBox(NULL, "Can't locate file:  PROJECT.DAT", "FST Project Error!", MB_ICONEXCLAMATION);
		return FALSE;
	}
	else {
		fscanf(fp, "%s", path_name);
		fclose(fp);
	}

	// If FST tools are on Read Only Media (CD), then ROMTOOLS.DAT contains full path of tools
	sprintf(romtools_file, "%s\\%s", tools_path, "romtools.dat");
	fp = fopen(romtools_file, "r");
	if (fp != NULL) {
		fscanf(fp, "%s", romtools_path);
		fclose(fp);
	}
	else {
		strcpy(romtools_path, tools_path);
	}
	if (!read_project_data()) {
		return FALSE;
	}
	return TRUE;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void set_tools_cwd(void)
{
	// makes CWD the tools directory - as setup at start
	setdisk(tools_drive);
	chdir(tools_path);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void tidyup_start(void)
{
	int i;
	HDC hdc;

	ReleaseDC(main_window, hdc);
	tidyup_tile();

	//UnregisterClass(szName, hInst);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
long FAR PASCAL _export WndProc (HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
	HFONT		hfont, hfontold;
	HMENU		hm;
	HDC			hdc;
	PAINTSTRUCT	ps;
	RECT			rect;
	FARPROC		lpProc;
	static int	cxChar, cyChar;
	int			i, button_start, button_height;
	TEXTMETRIC	tm ;

	switch (message) {

		case WM_GETMINMAXINFO:
			// set the MINMAXINFO structure pointer
			lpMinMaxInfo = (MINMAXINFO FAR *) lParam;
			lpMinMaxInfo->ptMinTrackSize.x = FORM_MIN_WIDTH;
			lpMinMaxInfo->ptMinTrackSize.y = FORM_MIN_HEIGHT;
			lpMinMaxInfo->ptMaxTrackSize.x = FORM_MAX_WIDTH;
			lpMinMaxInfo->ptMaxTrackSize.y = FORM_MAX_HEIGHT;
			break;

		case WM_CREATE:
			hbm_bitmap = LoadBitmap(hInst, "BITMAP_2");
			hdc = GetDC (hwnd);
			hdc_bitmap = CreateCompatibleDC(hdc);
			hbm_old_bitmap = SelectObject(hdc_bitmap, hbm_bitmap);
			SelectObject (hdc, GetStockObject (CreateFont(-12,0,0,0,0,0,0,0,0,0,0,0,0,"Arial")));
			GetTextMetrics (hdc, &tm) ;
			cxChar = tm.tmAveCharWidth;
			cyChar = tm.tmHeight + tm.tmExternalLeading;
			ReleaseDC (hwnd, hdc);
			button_start  = 77 - (20 * cxChar) / 2;
			button_height = 20;
			for (i = 0; i < 11; i++) {
				hwndButton [i] = CreateWindow (
					"button",								// object type
					button_names[i],						// object text
					WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,		// properties
					button_start,							// x position
					5 + ((button_height + 1) * i),			// y position
					20 * cxChar,							// width
					button_height - 1,						// height
					hwnd,								//
					i,									// index?
					((LPCREATESTRUCT) lParam) -> hInstance,		//
					NULL);								//
			}
			hm = GetMenu(main_window);
			CheckMenuItem(hm, IDM_LINES, (grid_type == 1) ? MF_CHECKED : MF_UNCHECKED);
			CheckMenuItem(hm, IDM_DOTS , (grid_type == 2) ? MF_CHECKED : MF_UNCHECKED);
			CheckMenuItem(hm, IDM_OFF  , (grid_type == 0) ? MF_CHECKED : MF_UNCHECKED);
			CheckMenuItem(hm, IDM_MAP  , (show_map)       ? MF_CHECKED : MF_UNCHECKED);
			return 0;

		case WM_PAINT:
			hdc = BeginPaint (hwnd, &ps);
			GetClientRect (hwnd, &rect);
			draw_status(TRUE);
			EndPaint (hwnd, &ps);
			BitBlt(hdc, 2, 0, 150, 258, hdc_bitmap, 0, 0, SRCCOPY);
			return 0;

		case WM_COMMAND:

			switch (wParam) {

				case 0:
					exec_world();
					break;

				case 1:
					exec_tool("shape");
					break;

				case 2:
					exec_tool("fst-colr");
					break;

				case 3:
					exec_tool("texture");
					break;

				case 4:
					exec_tool("cockpit");
					break;

				case 5:
					exec_tool("model");
					break;

				case 6:
					exec_tool("fst-view");
					break;

				case 7:
					exec_tool("fst-font");
					break;

				case 8:
					exec_tool("fst-purg");
					break;

				case 9:
					exec_tool2("fst-fly");
					break;

				case 10:
					exec_tool2("fly.exe");
					break;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

				// Menu items
				case IDM_ABOUT:
					lpProc = MakeProcInstance(db_about_proc, hInst);
					DialogBox(hInst, "DIALOG_3", hwnd, lpProc);
					FreeProcInstance(lpProc);
					break;

				case IDM_OPEN:
					if (FileOpenDlg (hwnd, szFileName, szTitleName)) {
						set_start();
						force_top_redraw();
						set_tools_cwd();
					}
					break;

				case IDM_CREATE:
					if (FileCreateDlg (hwnd)) {
						// szNewFile will contain full path spec
						sprintf(new_project, "%s", szNewFile);
						if (create_new()) {
							force_top_redraw();
						}
						else {
							MessageBox(hwnd, new_project, "Failed to Create New Project", MB_ICONEXCLAMATION);
							set_tools_cwd();
						}
					}
					break;

				case IDM_COPY:
					if (FileCopyDlg (hwnd, szCopyFileName, szCopyTitleName)) {
						// szFileName will contain full path spec
						if (!copy_file(szCopyFileName, szCopyTitleName)) {
							MessageBox(hwnd, szCopyFileName, "Failed to copy", MB_ICONEXCLAMATION);
						}
						else {
							MessageBox(hwnd, szCopyFileName, "Copied", MB_ICONINFORMATION);
							set_tools_cwd();
						}
					}
					break;

				case IDM_SIZE:
					lpProc = MakeProcInstance(db_size_proc, hInst);
					if (DialogBox(hInst, "DIALOG_2", hwnd, lpProc)) {
						resize();
						write_fst_fpj();
						force_top_redraw();
					}
					FreeProcInstance(lpProc);
					break;

				case IDM_ORIGIN:
					lpProc = MakeProcInstance(db_origin_proc, hInst);
					if (DialogBox(hInst, "DIALOG_4", hwnd, lpProc)) {
						write_fst_fpj();
						force_top_redraw();
					}
					FreeProcInstance(lpProc);
					break;

				case IDM_COLORS:
					lpProc = MakeProcInstance(db_colours_proc, hInst);
					if (DialogBox(hInst, "DIALOG_10", hwnd, lpProc)) {
						write_fst_fpj();
					}
					FreeProcInstance(lpProc);
					break;

				case IDM_MAP:
					show_map = !show_map;
					hm = GetMenu(main_window);
                        	CheckMenuItem(hm, IDM_MAP, (show_map) ? MF_CHECKED : MF_UNCHECKED);
                         force_top_redraw();
					break;

				case IDM_LINES:
                    	grid_type = 1;
					hm = GetMenu(main_window);
                        	CheckMenuItem(hm, IDM_LINES, (grid_type == 1) ? MF_CHECKED : MF_UNCHECKED);
                        	CheckMenuItem(hm, IDM_DOTS , (grid_type == 2) ? MF_CHECKED : MF_UNCHECKED);
                        	CheckMenuItem(hm, IDM_OFF  , (grid_type == 0) ? MF_CHECKED : MF_UNCHECKED);
                         force_top_redraw();
					break;

				case IDM_DOTS:
                    	grid_type = 2;
					hm = GetMenu(main_window);
                        	CheckMenuItem(hm, IDM_LINES, (grid_type == 1) ? MF_CHECKED : MF_UNCHECKED);
                        	CheckMenuItem(hm, IDM_DOTS , (grid_type == 2) ? MF_CHECKED : MF_UNCHECKED);
                        	CheckMenuItem(hm, IDM_OFF  , (grid_type == 0) ? MF_CHECKED : MF_UNCHECKED);
                         force_top_redraw();
					break;

				case IDM_OFF:
                    	grid_type = 0;
					hm = GetMenu(main_window);
                        	CheckMenuItem(hm, IDM_LINES, (grid_type == 1) ? MF_CHECKED : MF_UNCHECKED);
                        	CheckMenuItem(hm, IDM_DOTS , (grid_type == 2) ? MF_CHECKED : MF_UNCHECKED);
                        	CheckMenuItem(hm, IDM_OFF  , (grid_type == 0) ? MF_CHECKED : MF_UNCHECKED);
                         force_top_redraw();
					break;

				case IDM_HELP:
					WinHelp(hwnd,"FST.HLP", HELP_INDEX, 0L);
					break;

				case IDM_EXIT:
					DestroyWindow(hwnd);
					PostQuitMessage (0);
					return 0;
			}
			return 0;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

		case WM_DESTROY:
			SelectObject(hdc_bitmap, hbm_old_bitmap);
			DeleteDC(hdc_bitmap);
			DeleteObject(hbm_bitmap);
			DeleteObject(SetClassWord (hwnd, GCW_HBRBACKGROUND, GetStockObject (WHITE_BRUSH)));
			tidyup_start();
			PostQuitMessage (0);
			return 0;
	}
	return DefWindowProc (hwnd, message, wParam, lParam);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
static BOOL FAR PASCAL db_about_proc(HWND hdlg, unsigned msg, WORD wParam, LONG lParam)
{
	lParam = lParam;

	switch(msg) {

		case WM_INITDIALOG:
			// Updates 'Adout' dialog to current version number
			SetDlgItemText(hdlg, ID_VERSION, prog_version);
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
	}
	return(FALSE);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
static BOOL FAR PASCAL db_size_proc(HWND hdlg, unsigned msg, WORD wParam, LONG lParam)
{
	int w, h;
	BOOL ok;

	lParam = lParam;

	switch(msg) {

		case WM_INITDIALOG:
			SetDlgItemInt(hdlg, ID_VALUE, width, FALSE);
			SetDlgItemInt(hdlg, ID_VALUE1, height, FALSE);
			return(TRUE);

		case WM_COMMAND:

			switch (wParam) {

				case ID_VALUE:
					return TRUE;

				case IDOK:
					w = GetDlgItemInt(hdlg, ID_VALUE, &ok, FALSE);
					if (ok) {
						h = GetDlgItemInt(hdlg, ID_VALUE1, &ok, FALSE);
					}
					if (ok) {
						width = w;
						height = h;
						EndDialog(hdlg, TRUE);
					}
					else {
						EndDialog(hdlg, TRUE);
					}
					return(TRUE);

				case IDCANCEL:
					EndDialog(hdlg, FALSE);
					return(TRUE);
			}
			break;
	}
	return(FALSE);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
static BOOL FAR PASCAL db_origin_proc(HWND hdlg, unsigned msg, WORD wParam, LONG lParam)
{
	static int  lat_n, long_e;
	int  lat_d, lat_s, long_d, long_s;
	char s[32];
	BOOL ok;

	lParam = lParam;

	switch(msg) {

		case WM_INITDIALOG:
			long_e = (longitude > 0) ? TRUE : FALSE;
			lat_n  = (latitude > 0)  ? TRUE : FALSE;
			SetDlgItemInt(hdlg, ID_VALUE,  abs(latitude)/60, FALSE);
			SetDlgItemInt(hdlg, ID_VALUE1, abs(latitude)%60, FALSE);
			SetDlgItemInt(hdlg, ID_VALUE2, abs(longitude)/60, FALSE);
			SetDlgItemInt(hdlg, ID_VALUE3, abs(longitude)%60, FALSE);
			SetDlgItemText(hdlg, ID_LONG, long_e ? "E" : "W");
			SetDlgItemText(hdlg, ID_LAT,  lat_n  ? "N" : "S");
			return(TRUE);

		case WM_COMMAND:

			switch (wParam) {

				case ID_LAT:
					lat_n = !lat_n;
					SetDlgItemText(hdlg, ID_LAT,  lat_n ? "N" : "S");
					return TRUE;

				case ID_LONG:
					long_e = !long_e;
					SetDlgItemText(hdlg, ID_LONG, long_e ? "E" : "W");
					return TRUE;

				case IDOK:
					lat_d = GetDlgItemInt(hdlg, ID_VALUE, &ok, FALSE);
					if (ok) {
						lat_s = GetDlgItemInt(hdlg, ID_VALUE1, &ok, FALSE);
					}
					if (ok) {
						long_d = GetDlgItemInt(hdlg, ID_VALUE2, &ok, FALSE);
					}
					if (ok) {
						long_s = GetDlgItemInt(hdlg, ID_VALUE3, &ok, FALSE);
					}
					if (ok) {
						latitude  = lat_d * 60 + lat_s;
						longitude = long_d * 60 + long_s;
						if (!lat_n) latitude = -latitude; {
							if (!long_e) longitude = -longitude; {
								EndDialog(hdlg, TRUE);
							}
						}
					}
					else {
						EndDialog(hdlg, TRUE);
					}
					return(TRUE);

				case IDCANCEL:
					EndDialog(hdlg, FALSE);
					return(TRUE);
			}
			break;
	}
	return(FALSE);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
static BOOL FAR PASCAL db_colours_proc(HWND hdlg, unsigned msg, WORD wParam, LONG lParam)
{
	char s[80];

	lParam = lParam;

	switch(msg) {

		case WM_INITDIALOG:
			SendDlgItemMessage(hdlg, ID_DB_VALUE, CB_DIR, 0, (LPARAM)((LPSTR)proj_filename("*.fcd")));
			SendDlgItemMessage(hdlg, ID_DB_VALUE, CB_SELECTSTRING, -1, (LPARAM)colours_file);
			return(TRUE);

		case WM_COMMAND:

			switch (wParam) {

				case IDOK:
					if (GetDlgItemText(hdlg, ID_DB_VALUE, s, 16)) {
						strcpy(colours_file, s);
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

/*
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Window setup routines
static BOOL init_window(HANDLE hInstance)
{
	WNDCLASS wndclass;

	wndclass.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wndclass.lpfnWndProc   = WndProc ;
	wndclass.cbClsExtra    = 0 ;
	wndclass.cbWndExtra    = 0 ;
	wndclass.hInstance     = hInstance ;
	wndclass.hIcon         = NULL ; //LoadIcon (NULL, IDI_APPLICATION);
	wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW) ;
	wndclass.hbrBackground = CreateSolidBrush(RGB(96, 96, 96));
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

	main_window = CreateWindow (
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
}
*/

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Windows Main Entry point
int PASCAL WinMain (HANDLE hInstance, HANDLE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{
	static char szAppName[] = "GSF" ;
	HWND        hwnd ;
	MSG         msg;
	WNDCLASS    wndclass ;
	int i, x, y, xDesktop, yDesktop, width, height;

	lpszCmdLine = lpszCmdLine;

	if (hPrevInstance) {
		MessageBox(NULL, "Cannot start more than one copy of Project Manager", "Cannot run program", MB_ICONEXCLAMATION);
		return FALSE;
	}

	//if (!init_window(hInstance)) {
	//	return FALSE;
	//}

	hInst = hInstance;

	// Create base window class
	wndclass.style         = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc   = WndProc ;
	wndclass.cbClsExtra    = 0 ;
	wndclass.cbWndExtra    = 0 ;
	wndclass.hInstance     = hInstance ;
	wndclass.hIcon         = NULL ;
	wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW) ;
	wndclass.hbrBackground = GetStockObject(LTGRAY_BRUSH);
	wndclass.lpszMenuName  = "MENU_1";
	wndclass.lpszClassName = szAppName ;

	RegisterClass (&wndclass) ;

	init_tile();

	if (!setup_start()) {
		return FALSE;
	}

	xDesktop = GetSystemMetrics(SM_CXSCREEN);
	yDesktop = GetSystemMetrics(SM_CYSCREEN);

	width  = FORM_MIN_WIDTH;
	height = FORM_MIN_HEIGHT;

	x = (xDesktop - width) / 2;
	y = (yDesktop - height) / 2;

	// Create base window
	hwnd = CreateWindow (
		szAppName,
		"GSF",
		WS_THICKFRAME |WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX,
		x,
		y,
		width,
		height,
		NULL,
		NULL,
		hInstance,
		NULL) ;

	main_window = hwnd;

	set_window_title();

	// Create tile window
	create_tile_window(hwnd, hInst, 157, 1, 256, 256);

	setup_tile();

	FileInitialize (main_window) ;
	ShowWindow (hwnd, nCmdShow) ;
	UpdateWindow (hwnd);

	// Main loop
	while (GetMessage (&msg, NULL, 0, 0)) {
		if (hDlgModeless == 0 || !IsDialogMessage (hDlgModeless, &msg)) {
			TranslateMessage (&msg) ;
			DispatchMessage  (&msg) ;
		}
	}
	return msg.wParam;
}
