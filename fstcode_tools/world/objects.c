/* FST Terrain Editor */
/* Object editing functions */



#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "defs.h"
#include "lists.h"
#include "terrain.h"

#define ABS(X) (((X) < 0) ? -(X) : (X))

#ifndef MAX
	#define MAX(X,Y) (((X) > (Y)) ? (X) : (Y))
#endif

extern int box_size;

extern int redraw;

extern int show_paths;
extern int show_selpath;
extern int show_objects;
extern int show_texture;
extern int show_teams;

extern void draw_shape(HDC hdc, void *hshape, int wx, int wy, int ry, int x_size, int y_size);

extern long int current_shape_ycentre(void);

// All objects defined by Object_def structure, void *props - extended class specific structures
// defined by XXX_props structures relative height (->height) is used in dialog box, and for copy,
// move fns abs. height ->py is primary measure

typedef struct {
	long	px, py, pz;			// Position in M
	int	ry, height;			// Rotation (degrees), height offset,
	int	flags;				// PLAYER, LAND_ON, TARGET
	int	strength;				// strength in kp's
	char	name[16];				// name of shape includes .FSD
	char	symbol[16];			// symbolic name of object (nil by default)
	char	dead_shape[16];		// name of dead shape includes .FSD
	int	type;				// Class of object
	void	*props;				// extended properties
	void	*hshape;				// handle of shape
	char	extra_name[16];		// allows extra classes (treated as Cultural)
} Object_def;

#define KP_INDESTRUCT -1
#define KP_INVISIBLE   0

typedef struct {
	float freq;
} Nav_props;

#define MAX_ORD 7

int max_ord = MAX_ORD;			// Set by load fn(world file version)

typedef struct {
	int n, kp;
} Ordnance;

typedef struct {
	char	gauges[32];
	char	model[32];
	char	gear_down[32];
	char damaged[32];
	char	supersonic[32];
	int	rear_gun;
	int	nord;
	int	chaff;
	int	flare;
  	Ordnance ord[MAX_ORD];
} Plane_props;

// Fixed defences CLASS_AAGUN, CLASS_SAM
#define MAX_FD 3

typedef struct {
	int fixed;
	int data[MAX_FD];			// burst_rate, burst time, reload OR range, kp, floor
} FD_props;

/* AI models */
#define FIGHTER_MODEL 1
#define CIVIL_MODEL   2
#define BOMBER_MODEL  3
#define STRAFER_MODEL 4

/* Flag bits */
#define REAR_GUN 0x01
#define PREDICT  0x02
#define AAGUN	  0x04


typedef struct {
	char vehicle_shape[16];		// Depot and Hanger
	char ac_model[16];			// Hanger
	char ac_cockpit[16];		// Hanger
	int  speed;				// Depot
	int  ai_model;				// Hanger (XXX_MODEL)
	int  bombs;				// Hanger
	int  initial;				// Depot & Hanger
	int  interval;				// Depot
	int  size;				// Depot
	int  sub_interval;			// Depot
	int  vehicle_strength;		// Depot & Hanger
	int  shells;				// Depot
	int  fire_delay;			// Depot
	int  flags;				// Depot and Hanger
	List *paths;				// Depot and Hanger
} Depot_props;

// Object_def flags - bits 0-7 system use, masked of by application, bits 8-15 used by application
#define PLAYER  		0x0001
#define TARGET  		0x0100
#define LAND_ON 		0x0200
#define BLUE_TARGET 	0x0400

/* Object def classes */
#define MAX_CLASS 20

#define CLASS_CULTURAL	1
#define CLASS_MILITARY	2
#define CLASS_CIVIL		3
#define CLASS_BRICK   	4
#define CLASS_NDB    	5
#define CLASS_VOR		6
#define CLASS_ILS		7
#define CLASS_AAGUN		8
#define CLASS_SAM		9
#define CLASS_DEPOT		10
#define CLASS_HANGER	11
#define CLASS_ANIM		12		// <<<<<<<<<<<<<<<<<<<< Added ANIM class

static BOOL paths_string_loaded = FALSE;	// Set if "Paths" read while searching for object class name

static char *class_names[MAX_CLASS];		// strings naming classes indexed by class id
static List *objects = NULL;				// list of all objects in world

static char shape_name[12]	= "BOX.FSD";		// default shape file
static int  default_class	= CLASS_CULTURAL;	// default used when creating a new object
static int  default_angle	= 0;				// default for new object (0 - 360)
static int  default_height	= 0;				// default for new object - height above terrain
static int  default_strength	= 100;			// default for new object - height above terrain
static char dead_name[12]	= "BOX.FSD";		// default shape file

static Object_def *selected_object = NULL;
static Object_def *player = NULL;

static Object_def *object_copy = NULL;

