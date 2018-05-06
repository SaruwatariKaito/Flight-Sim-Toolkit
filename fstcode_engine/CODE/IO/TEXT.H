/* Header for text library */

#ifndef text_h
#define text_h

#include "graph.h"

#define FONT_SIZE (128 - 32)

typedef struct {
	int max_width, height;
	uchar *widths;
	short *data;
} Font;

extern Font *current_font;
extern Font *use_font(char *file);
extern Font *new_font(int max_width, int height);
extern void save_font(Font *f, char *file);
extern Font *load_font(char *file);
extern int draw_char(int x, int y, int c, int col);
extern int draw_string(int x, int y, char *s, int col);
extern void draw_boxed_string(sBox *box, char *s, int col);
extern int draw_string_width(char *s);
extern void draw_string_size(char *s, int *width, int *height);

#endif
