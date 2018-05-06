/* FST Model Editor */

#ifndef defs_h

#include <windows.h>
#include <stdarg.h>

typedef unsigned char uchar;

/* model.c */
extern HWND main_window;
extern HANDLE appInstance;
extern void stop(char *message);
extern void warning(char *message);
extern char* proj_filename(char *name);

/* edit.c */
extern int model_x_size, model_y_size;

extern void redraw_client(HDC hdc, RECT *rect);
extern void resize_client(HWND hwnd, int width, int height);
extern void scroll_client(HWND hwnd, int refresh, int xpos, int ypos);
extern void create_client(HWND hwnd);

extern void mouse_moved(HWND hwnd, int wx, int wy);
extern void mouse_pressed(HWND hwnd, int wx, int wy, int click_side);
extern void mouse_released(HWND hwnd, int wx, int wy, int click_side);
extern void mouse_double_clicked(HWND hwnd, int wx, int wy, int click_side);

extern void new_model(void);
extern int save_model(char *file_name);
extern int load_model(char *file_name);
extern void setup_model(void);
extern void tidyup_model(void);

extern void edit_properties(void);
extern void edit_engines(void);
extern void edit_canvas(void);

extern void edit_filled(HWND hwnd);
extern void edit_vertex(HWND hwnd);
extern void edit_grids(HWND hwnd);
extern void change_zoom(HWND hwnd, int zf);

#endif
