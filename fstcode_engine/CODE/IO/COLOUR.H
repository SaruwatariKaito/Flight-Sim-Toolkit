#ifndef colour_h
#define colour_h

#include "graph.h"

#define GAME_COLS 256

extern sColour *used_palette;
extern void reset_palette(void);
extern void save_as_base_palette(int first, int last);
extern void use_base_palette(int first, int last);
extern void set_palette(int n, int red, int green, int blue);
extern void use_palette(int first, int last);
extern void read_palette(sColour *pal, int first, int last);
extern void write_palette(sColour *pal, int first, int last);
extern void zero_palette(int first, int last);
extern void fade_out(int steps, int first, int last);
extern void fade_in(int steps, int first, int last);
extern int match_colour(int r, int g, int b, int first, int last);
extern void map_greens(int first, int last, int first_green, int num_greens);
extern void unmap_greens(int first, int last);
extern void map_flir(int first, int last, int first_green, int num_greens, uchar *map);
extern void default_colours(void);
extern void load_colourmap(char *map, int first, int last);
extern void save_colourmap(char *map, int first, int last);
extern void normal_colourmap(void);
extern void g_colourmap(int n);
extern void night_colourmap(int first, int last, int n, int max);
extern void setup_colours(void);
extern void tidyup_colours(void);

#endif
