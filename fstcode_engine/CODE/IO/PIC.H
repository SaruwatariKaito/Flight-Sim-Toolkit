/* PCX image functions */

#ifndef pic_h
#define pic_h

#include "graph.h"
#include "colour.h"

typedef struct {
	int width, height, cols;
} PCX_def;

extern PCX_def PCX_info;

extern void setup_pic(void);
extern void tidyup_pic(void);
extern int load_pic(char *file, sBitmap *pic_ptr, int max_width, int max_height, int set_palette, int num_cols);
extern int show_pic(char *file, int max_height, int set_palette, int num_cols);
extern int load_pcx_info(char *file);
extern void save_screen(char *file_name);

#endif