//static char *empty_string = "";
//static char msg[80];

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// initialises list box to list of files matching filedef, selectes file as current
static void init_cb(HWND hdlg, int id, char *file, char *filedef, int none)
{
	SendDlgItemMessage(hdlg, id, CB_DIR, 0, (LPARAM)((LPSTR)proj_filename(filedef)));

	if (none) {
		SendDlgItemMessage(hdlg, id, CB_ADDSTRING, 0, (LPARAM)"(none)");
		SendDlgItemMessage(hdlg, id, CB_SELECTSTRING, -1, (LPARAM)"(none)");
	}
	SendDlgItemMessage(hdlg, id, CB_SELECTSTRING, -1, (LPARAM)file);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void read_cb(HWND hdlg, int id, char *file)
{
	char s[32];

	GetDlgItemText(hdlg, id, s, 32);

	if (strcmp(s, "(none)") != 0) {
		strcpy(file, s);
	}
	else {
		strcpy(file, "");
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Useful general functions
static int str_eq(char *s1, char *s2, int max)
{
	int eq;

	while (*s1 && *s2 && ((eq = (tolower(*s1++) == tolower(*s2++))) == TRUE) && (max-- > 0))
		;
	return ((eq == TRUE) && (*s1 == NULL) && (*s2 == NULL));
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Path structures, data and foward function defs, Path is list of x/y/z postions
typedef struct {
	long int x, y, z, speed, hat;	// Added Height Above Terrain (HAT) var
	int active;
} Posn;

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
typedef struct {
	char name[32];
	int	flags;
	List *posns;
	Object_def *supply;
} Path;

static List *paths = NULL;
static int next_path = 0;
static Path *selected_path  = NULL; // Current path
static Posn *selected_posn  = NULL; // Current position

static Path *find_path(char *name);
static Path *get_path(char *name);

void save_paths(FILE *fp);
void load_paths(FILE *fp);
void draw_path(HDC hdc, Path *pt);	//RECT *rc,
void draw_paths(HDC hdc);		//RECT *rc;

static void change_shape_name(Object_def *o, char *file_name);

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Class handling functions
static void new_class_def(int n, char *name)
{
	class_names[n] = malloc(sizeof(char)*12);
	strcpy(class_names[n], name);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// finds identifier for class defined by the string name
static int get_class(char *name)
{
	int i;

	for (i = 1; i < MAX_CLASS; i++) {
		if ((class_names[i] != NULL) && (strcmp(name, class_names[i]) == 0)) {
			return i;
		}
	}
	return 0;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void set_default_class(char *name)
{
	default_class = get_class(name);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// returns name of class of object od
static char *class_name(Object_def *od, int use_extra_name)
{
	char *name;

	name = NULL;
	if (od->type > 0 && od->type < MAX_CLASS) {
		if ((od->type == CLASS_CULTURAL) && use_extra_name) {
      name = class_names[od->type];
		 //	name = od->extra_name;
		}
		else {
			name = class_names[od->type];
		}
	}
	if (name == NULL) {
		name = "";
	}
	return name;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
BOOL is_copy_object(void)
{
	return (object_copy) ? TRUE : FALSE;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
BOOL is_selected_object(void)
{
	return (selected_object) ? TRUE : FALSE;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
BOOL is_selected_path(void)
{
	return (selected_path) ? TRUE : FALSE;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
BOOL is_selected_posn(void)
{
	return (selected_posn) ? TRUE : FALSE;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// EXTERN get_selected_name used by status line
char *get_selected_name(void)
{
	static char str[40];

	switch (current_tool) {
		case TOOL_PTH: case TOOL_PTHADD:
			if (selected_path != NULL) {
				sprintf(str, "%s", selected_path->name);
			}
			else {
				strcpy(str, " ");
			}
			break;

		default:
			if (selected_object != NULL) {
				sprintf(str, "%s '%s'", class_name(selected_object, TRUE), selected_object->name);
			}
			else {
				strcpy(str, " ");
			}
			break;
	}
	return str;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
BOOL selected_object_associable(void)
{
	if (selected_object) {
		return (selected_object->type == CLASS_DEPOT || selected_object->type == CLASS_HANGER);
	}
	return FALSE;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
BOOL get_selected_start(void)
{
	switch (current_tool) {

		case TOOL_PTHNEW: case TOOL_PTHADD:
			return (selected_posn) ? TRUE : FALSE;
		case TOOL_OBJLNK:
			return (selected_object) ? TRUE : FALSE;
	}
	return FALSE;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
int get_selected_wx(void)
{
	switch (current_tool) {

		case TOOL_PTHNEW: case TOOL_PTHADD:
			if (selected_posn) {
				return get_wx_mx(selected_posn->x);
			}
			break;
		case TOOL_OBJLNK:
			if (selected_object) {
				return get_wx_mx(selected_object->px);
			}
			break;
	}
	return 0;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
int get_selected_wy(void)
{
	switch (current_tool) {

		case TOOL_PTHNEW: case TOOL_PTHADD:
			if (selected_posn) {
				return get_wy_my(selected_posn->z);
			}
			break;
		case TOOL_OBJLNK:
			if (selected_object) {
				return get_wy_my(selected_object->pz);
			}
			break;
	}
	return 0;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Object data structure and database functions
static void setup_object_class_data(Object_def *od);

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static Object_def *alloc_object(void)
{
	Object_def *od;

	od = malloc(sizeof(Object_def));
	if (od == NULL) {
		warning("No memory for new object");
		return od;
	}
	od->px 		= 0L;
	od->pz 		= 0L;
	od->py 		= 0L;
	od->ry 		= 0;
	od->height 	= 0L;
	od->flags 	= 0;
	od->strength	= 100;
	od->hshape	= NULL;
	strcpy(od->name, shape_name);
	strcpy(od->symbol, "nil");
	strcpy(od->dead_shape, shape_name);
	od->type = default_class;
	od->props = NULL;

	setup_object_class_data(od);

	return od;
}

// Foward declaration
static void copy_props(Object_def *copy, Object_def *od);

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void copy_object(Object_def *copy, Object_def *od)
{
	// ensure that relative height is correct
	od->height = (int)od->py - get_point_height(od->px, od->pz);

	*copy = *od;			// copy contents of original structure
	copy_props(copy, od);	// allocate new props structure and copy old to new
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// shape name in object includes .FSD
static void load_object_shape(Object_def *o)
{
	char file_name[20];
 //	char mssg[64];
	long int cpy;

	if (o != NULL) {
		strcpy(file_name, selected_object->name);
		o->hshape = load_current_shape(file_name, selected_object->name);
		// Make sure object is above ground
		cpy = get_point_height(selected_object->px, selected_object->pz) + current_shape_ycentre();
		selected_object->py = MAX(cpy, selected_object->py);
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// finds identifier for class defined by the string name
static Object_def *get_object_symbolic(char *name)
{
	List *l;
	Object_def *od;

	if ((strlen(name) == 0) || str_eq(name, "nil", 16)) {
		return NULL;
	}

	for (l = objects->next; l != objects; l = l->next) {
		od = l->item;
		if (str_eq(name, od->symbol, 16)) {
			return od;
		}
	}
	return NULL;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Get path rect
static void get_path_rect(Path *path, Posn *posn, RECT *rc)
{
	int temp, wx1, wy1, wx2, wy2;
	Posn *p1, *p2;
	List *l;
	RECT tmp_rc, rect;

	rect.left	= 0;
	rect.top		= 0;
	rect.right	= 0;
	rect.bottom	= 0;
	p1 = p2 = NULL;
	for (l = path->posns->next; l != path->posns; l = l->next) {
		p1 = p2;      // move last p2 to p1
		p2 = l->item; // p2 is current posn in path
		// if function argument is in selected posns add bounding rect
		if (p1 && p2 && ((posn == NULL) || ((p1 == posn) || (p2 == posn)))) {
			wx1 = get_wx_mx(p1->x);
			wy1 = get_wy_my(p1->z);
			wx2 = get_wx_mx(p2->x);
			wy2 = get_wy_my(p2->z);
			if (wx1 > wx2) {
				temp = wx1; wx1 = wx2; wx2 = temp;
			}
			if (wy1 > wy2) {
				temp = wy1; wy1 = wy2; wy2 = temp;
			}
			get_terrain_rect(get_tx(wx1) - 1, get_ty(wy1) + 1, get_tx(wx2) + 1, get_ty(wy2) - 1, &tmp_rc);
			UnionRect(&rect, &tmp_rc, &rect);
		}
	}
	*rc = rect;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Get rect covering area between object and path start
static void get_link_rect(Object_def *od, Path *path, RECT *rc)
{
	int temp, wx1, wy1, wx2, wy2;
	Posn *p;

	// position of object in world
	wx1 = get_wx_mx(od->px);
	wy1 = get_wy_my(od->pz);
	p = Car(path->posns);
	if (p) {
		wx2 = get_wx_mx(p->x);
		wy2 = get_wy_my(p->z);
		if (wx1 > wx2) {
			temp = wx1; wx1 = wx2; wx2 = temp;
		}
		if (wy1 > wy2) {
			temp = wy1; wy1 = wy2; wy2 = temp;
		}
		get_terrain_rect(get_tx(wx1) - 1, get_ty(wy1) + 1, get_tx(wx2) + 1, get_ty(wy2) - 1, rc);
	}
	else {
		get_terrain_rect(get_tx(wx1) - 1, get_ty(wy1) + 1, get_tx(wx1) + 1, get_ty(wy1) - 1, rc);
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// returns screen region which object and associated paths occupy, Only second argument may be NULL
static void get_object_rect(Object_def *od1, Object_def *od2, RECT *rc)
{
	int temp, wx1, wy1, wx2, wy2;

	if (od2 != NULL) {
		wx1 = get_wx_mx(od1->px);
		wy1 = get_wy_my(od1->pz);
		wx2 = get_wx_mx(od2->px);
		wy2 = get_wy_my(od2->pz);
		if (wx1 > wx2) {
			temp = wx1; wx1 = wx2; wx2 = temp;
		}
		if (wy1 > wy2) {
			temp = wy1; wy1 = wy2; wy2 = temp;
		}
		get_terrain_rect(get_tx(wx1) - 1, get_ty(wy1) + 1, get_tx(wx2) + 1, get_ty(wy2) - 1, rc);
	}
	else {
		wx1 = get_wx_mx(od1->px);
		wy1 = get_wy_my(od1->pz);
		get_terrain_rect(get_tx(wx1) - 1, get_ty(wy1) + 1, get_tx(wx1) + 1, get_ty(wy1) - 1, rc);
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void destroy_objects(void)
{
	List *l;
	Object_def *od;

	for (l = objects->next; l != objects; l = l->next) {
		od = l->item;
		free(od);
	}
	list_destroy(objects);
	list_destroy(paths);
	selected_object = NULL;
	player = NULL;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// EXTERN remove_object
void remove_object(void)
{
	if (selected_object) {
		list_remove(selected_object, objects);
		selected_object = NULL;
		redraw_terrain();
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
//  EXTERN set_object_angle
void set_object_angle(int ry)
{
	RECT rect;

	if (selected_object) {
		selected_object->ry = ry;
		GetClientRect (edit_window, &rect);
		get_object_rect(selected_object, selected_object, &rect);
		InvalidateRect(edit_window, &rect, FALSE);
	}
}

/*
//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Not used - strips .FSD from string
static void extract_shape_name(char *file_name, char *shape_name)
{
	int i;

	for (i = 0; i < 8; i++) {
		if (file_name[i] == '.') {
			break;
		}
		shape_name[i] = file_name[i];
	}
	shape_name[i] = '\0';
}
*/

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// called from basic properties box, changes name string
// and loades shape from .FSD file for selected object to be drawn
// correctly with new shape
static void change_shape_name(Object_def *o, char *file_name)
{
	strcpy(o->name, file_name);
	load_object_shape(o);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Dialog proc for default values used when creating a new object
static BOOL FAR PASCAL db_default_proc(HWND hdlg, unsigned msg, WORD wParam, LONG lParam)
{
	int  i;
	char s[40];

	lParam = lParam;
	switch(msg) {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case WM_INITDIALOG:
			// Class list
			for (i = 0; i < MAX_CLASS; i++) {
				if (class_names[i] != NULL) {
					SendDlgItemMessage(hdlg, ID_DB6_CLASS, CB_ADDSTRING, 0, (LPARAM)((LPSTR)class_names[i]));
				}
			}
			SendDlgItemMessage(hdlg, ID_DB6_CLASS, CB_SELECTSTRING, -1, (LPARAM)class_names[default_class]);

			// Shape list
			init_cb(hdlg, ID_DB6_SHAPE, shape_name, "*.FSD", TRUE);

			// Heading
			sprintf(s, "%d", default_angle);
			SetDlgItemText(hdlg, ID_DB6_HEADING, s);

			// Height
			sprintf(s, "%d", default_height);
			SetDlgItemText(hdlg, ID_DB6_HEIGHT, s);

			// Object strength
			SetDlgItemInt(hdlg, ID_DB6_STRENGTH, default_strength, TRUE);

			// Dead shape
			init_cb(hdlg, ID_DB6_DSHAPE, dead_name, "*.FSD", TRUE);

			return(TRUE);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case WM_COMMAND:

			switch (wParam) {

				case IDOK:
					GetDlgItemText(hdlg, ID_DB6_CLASS, s, 16);
					set_default_class(s);

					read_cb(hdlg, ID_DB6_SHAPE	, shape_name);

					GetDlgItemText(hdlg, ID_DB6_HEADING, s, 4);
					default_angle = atoi(s);

					GetDlgItemText(hdlg, ID_DB6_HEIGHT, s, 5);
					default_height = atoi(s);

					GetDlgItemText(hdlg, ID_DB6_STRENGTH, s, 5);
					default_strength = atoi(s);

					read_cb(hdlg, ID_DB6_DSHAPE	, dead_name);

					EndDialog(hdlg, TRUE);
					return(TRUE);

				case IDCANCEL:
					EndDialog(hdlg, FALSE);
					return(TRUE);
			}
			break;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case WM_DESTROY:
			EndDialog(hdlg, FALSE);
			return TRUE;
	}
	return(FALSE);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// choose_default_object - called from menu, sets default values used by new object procedure
void choose_default_object(HWND hwnd)
{
	FARPROC lpProc;

	lpProc = MakeProcInstance(db_default_proc, appInstance);
	DialogBox(appInstance, "DIALOG_11", hwnd, lpProc);
	FreeProcInstance(lpProc);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Class property structure allocation functions
static void *alloc_nav_props(float init_freq)
{
	Nav_props *props;

	props = malloc(sizeof(Nav_props));
	props->freq = init_freq;

	return props;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void *alloc_plane_props(void)
{
	Plane_props *props;

	props = malloc(sizeof(Plane_props));

	strcpy(props->gauges	, "");
	strcpy(props->model		, "");
	strcpy(props->gear_down	, "");
	strcpy(props->supersonic	, "");

	props->rear_gun = FALSE;

	props->nord     = 7;
	props->ord[0].n = 250;	props->ord[0].kp =  1;
	props->ord[1].n =  32;	props->ord[1].kp =  5;
	props->ord[2].n =   0;	props->ord[2].kp =  5;
	props->ord[3].n =   0;	props->ord[3].kp =  5;
	props->ord[4].n =   0;	props->ord[4].kp = 10;
	props->ord[5].n =   0;	props->ord[5].kp =  2;
	props->ord[6].n =   0;	props->ord[6].kp = 20;

  	props->chaff    =  60;
	props->flare 	 =  60;

	return props;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void *alloc_fd_props(int class)
{
	FD_props *props;

	props = malloc(sizeof(FD_props));
	props->fixed = FALSE;

	switch (class) {
		case CLASS_AAGUN:
			props->data[0] =  2;
			props->data[1] = 20;
			props->data[2] = 10;
			break;

		case CLASS_SAM:
			props->data[0] =  50;
			props->data[1] =   5;
			props->data[2] = 100;
			break;

		default:
			props->data[0] = 0;
			props->data[1] = 0;
			props->data[2] = 0;
			break;
	}
	return props;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Depot proerties - shared by Depot and Hanger
static void *alloc_depot_props(void)
{
	Depot_props *props;

	props = malloc(sizeof(Depot_props));

	strcpy(props->vehicle_shape, "");
	strcpy(props->ac_model		, "");
	strcpy(props->ac_cockpit	, "");

	props->speed 				=  40;
	props->ai_model 			= CIVIL_MODEL;
	props->bombs 				=   0;
	props->initial 			=  99;
	props->interval 			= 180;
	props->size 				=   3;
	props->sub_interval 		=  30;
	props->vehicle_strength 		=  10;
	props->shells				=   0;
	props->fire_delay			=   5;
	props->flags				=   0;
	props->paths				= new_list();

	return props;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Functions using props structs
static void add_path_to_depot(Path *p)
{
	Depot_props *props;

	if (selected_object->type == CLASS_DEPOT || selected_object->type == CLASS_HANGER) {
		props = selected_object->props;
		if (!list_member(p, props->paths)) {
			list_add_last(p, props->paths);
		}
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void remove_path_from_depot(Path *p)
{
	Depot_props *props;

	if (selected_object->type == CLASS_DEPOT || selected_object->type == CLASS_HANGER) {
		props = selected_object->props;
		if (list_member(p, props->paths)) {
			list_remove(p, props->paths);
		}
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Object allocation/selection functions

// Allocates props memory for class
// called when creating new object, changing the class of object or loading objects
static void setup_object_class_data(Object_def *od)
{
	switch (od->type) {

		case CLASS_CULTURAL:
			od->props = NULL;
			break;

		case CLASS_ANIM:			// Added ANIM class
			od->props = NULL;
			break;

		case CLASS_VOR:
		case CLASS_ILS:
			od->props = alloc_nav_props(108.00);
			break;

		case CLASS_NDB:
			od->props = alloc_nav_props(500.00);
			break;

		case CLASS_BRICK:
		case CLASS_CIVIL:
		case CLASS_MILITARY:
			od->props = alloc_plane_props();
			break;

		case CLASS_AAGUN:
			od->props = alloc_fd_props(CLASS_AAGUN);
			break;

		case CLASS_SAM:
			od->props = alloc_fd_props(CLASS_SAM);
			break;

		case CLASS_DEPOT:
		case CLASS_HANGER:
			od->props = alloc_depot_props();
			break;

		default:
			od->props = NULL;
			break;
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void copy_props(Object_def *copy, Object_def *od)
{
//	Nav_props	*nav_ptr;
//	Ordnance 	*ord_ptr;
//	Plane_props *plane_ptr;
//	FD_props 	*fd_ptr;
	Depot_props	*depot_ptr;
	List			*copy_paths;

	switch (od->type) {

		case CLASS_CULTURAL:
			od->props = NULL;
			break;

		case CLASS_ANIM:			// Added ANIM class
			copy->props = NULL;
			break;

		case CLASS_VOR:
		case CLASS_ILS:
			copy->props = alloc_nav_props(108.00);
			*((Nav_props*)(copy->props)) = *((Nav_props*)(od->props));
			break;

		case CLASS_NDB:
			copy->props = alloc_nav_props(500.00);
			*((Nav_props*)(copy->props)) = *((Nav_props*)(od->props));
			break;

		case CLASS_BRICK:
		case CLASS_CIVIL:
		case CLASS_MILITARY:
			copy->props = alloc_plane_props();
			*((Plane_props*)(copy->props)) = *((Plane_props*)(od->props));
			break;

		case CLASS_AAGUN:
		case CLASS_SAM:
			copy->props = alloc_fd_props(NULL);
			*((FD_props*)(copy->props)) = *((FD_props*)(od->props));
			break;

		case CLASS_DEPOT:
		case CLASS_HANGER:
			depot_ptr = copy->props = alloc_depot_props();
			copy_paths = depot_ptr->paths;								// save pointer to empty list
			*((Depot_props*)(copy->props)) = *((Depot_props*)(od->props));		// copy structure contents
			copy_list(copy_paths, ((Depot_props*)(od->props))->paths);
			depot_ptr->paths = copy_paths;								// copy back list
			break;

		default:
			od->props = NULL;
			break;
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Sets the class of an object, allocates appropriate props data
static void find_object_class(Object_def *od, char *name)
{
	int old; //, i;

	old = od->type;
	od->type = get_class(name);
	if (od->type == NULL) {
		od->type = old;
		return;
	}
	else if (od->type == old) {
		return;
	}
	setup_object_class_data(od);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// select_object()
static long get_distance(long x1, long y1, long x2, long y2)
{
	long dx, dy, dist;

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
// EXTERN select_object, wx,wy are window coords, rc is rectangle to be redrawn, select_object loads shape for newly select object
void select_object(int wx, int wy, RECT *rc)
{
	long mx, my, dist, min_dist;
	List *l;
	Object_def *od, *last_selected;

	last_selected = selected_object;
	mx = get_mx(wx);
	my = get_my(wy);
	min_dist = 50000L;
	for (l = objects->next; l != objects; l = l->next) {
		od = l->item;
		dist = get_distance(mx, my, od->px, od->pz);
		if (dist < min_dist) {
			min_dist = dist;
			selected_object = od;
		}
	}
	if (selected_object != NULL) {
		load_object_shape(selected_object);
		get_object_rect(selected_object, last_selected, rc);
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void move_selected_object(int wx, int wy, RECT *rc)
{
	Object_def last_selected;
	RECT rect;

	if (selected_object != NULL) {
		last_selected = *selected_object;
		// ensure that relative height is correct
		selected_object->height = (int)selected_object->py - get_point_height(selected_object->px, selected_object->pz);
		selected_object->px = get_mx(wx);
		selected_object->pz = get_my(wy);
		// set abs. height
		selected_object->py  = get_point_height(selected_object->px, selected_object->pz) + selected_object->height;
		get_object_rect(selected_object, &last_selected, &rect);
		UnionRect(rc, &rect, rc);
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// EXTERN add_object - wx,wy window coords, used default_XXX to create object
void add_object(int wx, int wy)
{
	Object_def *od;

	if (strcmp(shape_name, "~NOSHAPE") == 0) {
		warning("No shape has been selected");
	}
	else {
		od = alloc_object();
		if (od) {
			od->px		= get_mx(wx);
			od->pz		= get_my(wy);
			od->py		= get_point_height(od->px, od->pz) + default_height;
			od->ry		= default_angle;
			od->height	= default_height;
			od->type		= default_class;
			od->strength	= default_strength;
			strcpy(od->name, shape_name);
			strcpy(od->dead_shape, dead_name);
			list_add(od, objects);
			selected_object = od;
			load_object_shape(selected_object);
		}
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// EXTERN copy_selected_object - wx,wy window coords, used default_XXX to create object
void copy_selected_object(void)
{
	if (selected_object) {
		object_copy = alloc_object();
		if (object_copy) {
			copy_object(object_copy, selected_object);
		}
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// EXTERN paste_object - wx,wy window coords, used object_copy to create object
void paste_object(int wx, int wy)
{
	Object_def *od;

	if (object_copy) {
		od = alloc_object();
		if (od) {
			copy_object(od, object_copy);
			od->px = get_mx(wx);
			od->pz = get_my(wy);
			// relative height has been correctly set in copy object
			od->py = get_point_height(od->px, od->pz) + od->height;
			od->height = default_height;
			strcpy(od->symbol, "nil");
			list_add(od, objects);
			selected_object = od;
		}
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Dialog procs modifying class props

static char type_title[32]    = "Extended Properties";
static char type_iv1_text[16] = "";
static Object_def *dialog_object;
static float freq_min = 000.00;
static float freq_max = 999.95;

/*
//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static BOOL FAR PASCAL db_dummy_properties_proc(HWND hdlg, unsigned msg, WORD wParam, LONG lParam)
{
	lParam = lParam;

	switch(msg) {

		case WM_INITDIALOG:
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
*/

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static BOOL FAR PASCAL db_nav_properties_proc(HWND hdlg, unsigned msg, WORD wParam, LONG lParam)
{
	char s[40];
	float f;
	Nav_props *p;

	lParam = lParam;

	switch(msg) {

		case WM_INITDIALOG:
			p = dialog_object->props;
			SetWindowText(hdlg, type_title);
			SetDlgItemText(hdlg, ID_DB8_TEXT, type_iv1_text);
			sprintf(s, "%05.2f", p->freq);				// was "%03.02f
			SetDlgItemText(hdlg, ID_DB_VALUE, s);
			return(TRUE);

		case WM_COMMAND:

			switch (wParam) {

				case IDOK:
					p = dialog_object->props;
					GetDlgItemText(hdlg, ID_DB_VALUE, s, 40);
					f = atof(s);
					if (f < freq_min || f > freq_max) {
						sprintf(s, "Freq. out of legal range (%5.2f - %5.2f)", freq_min, freq_max);
						MessageBox(NULL, s, type_iv1_text, MB_ICONEXCLAMATION);
						EndDialog(hdlg, FALSE);
					}
					else {
						p->freq = f;
						EndDialog(hdlg, TRUE);
					}
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
static BOOL FAR PASCAL db_brick_properties_proc(HWND hdlg, unsigned msg, WORD wParam, LONG lParam)
{
	Plane_props *p;

	lParam = lParam;

	switch(msg) {

		case WM_INITDIALOG:
			p = dialog_object->props;
			init_cb(hdlg, ID_DB_COCKPIT, p->gauges, "*.FGD", TRUE);
			return(TRUE);

		case WM_COMMAND:

			switch (wParam) {

				case IDOK:
					p = dialog_object->props;
					read_cb(hdlg, ID_DB_COCKPIT, p->gauges);
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
static BOOL FAR PASCAL db_plane_properties_proc(HWND hdlg, unsigned msg, WORD wParam, LONG lParam)
{
	int i;
	BOOL translated;
	Plane_props *p;

	lParam = lParam;

	switch(msg) {

		case WM_INITDIALOG:
			p = dialog_object->props;
			init_cb(hdlg, ID_DB_COCKPIT	, p->gauges	, "*.FGD", TRUE);
			init_cb(hdlg, ID_DB_MODEL	, p->model	, "*.FMD", TRUE);
			init_cb(hdlg, ID_DB_GEAR_SHAPE, p->gear_down	, "*.FSD", TRUE);
			if (dialog_object->type == CLASS_MILITARY) {
				init_cb(hdlg, ID_DB_DMG_SHAPE	, p->damaged   , "*.FSD", TRUE);
				init_cb(hdlg, ID_DB_SUPER_SHAPE, p->supersonic, "*.FSD", TRUE);
				CheckDlgButton(hdlg, ID_DB14_RGUN, p->rear_gun);
				for (i = 0;  i < MAX_ORD; i++) {
					SetDlgItemInt(hdlg, ID_DB_ORD_N+i, p->ord[i].n, TRUE);
					SetDlgItemInt(hdlg, ID_DB_ORD_KP+i, p->ord[i].kp, TRUE);
				}
				SetDlgItemInt(hdlg, ID_DB5_CHAFF, p->chaff, TRUE);
				SetDlgItemInt(hdlg, ID_DB5_FLARE, p->flare, TRUE);
			}
			return(TRUE);

		case WM_COMMAND:

			switch (wParam) {

				case IDOK:
					p = dialog_object->props;
					read_cb(hdlg, ID_DB_COCKPIT	, p->gauges);
					read_cb(hdlg, ID_DB_MODEL	, p->model);
					read_cb(hdlg, ID_DB_GEAR_SHAPE, p->gear_down);
					if (dialog_object->type == CLASS_MILITARY) {
						read_cb(hdlg, ID_DB_DMG_SHAPE	, p->damaged);
						read_cb(hdlg, ID_DB_SUPER_SHAPE, p->supersonic);
						p->rear_gun = IsDlgButtonChecked(hdlg, ID_DB14_RGUN);
						EndDialog(hdlg, TRUE);
						for (i = 0;  i < MAX_ORD; i++) {
							p->ord[i].n  = GetDlgItemInt(hdlg, ID_DB_ORD_N+i, &translated, TRUE);
							p->ord[i].kp = GetDlgItemInt(hdlg, ID_DB_ORD_KP+i, &translated, TRUE);
						}
						p->chaff = GetDlgItemInt(hdlg, ID_DB5_CHAFF, &translated, TRUE);
						p->flare = GetDlgItemInt(hdlg, ID_DB5_FLARE, &translated, TRUE);
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
//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static BOOL FAR PASCAL db_track_proc(HWND hdlg, unsigned msg, WORD wParam, LONG lParam)
{
 //	char s[40];

	lParam = lParam;

	switch(msg) {

		case WM_INITDIALOG:
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
*/

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static BOOL FAR PASCAL db_fd_properties_proc(HWND hdlg, unsigned msg, WORD wParam, LONG lParam)
{
	BOOL t;
	FD_props *p;

	lParam = lParam;

	switch(msg) {

		case WM_INITDIALOG:
			p = dialog_object->props;
			if (dialog_object->type == CLASS_AAGUN) {
				CheckDlgButton(hdlg, ID_DB17_FIXED, p->fixed);
			}
			SetDlgItemInt(hdlg, ID_DB_VALUE,  p->data[0], TRUE);
			SetDlgItemInt(hdlg, ID_DB_VALUE1, p->data[1], TRUE);
			SetDlgItemInt(hdlg, ID_DB_VALUE2, p->data[2], TRUE);
			return(TRUE);

		case WM_COMMAND:

			switch (wParam) {

				case IDOK:
					p = dialog_object->props;
					if (dialog_object->type == CLASS_AAGUN) {
						p->fixed = IsDlgButtonChecked(hdlg, ID_DB17_FIXED);
					}
					p->data[0] = GetDlgItemInt(hdlg, ID_DB_VALUE,  &t, TRUE);
					p->data[1] = GetDlgItemInt(hdlg, ID_DB_VALUE1, &t, TRUE);
					p->data[2] = GetDlgItemInt(hdlg, ID_DB_VALUE2, &t, TRUE);
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
static BOOL FAR PASCAL db_depot_properties_proc(HWND hdlg, unsigned msg, WORD wParam, LONG lParam)
{
	BOOL			t;
	int			item;
	Depot_props	*dp;
	List			*l;
	Path			*path;

	lParam = lParam;

	switch(msg) {

		case WM_INITDIALOG:
			 dp = dialog_object->props;
			// Paths
			Map (l, dp->paths) {
				path = l->item;
				SendDlgItemMessage(hdlg, ID_DB13_PATHS, LB_ADDSTRING, 0, (LPARAM)((LPSTR)path->name));
			}
			init_cb(hdlg, ID_DB13_SHAPE, dp->vehicle_shape, "*.FSD", TRUE);

			SetDlgItemInt(hdlg	, ID_DB_VALUE		, dp->initial			, TRUE);
			SetDlgItemInt(hdlg	, ID_DB_VALUE1		, dp->interval			, TRUE);
			SetDlgItemInt(hdlg	, ID_DB_VALUE2		, dp->size			, TRUE);
			SetDlgItemInt(hdlg	, ID_DB_VALUE3		, dp->sub_interval		, TRUE);
			SetDlgItemInt(hdlg	, ID_DB_SPEED		, dp->speed			, TRUE);
			SetDlgItemInt(hdlg	, ID_DB_STRENGTH	, dp->vehicle_strength	, TRUE);
			SetDlgItemInt(hdlg	, ID_DB_SHELLS		, dp->shells			, TRUE);
			SetDlgItemInt(hdlg	, ID_DB_GAP		, dp->fire_delay		, TRUE);

			CheckDlgButton(hdlg	, ID_DB_PREDICT	, dp->flags & PREDICT);
			CheckDlgButton(hdlg	, ID_DB_AA		, dp->flags & AAGUN);
			return(TRUE);

		case WM_COMMAND:

			switch (wParam) {

				case ID_DB13_PATHS:
					return (TRUE);

				case IDOK:
					dp = dialog_object->props;
					read_cb(hdlg, ID_DB13_SHAPE, dp->vehicle_shape);
					dp->initial 			= GetDlgItemInt(hdlg, ID_DB_VALUE	, &t, TRUE);
					dp->interval 			= GetDlgItemInt(hdlg, ID_DB_VALUE1	, &t, TRUE);
					dp->size 				= GetDlgItemInt(hdlg, ID_DB_VALUE2	, &t, TRUE);
					dp->sub_interval 		= GetDlgItemInt(hdlg, ID_DB_VALUE3	, &t, TRUE);
					dp->speed 			= GetDlgItemInt(hdlg, ID_DB_SPEED	, &t, TRUE);
					dp->vehicle_strength	= GetDlgItemInt(hdlg, ID_DB_STRENGTH, &t, TRUE);
					dp->shells			= GetDlgItemInt(hdlg, ID_DB_SHELLS	, &t, TRUE);
					dp->fire_delay			= GetDlgItemInt(hdlg, ID_DB_GAP	, &t, TRUE);
					if (IsDlgButtonChecked(hdlg, ID_DB_PREDICT)) {
						dp->flags |= PREDICT;
					}
					else {
						dp->flags &= ~PREDICT;
					}
					if (IsDlgButtonChecked(hdlg, ID_DB_AA)) {
						dp->flags |= AAGUN;
					}
					else {
						dp->flags &= ~AAGUN;
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

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static BOOL FAR PASCAL db_hanger_properties_proc(HWND hdlg, unsigned msg, WORD wParam, LONG lParam)
{
	static int ai;
	BOOL t;
	Depot_props *dp;
	List *l;
	Path *path;

	lParam = lParam;

	switch(msg) {

		case WM_INITDIALOG:
			dp = dialog_object->props;
			// Paths
			Map (l, dp->paths) {
				path = l->item;
				SendDlgItemMessage(hdlg, ID_DB13_PATHS, LB_ADDSTRING, 0, (LPARAM)((LPSTR)path->name));
			}
			init_cb(hdlg, ID_DB6_DSHAPE, dp->vehicle_shape, "*.FSD", TRUE);
			init_cb(hdlg, ID_DB_MODEL, dp->ac_model, "*.FMD", TRUE);
			SetDlgItemInt(hdlg, ID_DB_VALUE, dp->initial, TRUE);
			ai = dp->ai_model;
			CheckDlgButton(hdlg, ID_DB_AI1, (ai == FIGHTER_MODEL));
			CheckDlgButton(hdlg, ID_DB_AI2, (ai == CIVIL_MODEL));
			CheckDlgButton(hdlg, ID_DB_AI3, (ai == BOMBER_MODEL));
			CheckDlgButton(hdlg, ID_DB_AI4, (ai == STRAFER_MODEL));
			CheckDlgButton(hdlg, ID_DB19_RGUN, dp->flags & REAR_GUN);
			SetDlgItemInt(hdlg, ID_DB19_STRENGTH, dp->vehicle_strength, TRUE);
			SetDlgItemInt(hdlg, ID_DB19_BOMBS, dp->bombs, TRUE);
			return(TRUE);

		case WM_COMMAND:

			switch (wParam) {

				case ID_DB_AI1:
					ai = FIGHTER_MODEL;
					CheckDlgButton(hdlg, ID_DB_AI1, (ai == FIGHTER_MODEL));
					CheckDlgButton(hdlg, ID_DB_AI2, (ai == CIVIL_MODEL));
					CheckDlgButton(hdlg, ID_DB_AI3, (ai == BOMBER_MODEL));
					CheckDlgButton(hdlg, ID_DB_AI4, (ai == STRAFER_MODEL));
					break;

				case ID_DB_AI2:
					ai = CIVIL_MODEL;
					CheckDlgButton(hdlg, ID_DB_AI1, (ai == FIGHTER_MODEL));
					CheckDlgButton(hdlg, ID_DB_AI2, (ai == CIVIL_MODEL));
					CheckDlgButton(hdlg, ID_DB_AI3, (ai == BOMBER_MODEL));
					CheckDlgButton(hdlg, ID_DB_AI4, (ai == STRAFER_MODEL));
					break;

				case ID_DB_AI3:
					ai = BOMBER_MODEL;
					CheckDlgButton(hdlg, ID_DB_AI1, (ai == FIGHTER_MODEL));
					CheckDlgButton(hdlg, ID_DB_AI2, (ai == CIVIL_MODEL));
					CheckDlgButton(hdlg, ID_DB_AI3, (ai == BOMBER_MODEL));
					CheckDlgButton(hdlg, ID_DB_AI4, (ai == STRAFER_MODEL));
					break;

				case ID_DB_AI4:
					ai = STRAFER_MODEL;
					CheckDlgButton(hdlg, ID_DB_AI1, (ai == FIGHTER_MODEL));
					CheckDlgButton(hdlg, ID_DB_AI2, (ai == CIVIL_MODEL));
					CheckDlgButton(hdlg, ID_DB_AI3, (ai == BOMBER_MODEL));
					CheckDlgButton(hdlg, ID_DB_AI4, (ai == STRAFER_MODEL));
					break;

				case IDOK:
					dp = dialog_object->props;
					read_cb(hdlg, ID_DB6_DSHAPE, dp->vehicle_shape);
					read_cb(hdlg, ID_DB_MODEL, dp->ac_model);
					read_cb(hdlg, ID_DB_COCKPIT, dp->ac_cockpit);
					dp->initial 	= GetDlgItemInt(hdlg, ID_DB_VALUE, &t, TRUE);
					dp->ai_model 	= ai;
					if (IsDlgButtonChecked(hdlg, ID_DB19_RGUN)) {
						dp->flags |= REAR_GUN;
					}
					else {
						dp->flags &= ~REAR_GUN;
					}
					dp->vehicle_strength 	= GetDlgItemInt(hdlg, ID_DB19_STRENGTH, &t, TRUE);
					dp->bombs 	= GetDlgItemInt(hdlg, ID_DB19_BOMBS, &t, TRUE);
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
// set_XXX - functions called from basic props dialog box by properties button
void set_ndb_class_properties(HWND hdlg)
{
	FARPROC lpProc;

	strcpy(type_iv1_text, "NDB Freq");
	if (strcmp(dialog_object->symbol, "nil") != 0) {
		sprintf(type_title, "%s NDB", dialog_object->symbol);
	}
	else {
		strcpy(type_title, "NDB");
	}
	freq_min = 0.0;
	freq_max = 999.95;
	lpProc = MakeProcInstance(db_nav_properties_proc, appInstance);
	DialogBox(appInstance, "DIALOG_8", hdlg, lpProc);
	FreeProcInstance(lpProc);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void set_vor_class_properties(HWND hdlg)
{
	FARPROC lpProc;

	strcpy(type_iv1_text, "VOR Freq");
	if (strcmp(dialog_object->symbol, "nil") != 0) {
		sprintf(type_title, "%s VOR", dialog_object->symbol);
	}
	else {
		strcpy(type_title, "VOR");
	}
	freq_min = 108.00;
	freq_max = 117.95;
	lpProc = MakeProcInstance(db_nav_properties_proc, appInstance);
	DialogBox(appInstance, "DIALOG_8", hdlg, lpProc);
	FreeProcInstance(lpProc);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void set_ils_class_properties(HWND hdlg)
{
	FARPROC lpProc;

	strcpy(type_iv1_text, "ILS Freq");
	if (strcmp(dialog_object->symbol, "nil") != 0) {
		sprintf(type_title, "%s ILS", dialog_object->symbol);
	}
	else {
		strcpy(type_title, "ILS");
	}
	freq_min = 108.00;
	freq_max = 117.95;
	lpProc = MakeProcInstance(db_nav_properties_proc, appInstance);
	DialogBox(appInstance, "DIALOG_8", hdlg, lpProc);
	FreeProcInstance(lpProc);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void set_aagun_class_properties(HWND hdlg)
{
	FARPROC lpProc;

	lpProc = MakeProcInstance(db_fd_properties_proc, appInstance);
	DialogBox(appInstance, "DIALOG_17", hdlg, lpProc);
	FreeProcInstance(lpProc);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void set_sam_class_properties(HWND hdlg)
{
	FARPROC lpProc;

	lpProc = MakeProcInstance(db_fd_properties_proc, appInstance);
	DialogBox(appInstance, "DIALOG_18", hdlg, lpProc);
	FreeProcInstance(lpProc);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void set_depot_class_properties(HWND hdlg)
{
	FARPROC lpProc;

	lpProc = MakeProcInstance(db_depot_properties_proc, appInstance);
	DialogBox(appInstance, "DIALOG_13", hdlg, lpProc);
	FreeProcInstance(lpProc);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void set_hanger_class_properties(HWND hdlg)
{
	FARPROC lpProc;

	lpProc = MakeProcInstance(db_hanger_properties_proc, appInstance);
	DialogBox(appInstance, "DIALOG_19", hdlg, lpProc);
	FreeProcInstance(lpProc);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void set_brick_class_properties(HWND hdlg)
{
	FARPROC lpProc;

	lpProc = MakeProcInstance(db_brick_properties_proc, appInstance);
	DialogBox(appInstance, "DIALOG_15", hdlg, lpProc);
	FreeProcInstance(lpProc);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void set_plane_class_properties(HWND hdlg)
{
	FARPROC lpProc;

	lpProc = MakeProcInstance(db_plane_properties_proc, appInstance);
	DialogBox(appInstance, "DIALOG_12", hdlg, lpProc);
	FreeProcInstance(lpProc);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void set_military_class_properties(HWND hdlg)
{
	FARPROC lpProc;

	lpProc = MakeProcInstance(db_plane_properties_proc, appInstance);
	DialogBox(appInstance, "DIALOG_14", hdlg, lpProc);
	FreeProcInstance(lpProc);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void set_class_properties(HWND hdlg)
{
	switch (dialog_object->type) {

		case CLASS_BRICK:
			set_brick_class_properties(hdlg);
			break;

		case CLASS_CIVIL:
			set_plane_class_properties(hdlg);
			break;

		case CLASS_MILITARY:
			set_military_class_properties(hdlg);
			break;

		case CLASS_NDB:
			set_ndb_class_properties(hdlg);
			break;

		case CLASS_VOR:
			set_vor_class_properties(hdlg);
			break;

		case CLASS_ILS:
			set_ils_class_properties(hdlg);
			break;

		case CLASS_AAGUN:
			set_aagun_class_properties(hdlg);
			break;

		case CLASS_SAM:
			set_sam_class_properties(hdlg);
			break;

		case CLASS_DEPOT:
			set_depot_class_properties(hdlg);
			break;

		case CLASS_HANGER:
			set_hanger_class_properties(hdlg);
			break;

		default:
			break;
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Basic properties dialog box
static BOOL FAR PASCAL db_properties_proc(HWND hdlg, unsigned msg, WORD wParam, LONG lParam)
{
	BOOL t;
	int  i, str;
	char s[40], symbolic_name[16];
	Object_def *od; // tmp var used to check if other object exist with name

	lParam = lParam;

	switch(msg) {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case WM_INITDIALOG:
			// Class name
			for (i = 0; i < MAX_CLASS; i++) {
				if (class_names[i] != NULL) {
					SendDlgItemMessage(hdlg, ID_DB6_CLASS, CB_ADDSTRING, 0, (LPARAM)((LPSTR)class_names[i]));
				}
			}
			SendDlgItemMessage(hdlg, ID_DB6_CLASS, CB_SELECTSTRING, -1, (LPARAM)class_name(dialog_object, FALSE));

			// Main shape
			init_cb(hdlg, ID_DB6_SHAPE, dialog_object->name, "*.FSD", TRUE);

			// Symbolic object name
			SetDlgItemText(hdlg, ID_DB6_NAME, dialog_object->symbol);

			// rotation
			SetDlgItemInt(hdlg, ID_DB6_HEADING, dialog_object->ry, TRUE);

			// relative height above ground
			SetDlgItemInt(hdlg, ID_DB6_HEIGHT, dialog_object->height, TRUE);

			// X & Z location
			sprintf(s, "%ld", dialog_object->px);
			SetDlgItemText(hdlg, ID_DB6_XPOS, s);
			sprintf(s, "%ld", dialog_object->pz);
			SetDlgItemText(hdlg, ID_DB6_ZPOS, s);

			// Object strength
			SetDlgItemInt(hdlg, ID_DB6_STRENGTH, dialog_object->strength, TRUE);

			// Dead shape
			init_cb(hdlg, ID_DB6_DSHAPE, dialog_object->dead_shape, "*.FSD", TRUE);

			// Flags
			CheckDlgButton(hdlg, ID_DB6_PLAYER, (dialog_object == player));
			CheckDlgButton(hdlg, ID_DB6_LANDON, (dialog_object->flags & LAND_ON));

			// Team
			CheckDlgButton(hdlg, ID_DB6_BLUE,   (dialog_object->flags & BLUE_TARGET));
			CheckDlgButton(hdlg, ID_DB6_NEUTRAL,!(dialog_object->flags & BLUE_TARGET) && !(dialog_object->flags & TARGET));
			CheckDlgButton(hdlg, ID_DB6_TGT,    (dialog_object->flags & TARGET));

			EnableWindow(GetDlgItem(hdlg, ID_DB6_DSHAPE), (dialog_object->strength > 0));
			EnableWindow(GetDlgItem(hdlg, ID_DB6_PROPS), (dialog_object->type != CLASS_CULTURAL && dialog_object->type != CLASS_ANIM));
			return(TRUE);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case WM_COMMAND:

			switch (wParam) {

				case ID_DB6_CLASS:
					GetDlgItemText(hdlg, ID_DB6_CLASS, s, 16);
					find_object_class(dialog_object, s);
					EnableWindow(GetDlgItem(hdlg, ID_DB6_PROPS), (dialog_object->type != CLASS_CULTURAL && dialog_object->type != CLASS_ANIM));
				break;

				case ID_DB6_LANDON:
					if (dialog_object->flags & LAND_ON) {
						dialog_object->flags &= ~LAND_ON;
					}
					else {
						dialog_object->flags |= LAND_ON;
						dialog_object->strength = 0;
						SetDlgItemInt(hdlg, ID_DB6_STRENGTH, dialog_object->strength, TRUE);
					}
					CheckDlgButton(hdlg, ID_DB6_LANDON, (dialog_object->flags & LAND_ON));
					break;

				case ID_DB6_BLUE:
					if (dialog_object->flags & BLUE_TARGET) {
						dialog_object->flags &= ~BLUE_TARGET;
					}
					else {
						dialog_object->flags |= BLUE_TARGET;
						dialog_object->flags &= ~TARGET;
					}
					CheckDlgButton(hdlg, ID_DB6_TGT    , (dialog_object->flags & TARGET));
					CheckDlgButton(hdlg, ID_DB6_NEUTRAL, 0);
					CheckDlgButton(hdlg, ID_DB6_BLUE   , (dialog_object->flags & BLUE_TARGET));
					break;

				case ID_DB6_NEUTRAL:
					if (dialog_object->flags & BLUE_TARGET) {
						dialog_object->flags &= ~BLUE_TARGET;
					}
					if (dialog_object->flags & TARGET) {
						dialog_object->flags &= ~TARGET;
					}
					CheckDlgButton(hdlg, ID_DB6_TGT, (dialog_object->flags & TARGET));
					CheckDlgButton(hdlg, ID_DB6_NEUTRAL, 1);
					CheckDlgButton(hdlg, ID_DB6_BLUE, (dialog_object->flags & BLUE_TARGET));
					break;


				case ID_DB6_TGT:
					if (dialog_object->flags & TARGET) {
						dialog_object->flags &= ~TARGET;
					}
					else {
						dialog_object->flags |= TARGET;
						dialog_object->flags &= ~BLUE_TARGET;
					}
					CheckDlgButton(hdlg, ID_DB6_BLUE, (dialog_object->flags & BLUE_TARGET));
					CheckDlgButton(hdlg, ID_DB6_NEUTRAL, 0);
					CheckDlgButton(hdlg, ID_DB6_TGT, (dialog_object->flags & TARGET));
					break;

				case ID_DB6_STRENGTH:
					str = GetDlgItemInt(hdlg, ID_DB6_STRENGTH, &t, TRUE);
					if (t) {
						if (str == 0) {
							SendDlgItemMessage(hdlg, ID_DB6_DSHAPE, CB_SETCURSEL, -1, 0L);
						}
						else {
							SendDlgItemMessage(hdlg, ID_DB6_DSHAPE, CB_SELECTSTRING, -1, (LPARAM)dialog_object->name);
						}
						EnableWindow(GetDlgItem(hdlg, ID_DB6_DSHAPE), (str > 0));
					}
					break;

				case ID_DB6_PROPS:
					set_class_properties(hdlg);
					break;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
				case IDOK:
					GetDlgItemText(hdlg, ID_DB6_CLASS, s, 16);
					find_object_class(dialog_object, s);

					read_cb(hdlg, ID_DB6_SHAPE	, dialog_object->name);
					change_shape_name(dialog_object, s);

					GetDlgItemText(hdlg, ID_DB6_NAME, symbolic_name, 16);
					od = get_object_symbolic(symbolic_name);
					if (od && (od != dialog_object)) {
						warning("Object with this name already exists!");
					}
					else {
						strcpy(dialog_object->symbol, symbolic_name);
					}

					GetDlgItemText(hdlg, ID_DB6_HEADING, s, 4);
					dialog_object->ry = atoi(s);
					if (dialog_object->ry > 360) {
						warning("Angle must be between 0° and 360°!");
					}
					else {
						dialog_object->ry = dialog_object->ry;
					}

					GetDlgItemText(hdlg, ID_DB6_HEIGHT, s, 5);
					dialog_object->height = atoi(s);
					// py is abs height above ground, set
					dialog_object->py = get_point_height(dialog_object->px, dialog_object->pz) + dialog_object->height;

					// object X coordinate
					GetDlgItemText(hdlg, ID_DB6_XPOS, s, 10);
					dialog_object->px = atol(s);

					// object Y coordinate
					GetDlgItemText(hdlg, ID_DB6_ZPOS, s, 10);
					dialog_object->pz = atol(s);

					GetDlgItemText(hdlg, ID_DB6_STRENGTH, s, 6);
					dialog_object->strength = atoi(s);

					read_cb(hdlg, ID_DB6_DSHAPE	, dialog_object->dead_shape);

					if (IsDlgButtonChecked(hdlg, ID_DB6_PLAYER)) {
						// unset old player flags
						if (player != NULL) {
							player->flags &= ~PLAYER;
							strcpy(player->symbol, "nil");
						}
						// set player variable and set flags
						player = dialog_object;
						player->flags |= PLAYER;
						strcpy(player->symbol, "Player");
					}
					EndDialog(hdlg, TRUE);
					return(TRUE);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
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
// EXTERN set_selected_properties, open basic proerties dialog box for selected object
void set_selected_properties(RECT *rc)
{
	FARPROC lpProc;
	Object_def saved_od;
	RECT dummy = {0, 0, 0, 0};

	// if called from menu rc is not supplied
	if (rc == NULL) {
		rc = &dummy;
	}
	if (selected_object != NULL) {
		// find relative height above terrain
		selected_object->height = (int)selected_object->py - get_point_height(selected_object->px, selected_object->pz);
		copy_object(&saved_od, selected_object);
		
		dialog_object = selected_object;
		lpProc = MakeProcInstance(db_properties_proc, appInstance);
		if (!DialogBox(appInstance, "DIALOG_6", main_window, lpProc)) {
			copy_object(selected_object, &saved_od);
		}
		FreeProcInstance(lpProc);
		get_object_rect(selected_object, selected_object, rc);
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Draw functions - draw dashed line from object to start of linked paths
static void draw_links_to_paths(HDC hdc, Object_def *od, int x, int y)
{
	HPEN old_pen;
	List *l;
	Depot_props *dp;
	Path *path;
	Posn *p;
	int px, py;

	dp = od->props;
	for (l = dp->paths->next; l != dp->paths; l = l->next) {
		path = l->item;
		p = Car(path->posns);

		if ((show_paths) || (!show_paths && show_selpath && (selected_path == path))) {
			if (p) {
				old_pen = SelectObject(hdc, dot_pen);
				MoveTo(hdc, x, y);
				px = get_wx_mx(p->x);
				py = get_wy_my(p->z);
				LineTo(hdc, px, py);
				SelectObject(hdc, old_pen);
			}
		}
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void set_team_color(HDC hdc, int tf, int selected)
{
	if (show_teams) {
		SelectObject(hdc, gry_pen);
		if (selected) {
			SelectObject(hdc, pur_pen);
			return;
		}
		if (tf == 1) {
			SelectObject(hdc, blu_pen);
			return;
		}
		if (tf == 2) {
			SelectObject(hdc, red_pen);
			return;
		}
	}
	else {
		SelectObject(hdc, white_pen);
		if (selected) {
			SelectObject(hdc, red_pen);
			return;
		}
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// EXTERN draw_objects - draw all objects in region rc
void draw_objects(HDC hdc, RECT *rc)
{
	int x, y, tf;	//, rng;
	List *l;
	Object_def *od;
	HPEN old_pen;
	//FD_props *p;

	old_pen = SelectObject(hdc, white_pen);

	for (l = objects->next; l != objects; l = l->next) {
		od = l->item;
		x = get_wx_mx(od->px);
		if ((x >= rc->left) && (x <= rc->right)) {
			y = get_wy_my(od->pz);
			if ((y >= rc->top) && (y <= rc->bottom)) {

				// All objects
				if (show_objects) {
					tf = 0;
					if (od->flags & BLUE_TARGET) {
						tf = 1;
					}
					else if (od->flags & TARGET) {
						tf = 2;
					}
					set_team_color(hdc, tf, FALSE);

					if (od == selected_object) {
						set_team_color(hdc, tf, TRUE);
						draw_current_shape(hdc, x, y, od->ry, get_xsize(500), get_ysize(500));
					}
					else if (od->hshape) {
						set_team_color(hdc, tf, FALSE);
						draw_shape(hdc, od->hshape, x, y, od->ry, get_xsize(500), get_ysize(500));
					}
					MoveTo(hdc, x - box_size, y - box_size);
					LineTo(hdc, x + box_size, y - box_size);
					LineTo(hdc, x + box_size, y + box_size);
					LineTo(hdc, x - box_size, y + box_size);
					LineTo(hdc, x - box_size, y - box_size);
				}
				// Depots & Hanger
				if (od->type == CLASS_DEPOT || od->type == CLASS_HANGER) {
					draw_links_to_paths(hdc, od, x, y);
				}
				// SAM Range rings
				if (od->type == CLASS_SAM) {
				//	p = od->props;
				//	rng = p->data[0];
				//	SelectObject(hdc, red_pen);
				//	Arc(hdc, x - rng, y - rng, x + rng, y + rng, x, y - rng, x, y - rng);
				}
			}
		}
	}
	SelectObject(hdc, old_pen);

	draw_paths(hdc);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// SAVE/LOAD functions - sl_string and sl_int save/load respective data types
// to/from file (dependant on save flag value), sl_XXX - save/load functions for object properties
static void replace_spaces_with_underbar(char *str)
{
	char *ptr;

	for (ptr = str; *ptr != NULL; ptr += 1) {
		if (*ptr == ' ') {
			*ptr = '_';
		}
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void sl_string(FILE *fp, BOOL save, char *str)
{
	if (save) {
		if (strlen(str) > 0) {
			replace_spaces_with_underbar(str);
			fprintf(fp, "%s\n", str);
		}
		else {
			fprintf(fp, "nil\n");
		}
	}
	else {
		fscanf(fp, "%s", str);
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void sl_int(FILE *fp, BOOL save, int *i)
{
	if (save) {
		fprintf(fp, " %d ", *i);
	}
	else {
		fscanf(fp, "%d", i);
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void sl_float(FILE *fp, BOOL save, float *f)
{
	if (save) {
		fprintf(fp, " %5.2f ", *f);
	}
	else {
		fscanf(fp, "%f", f);
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Object properties:
static void sl_brick_props(FILE *fp, BOOL save, Plane_props *p)
{
	sl_string(fp, save, p->gauges);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Military (if ordnance is TRUE), Civil
static void sl_plane_props(FILE *fp, BOOL save, Plane_props *p, BOOL ordnance)
{
	int i;

	sl_string(fp, save, p->gauges);
	sl_string(fp, save, p->model);
	sl_string(fp, save, p->gear_down);
	if (world_file_version >= 5) {
		sl_string(fp, save, p->damaged);
	}

	if (ordnance) {
		if (save) {
			sl_string(fp, save, p->supersonic);
			sl_int(fp, save, &p->rear_gun);

			for (i = 0; i < MAX_ORD; i++) {
				sl_int(fp, save, &p->ord[i].n);
				sl_int(fp, save, &p->ord[i].kp);
			}
			if (world_file_version >= 5) {
				fprintf(fp, "\n");
				sl_int(fp, save, &p->chaff);
				sl_int(fp, save, &p->flare);
			}
		}
		else {
			sl_string(fp, save, p->supersonic);

			if (world_file_version >= 3) {
				sl_int(fp, save, &p->rear_gun);
			}
			for (i = 0; i < max_ord; i++) {
				sl_int(fp, save, &p->ord[i].n);
				sl_int(fp, save, &p->ord[i].kp);
			}
			if (world_file_version >= 5) {
				sl_int(fp, save, &p->chaff);
				sl_int(fp, save, &p->flare);
			}
		}
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// NDB, VOR, ILS
static void sl_nav_props(FILE *fp, BOOL save, Nav_props *p)
{
	sl_float(fp, save, &p->freq);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// AA Gun, SAM
static void sl_fd_props(FILE *fp, BOOL save, FD_props *p)
{
	int i;

	if (world_file_version >= 3) {
		sl_int(fp, save, &p->fixed);
	}
	for (i = 0; i < MAX_FD; i++) {
		sl_int(fp, save, &p->data[i]);
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Used by sl_depot and sl_hanger - save/load symbolic references to paths linked to object
static void sl_paths(FILE *fp, BOOL save, List *paths)
{
	int n;
	List *l;
	char path_name[32];
	Path *path;

	if (save) {
		// paths Depot and Hanger
		n = list_length(paths);
		fprintf(fp, "%d\n", n);
		for (l = paths->next; l != paths; l = l->next) {
			path = l->item;
			sl_string(fp, save, path->name);
		}
	}
	else {
		sl_int(fp, save, &n);
		while (n--) {
			sl_string(fp, save, path_name);
			path = get_path(path_name);
			if (path) {
				list_add_last(path, paths);
			}
		}
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void sl_depot_props(FILE *fp, BOOL save, Depot_props *p)
{
  //	int n;

	sl_string(fp, save, p->vehicle_shape);		// Depot and Hanger
	sl_int(fp, save, &p->initial);
	sl_int(fp, save, &p->speed);
	sl_int(fp, save, &p->interval);
	sl_int(fp, save, &p->size);
	sl_int(fp, save, &p->sub_interval);
	if (world_file_version >= 4) {
		sl_int(fp, save, &p->flags);
		sl_int(fp, save, &p->vehicle_strength);
		sl_int(fp, save, &p->shells);
		sl_int(fp, save, &p->fire_delay);
	}
	sl_paths(fp, save, p->paths);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void sl_hanger_props(FILE *fp, BOOL save, Depot_props *p)
{
	sl_string(fp, save, p->vehicle_shape);	// Depot and Hanger
	sl_string(fp, save, p->ac_model);
	sl_string(fp, save, p->ac_cockpit);
	sl_int(fp, save, &p->initial);
	sl_int(fp, save, &p->ai_model);
	if (world_file_version >= 3) {
		sl_int(fp, save, &p->flags);
		sl_int(fp, save, &p->vehicle_strength);
		sl_int(fp, save, &p->bombs);
	}
	sl_paths(fp, save, p->paths);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void load_object_class_data(FILE *fp, Object_def *od)
{
	//char str[32];

	switch (od->type) {

		case CLASS_BRICK:
			sl_brick_props(fp, FALSE, od->props);
			break;

		case CLASS_CIVIL:
			sl_plane_props(fp, FALSE, od->props, FALSE);
			break;

		case CLASS_MILITARY:
			sl_plane_props(fp, FALSE, od->props, TRUE);
			break;

		case CLASS_NDB:
		case CLASS_VOR:
		case CLASS_ILS:
			sl_nav_props(fp, FALSE, od->props);
			break;

		case CLASS_AAGUN:
		case CLASS_SAM:
			sl_fd_props(fp, FALSE, od->props);
			break;

		case CLASS_DEPOT:
			sl_depot_props(fp, FALSE, od->props);
			break;

		case CLASS_HANGER:
			sl_hanger_props(fp, FALSE, od->props);
			break;

		default:
			break;
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Returns TRUE is object loaded correctly, FALSE if EOF encountered or Paths string
BOOL load_object(FILE *fp)
{
  //	long px, py;
	Object_def *od;
	char type[40]; //, name[40];
	int  c, scanrtn;

	c = 0;
	while (c == 0) {
		scanrtn = fscanf(fp, "%s", type);
		if (scanrtn == 0) {
			return FALSE;
		}
		c = get_class(type);
		if (c == 0) {
			if (strcmp(type, "Paths") == 0) {
				paths_string_loaded = TRUE;
				return FALSE;
			}
			else {
				// Assume object is an extra class so treat as Cultural
				c = CLASS_CULTURAL;
			}
		}
	}
	// c is class of object
	od = malloc(sizeof(Object_def));
	od->type = c;
	strcpy(od->extra_name, type);
	od->hshape = NULL;
	if (od == NULL) {
		stop("No memory for new object");
	}
	fscanf(fp, "%s\n", od->symbol);
	fscanf(fp, "%ld %ld %ld %d %d %s\n", &od->px, &od->py, &od->pz, &od->ry, &od->flags, od->name);
	fscanf(fp, "%d", &od->strength);

	if (world_file_version < 5) {
		od->ry = 360 - od->ry;
	}

	sl_string(fp, FALSE, od->dead_shape);

	if (od->flags & PLAYER) {
		player = od;
	}
	setup_object_class_data(od);
	load_object_class_data(fp, od);
	list_add(od, objects);
	return TRUE;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void load_objects(FILE *fp)
{
	int  n;
	char comment[40];

	fscanf(fp, "%s %d\n", comment, &n);
	if (strcmp(comment, "Objects") != 0) {
		stop("Error when reading Objects string");
	}
	paths_string_loaded = FALSE;
	while (load_object(fp) && (n > 0)) {
		n--;
	}
	load_paths(fp);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void save_object_class_data(FILE *fp, Object_def *od)
{
	BOOL save;

	save = TRUE;

	switch (od->type) {

		case CLASS_BRICK:
			sl_brick_props(fp, save, od->props);
			break;

		case CLASS_CIVIL://
			sl_plane_props(fp, save, od->props, FALSE);
			fprintf(fp, "\n");
			break;

		case CLASS_MILITARY://
			sl_plane_props(fp, save, od->props, TRUE);
			fprintf(fp, "\n");
			break;

		case CLASS_NDB://
		case CLASS_VOR://
		case CLASS_ILS://
			sl_nav_props(fp, save, od->props);
			fprintf(fp, "\n");
			break;

		case CLASS_AAGUN://
		case CLASS_SAM://
			sl_fd_props(fp, save, od->props);
			fprintf(fp, "\n");
			break;

		case CLASS_DEPOT:
			sl_depot_props(fp, save, od->props);
			break;

		case CLASS_HANGER:
			sl_hanger_props(fp, save, od->props);
			break;

		default:
			break;
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// EXTERN save_objects
void save_objects(FILE *fp)
{
	List *l;
	Object_def *od;

	fprintf(fp, "Objects %d\n\n", list_length(objects));

	for (l = objects->next; l != objects; l = l->next) {

		od = l->item;
		fprintf(fp, "%s ", class_name(od, TRUE));

		if (od == player) {
			fprintf(fp, "Player\n");
		}
		else {
			sl_string(fp, TRUE, od->symbol);
		}
		fprintf(fp, "%ld %ld %ld %d %d %s\n", od->px, od->py, od->pz, od->ry, od->flags, od->name);

		fprintf(fp, "%d ", od->strength);

		sl_string(fp, TRUE, od->dead_shape);

		save_object_class_data(fp, od);
		fprintf(fp, "\n");
	}
	save_paths(fp);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// PATHS
static long int default_posn_height = 0L;

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static BOOL FAR PASCAL db_path_proc(HWND hdlg, unsigned msg, WORD wParam, LONG lParam)
{
  //	int  i;
	List *l;
	char s[40];
	Object_def *od;
	Path *p;
	BOOL translated;

	lParam = lParam;

	switch(msg) {

		case WM_INITDIALOG:
			SetDlgItemText(hdlg, ID_DB_VALUE, selected_path->name);
			// Name of linked object for supply
			for (l = objects->next; l != objects; l = l->next) {
				od = l->item;
				if ((strlen(od->symbol) > 0) && (strcmp(od->symbol, "nil") != 0)) {
					SendDlgItemMessage(hdlg, ID_DB6_NAME, CB_ADDSTRING, 0, (LPARAM)((LPSTR)od->symbol));
				}
			}
			if (selected_path->supply) {
				SendDlgItemMessage(hdlg, ID_DB6_NAME, CB_SELECTSTRING, -1, (LPARAM)(selected_path->supply->symbol));
			}
			if (selected_posn) {
				SetDlgItemInt(hdlg, ID_DB_VALUE1, (int)selected_posn->y, TRUE);
				SetDlgItemInt(hdlg, ID_DB_VALUE2, (int)selected_posn->speed, TRUE);
				CheckDlgButton(hdlg, ID_DB_ACTIVE, selected_posn->active);
			}
			else {
				EnableWindow(GetDlgItem(hdlg, ID_DB_VALUE1), FALSE);
				EnableWindow(GetDlgItem(hdlg, ID_DB_VALUE2), FALSE);
				EnableWindow(GetDlgItem(hdlg, ID_DB_VALUE3), FALSE);
			}
			return(TRUE);

		case WM_COMMAND:

			switch (wParam) {

				case IDOK:
					GetDlgItemText(hdlg, ID_DB_VALUE, s, 16);
					p = find_path(s);
					if (p && (p != selected_path)) {
						warning("Path with name already exists");
					}
					else {
						strcpy(selected_path->name, s);
					}
					GetDlgItemText(hdlg, ID_DB6_NAME, s, 16);
					selected_path->supply = get_object_symbolic(s);
					if (selected_posn) {
						selected_posn->y     = GetDlgItemInt(hdlg, ID_DB_VALUE1, &translated, TRUE);
						selected_posn->speed = GetDlgItemInt(hdlg, ID_DB_VALUE2, &translated, TRUE);
						selected_posn->active = IsDlgButtonChecked(hdlg, ID_DB_ACTIVE);
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

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// EXTERN
void set_path_properties(RECT *rc)
{
	FARPROC lpProc;

	rc = rc;
	if (selected_path != NULL) {
		lpProc = MakeProcInstance(db_path_proc, appInstance);
		DialogBox(appInstance, "DIALOG_21", main_window, lpProc);
		FreeProcInstance(lpProc);
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static Path *alloc_path(void)
{
	Path *p;

	p = malloc(sizeof(Path));
	sprintf(p->name, "PATH_%03d", next_path++);
	p->supply = NULL;
	p->posns = new_list();
	list_add_last(p, paths);

	return p;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static Path *find_path(char *name)
{
	List *l;
	Path *p;

	// Check for existing path
	Map (l, paths) {
		p = l->item;
		if (str_eq(p->name, name, 16)) {
			return p;
		}
	}
	// Not found
	return NULL;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static Path *get_path(char *name)
{
 //	List *l;
	Path *p;

	// Check for existing path
	p = find_path(name);
	if (p) {
		return p;
	}

	// Not found - alloc new path
	p = alloc_path();
	strcpy(p->name, name);
	return p;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static Posn *new_posn(long int x, long int y, long int z)
{
	Posn *p;

	p = malloc(sizeof(Posn));
	p->x = x; p->y = y; p->z = z;
	p->speed = 0L;
	p->active = 0;

	default_posn_height = y;

	return p;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// EXTERN add_path - add new position to current path, create new path if current path is NULL
void add_path(long int x, long int z, BOOL last, RECT *rc)
{
	if (selected_path == NULL) {
		selected_path = alloc_path();
	}
	selected_posn = new_posn(x, default_posn_height, z);
	list_add_last(selected_posn, selected_path->posns);
	if (last) {
		change_tool(TOOL_PTH);
	}
	if (rc && selected_path && selected_posn) {
	  get_path_rect(selected_path, selected_posn, rc);
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void insert_path(long int x, long int z, RECT *rc)
{
	Posn *p;

	if (selected_path && selected_posn) {
		p = new_posn(x, default_posn_height, z);
		if (p) {
			list_insert_after(p, selected_posn, selected_path->posns);
			selected_posn = p;
			if (rc && selected_path && selected_posn) {
				get_path_rect(selected_path, selected_posn, rc);
			}
		}
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// EXTERN
void select_path(long int x, long int z, RECT *rc)
{
	RECT old_rect, new_rect;
	List *l, *l1;
	long int d, close;
	Path *old_path, *pt, *closest;
	Posn *p, *selp;

	old_path = NULL;
	if (selected_path) {
		old_path = selected_path;
	}
	closest = NULL;
	close = 0L;
	for (l = paths->next; l != paths; l = l->next) {
		pt = l->item;
		for (l1 = pt->posns->next; l1 != pt->posns; l1 = l1->next) {
			p = l1->item;
			d = ABS(p->x - x) + ABS(p->z - z);
			if (closest == NULL || d < close) {
				closest = pt;
				selp  = p;
				close = d;
			}
		}
	}
	if (closest) {
		selected_path = closest;
		selected_posn = selp;
	}
	if (selected_path && rc) {
		get_path_rect(selected_path, NULL, &new_rect);
		if (old_path  && (selected_path != old_path)) {
			get_path_rect(old_path, NULL, &old_rect);
			UnionRect(rc, &old_rect, &new_rect);
		}
		else {
			*rc = new_rect;
		}
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Create RECT covering all links from objects to selected_path
static void union_links(RECT *rc)
{
	RECT link;
	List *l;
	Object_def *od;
	Depot_props *dp;

	Map (l, objects) {
		od = l->item;
		if (od->type == CLASS_DEPOT || od->type == CLASS_HANGER) {
			dp = od->props;
			if (list_member(selected_path, dp->paths)) {
				get_link_rect(od, selected_path, &link);
				UnionRect(rc, rc, &link);
			}
		}
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// EXTERN move_path
void move_path(long int x, long int z, RECT *rc)
{
	RECT old_rect, new_rect;

	if (selected_posn) {
		get_path_rect(selected_path, selected_posn, &old_rect);
		if (selected_posn == Car(selected_path->posns)) {
			// find all objects linked to path
			union_links(&old_rect);
		}
		selected_posn->x = x; selected_posn->z = z;
		get_path_rect(selected_path, selected_posn, &new_rect);
		if (selected_posn == Car(selected_path->posns)) {
			// find all objects linked to path
			union_links(&new_rect);
		}
		UnionRect(rc, &old_rect, &new_rect);
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void path_delete(void)
{
	List *l;
	Object_def *od;
	Depot_props *dp;

	if (selected_path) {
		list_remove(selected_path, paths);
		Map (l, objects) {
			od = l->item;
			if (od->type == CLASS_DEPOT || od->type == CLASS_HANGER) {
				// remove path reference from od props
				dp = od->props;
				list_remove(selected_path,  dp->paths);
			}
		}
		selected_path = NULL;
		selected_posn = NULL;
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void path_delete_point(void)
{
	if (selected_path && selected_posn) {
		list_remove(selected_posn, selected_path->posns);
		selected_posn = NULL;
	}
	if (selected_path) {
		if (list_length(selected_path->posns) <= 0) {
			path_delete();
		}
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void draw_path(HDC hdc, Path *pt)
{
	int x0, y0, x1, y1;	//, tf;
	Posn *lp, *p;
	List *l;
	HPEN old_pen;

	lp = p = NULL;

	old_pen = SelectObject(hdc, (selected_path == pt) ? red_pen : white_pen);

	if ((show_paths) || (!show_paths && show_selpath && (selected_path == pt))) {

		//tf = 0;
		//if (pt->flags & BLUE_TARGET) {
		//	tf = 1;
		//}
		//if (pt->flags & TARGET) {
		//	tf = 2;
		//}
		//set_team_color(hdc, tf, (selected_path == pt) ? TRUE : FALSE);

		for (l = pt->posns->next; l != pt->posns; l = l->next) {
			p  = l->item;
			x1 = get_wx_mx(p->x);
			y1 = get_wy_my(p->z);

			if (lp) {
				x0 = get_wx_mx(lp->x);
				y0 = get_wy_my(lp->z);
				MoveTo(hdc, x0, y0);
				LineTo(hdc, x1, y1);
			}
			if (p == selected_posn) {
				MoveTo(hdc, x1 - box_size, y1 + box_size);
				LineTo(hdc, x1 + box_size, y1 + box_size);
				LineTo(hdc, x1 + box_size, y1 - box_size);
				LineTo(hdc, x1 - box_size, y1 - box_size);
				LineTo(hdc, x1 - box_size, y1 + box_size);
			}
			lp = p;
		}
	}
	SelectObject(hdc, old_pen);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void draw_paths(HDC hdc)
{
	Path *p;
	List *l;

	for (l = paths->next; l != paths; l = l->next) {
		p = l->item;

		draw_path(hdc, p);

		if (!show_paths && !show_selpath && !redraw){
			redraw_terrain();
			redraw = TRUE;
		}
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void save_path(FILE *fp, Path *path)
{
	List *l;
	Posn *p;

	// Write path name
	sl_string(fp, TRUE, path->name);

	// Write supply object & number of vertices
	fprintf(fp, "%s %d\n", ((path->supply) ? path->supply->symbol : "nil"), list_length(path->posns));
	for (l = path->posns->next; l != path->posns; l = l->next) {
		p = l->item;
		// Write path verticee data
		fprintf(fp, "%ld %ld %ld %d %d ", p->x, p->y, p->z, (int)p->speed, p->active);
	}
	fprintf(fp, "\n");
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void save_paths(FILE *fp)
{
	List *l;

	// Write section paths header
	fprintf(fp, "Paths %d %d\n", list_length(paths), next_path);
	for (l = paths->next; l != paths; l = l->next) {
		// Write individual paths
		save_path(fp, (Path*)(l->item));
		fprintf(fp, "\n");
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void load_path(FILE *fp)              // BOB NOTE ADDINS FOR PATHS
{
	long int x, y, z;
	char name[32];
	char supply[32];
 //	List *l;
	Posn *p;
	int   n, speed, active;
	Path *path;

	fscanf(fp, "%s %s %d", name, supply, &n);
	// get_path - may have already been created by load_object
	path = get_path(name);
	path->supply = get_object_symbolic(supply);
	while (n--) {
		active = 0;
		if (world_file_version >= 4) {
			fscanf(fp, "%ld %ld %ld %d %d", &x, &y, &z, &speed, &active);
		}
		else {
			fscanf(fp, "%ld %ld %ld %d", &x, &y, &z, &speed);
		}
		p = new_posn(x, y, z);
		p->speed = speed;
		p->active = active;
		list_add_last(p, path->posns);
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void load_paths(FILE *fp)
{
	char comment[32];
	int  n;

	n = 0;
	if (!paths_string_loaded) {
		fscanf(fp, "%s\n", comment);
		//if (comment == NULL) {
		//	if (strcmp(comment, "Paths") != 0) {
		//		stop("Error reading Paths string");
		//	}
		//}
	}

	fscanf(fp, "%d %d\n", &n, &next_path);
	while (n--) {
		load_path(fp);
	}
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// EXTERN associate_object_path:  called from edit.c to link selected_object to path closest to wx/wy
void associate_object_path(int wx, int wy, RECT *rc)
{
	RECT rect;

	if (selected_object) {
		select_path(get_mx(wx), get_my(wy), rc);
		if (selected_path) {
			add_path_to_depot(selected_path);
			get_link_rect(selected_object, selected_path, &rect);
			UnionRect(rc, &rect, rc);
		}
	}
	change_tool(TOOL_OBJSEL);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// EXTERN break_object_path:  called from edit.c to break link from selected_object to path closest to wx/wy
void break_object_path(int wx, int wy, RECT *rc)
{
	RECT rect;

	if (selected_object) {
		select_path(get_mx(wx), get_my(wy), rc);
		if (selected_path) {
			get_link_rect(selected_object, selected_path, &rect);
			UnionRect(rc, &rect, rc);
			remove_path_from_depot(selected_path);
		}
	}
	change_tool(TOOL_OBJSEL);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
char *associate_text(void)
{
	return "Select path";
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void reset_path_selections(void)
{
	Path *old_path;
	HDC hdc;

	old_path = selected_path;
	selected_path = NULL;
	selected_posn = NULL;

	// undraw old path selection
	hdc = GetDC(edit_window);
	if (old_path) {
		draw_path(hdc, old_path);
	}
	ReleaseDC(edit_window, hdc);
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
// Setup and Initialisation
void reset_selections(void)
{
	selected_object = NULL;
	selected_path = NULL;
	selected_posn = NULL;
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————
void setup_objects(void)
{
	int i;

	for (i = 0; i < MAX_CLASS; i++) {
		class_names[i] = NULL;
	}

	// Animators
	new_class_def(CLASS_ANIM		, "Anim");

	// Basic class
	new_class_def(CLASS_CULTURAL	, "Cultural");

	// Aircraft classes
	new_class_def(CLASS_MILITARY	, "Military");
	new_class_def(CLASS_CIVIL	, "Civil");
	new_class_def(CLASS_BRICK	, "Brick");

	// Navigation classes
	new_class_def(CLASS_NDB		, "NDB");
	new_class_def(CLASS_VOR		, "VOR");
	new_class_def(CLASS_ILS		, "ILS");

	// Fixed defences
	new_class_def(CLASS_AAGUN	, "AAGun");
	new_class_def(CLASS_SAM		, "SAM");

	// Generators
	new_class_def(CLASS_DEPOT	, "Depot");
	new_class_def(CLASS_HANGER	, "Hanger");

	objects = new_list();
	paths   = new_list();

	reset_selections();
}
