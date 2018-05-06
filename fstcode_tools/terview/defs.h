/* FST Shape Viewer */

#ifndef defs_h
#define defs_h

#include <windows.h>

#include "graph.h"

/* terview.c */
extern void stop(char *message, ...);
extern void warning(char *message, ...);

/* ground.c */
#define M(X) ((int32)(X) << 11)
#define MM(X) ((int32)(X) << 1)
#define KM(X) (M(X) * 1000L)
#define Meter(X) ((int32)(X) << 3)
extern int grid_on;
extern int filled;
extern int solid_polys;
extern int 	update_required;

extern void mouse_moved(HWND hwnd, int wx, int wy);
extern void mouse_down(HWND hwnd, int wx, int wy, int right);
extern void mouse_up(HWND hwnd, int wx, int wy, int right, int key);
extern void mouse_double_pressed(HWND hwnd, int wx, int wy, int right);

extern void redraw_client(HDC hdc, RECT *rect);
extern void create_client(HWND hwnd);
extern void resize_client(HWND hwnd, int width, int height);

extern int  load_terrain(char *file, int startup);
extern void setup_ground(void);
extern void tidyup_ground(void);


#endif
