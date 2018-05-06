/* FST Terrain Editor */

#ifndef defs_h
#include <windows.h>
#include <stdio.h>

/* terrain.c */
extern char szAppName[];
extern char szEditName[];
extern char szControlName[];
extern HANDLE appInstance;
extern HWND main_window;
extern void warning(char *message);
extern void stop(char *message);
extern void open_view_window(void);

/* control.c */
#define TERRAIN_MODE 1
#define OBJECT_MODE  2 

#define TOOL_OBJSEL 9
#define TOOL_OBJNEW 10
#define TOOL_OBJLNK 11
#define TOOL_OBJBRK 12
#define TOOL_PTH    13
#define TOOL_PTHNEW 14
#define TOOL_PTHADD 15
#define TOOL_OBJPST 16

extern int edit_mode;
extern char *tool_name(int tool);
extern int current_tool, last_tool;
extern int edit_height, edit_delta, edit_radius, fractal_iters;
extern HWND create_control_window(HANDLE hInstance, HWND parent);
extern void change_tool(int tool);

/* edit.c */
extern int world_file_version;
extern char* proj_filename(char *name);
extern HPEN red_pen, white_pen, blu_pen, pur_pen, gry_pen;
extern HWND edit_window;
extern int terrain_x_size, terrain_y_size;
extern int edit_step, edit_radius;
extern int fractal_iters;
extern int get_tx(int wx);
extern int get_ty(int wy);
extern long get_mx(int wx);
extern long get_my(int wy);
extern int get_wx_mx(long mx);
extern int get_wy_my(long my);
extern int get_xsize(long mx);
extern int get_ysize(long my);
extern void get_terrain_rect(int tx1, int ty1, int tx2, int ty2, RECT *rc);
extern int get_point_height(long mx, long my);
extern HWND create_edit_window(HANDLE hInstance, HWND parent);
extern int check_edit_palette(void);
extern void zoom_out(void);
extern void redraw_terrain(void);
extern void resize_edit(int width, int height);
extern void new_terrain(int width, int height);
extern int save_world(char *file_name);
extern int load_world(char *file_name);
extern void save_view_data(char *file_name);
extern void setup_terrain(void);
extern void tidyup_terrain(void);

extern void show_status_line(int tool);

/* objects.c */
extern char *get_selected_name(void);
extern void select_object(int wx, int wy, RECT *rc);
extern void move_selected_object(int wx, int wy, RECT *rc);
extern void add_object(int wx, int wy);
extern void set_selected_properties(RECT *rc);
extern void draw_objects(HDC hdc, RECT *rc);
extern void choose_object(char *file_name);
extern void remove_object(void);
extern void destroy_objects(void);
extern void set_object_angle(int ry);
extern void load_objects(FILE *fp);
extern void save_objects(FILE *fp);
extern void setup_objects(void);

/* shape.c */
extern void *load_current_shape(char *file_name, char *shape_name);
extern void draw_current_shape(HDC hdc, int wx, int wy, int ry, int x_size, int y_size);
extern void setup_shape(void);
extern void tidyup_shape(void);

// ---------------------------
// edit.c
extern char proj_path[128];
extern int edit_grid_size; // Grid size in M
extern HPEN dash_pen;
extern HPEN dot_pen;

// objects.c
extern void path_delete_point(void);
extern void path_delete(void);
extern void choose_default_object(HWND hwnd);

extern BOOL get_selected_start(void);
extern int  get_selected_wx(void);
extern int  get_selected_wy(void);
extern BOOL selected_object_associable(void);


extern void copy_selected_object(void);
extern void paste_object(int wx, int wy);
extern BOOL is_copy_object(void);extern BOOL is_selected_object(void);
extern BOOL is_selected_path(void);
extern BOOL is_selected_posn(void);

extern void new_path(void);
extern char *associate_text(void);
extern void reset_path_selections(void);
extern void set_path_properties(RECT *rc);
extern void associate_object_path(int wx, int wy, RECT *rc);
extern void break_object_path(int wx, int wy, RECT *rc);
extern void add_path(long int x, long int z, BOOL last, RECT *rc);
extern void select_path(long int x, long int z, RECT *rc);
extern void move_path(long int x, long int z, RECT *rc);
extern void insert_path(long int x, long int z, RECT *rc);
extern void draw_paths(HDC hdc);	//, RECT *rc);

// control.c
extern void force_redraw_toolbar(void);

// terrain.c
extern long int start_x, start_z;
extern void update_menu(void);
extern void open_view_window(void);
extern void ihelp(char *message);

// symbols (defined in terrain.c used in edit.c)
extern int wind_speed;
extern int wind_dirn;
extern int time_of_day;
extern int automatic_colours;

#endif
