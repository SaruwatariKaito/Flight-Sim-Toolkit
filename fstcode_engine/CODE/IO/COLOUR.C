/* Colour functions */

#include <stdlib.h>
#include <stdio.h>

#include "colour.h"

#define MAX_COLOURS 256

static sColour *palette = NULL;
sColour *used_palette = NULL;
static sColour *grey_palette = NULL;
static sColour *base_palette = NULL;

extern void *alloc_bytes(unsigned int nbytes);

/* colour values are 0 - 255 */
void set_palette(int n, int red, int green, int blue)
{
	if (n < MAX_COLOURS) {
		palette[n].red = red;
		palette[n].green = green;
		palette[n].blue = blue;
		palette[n].average = (red + green + blue) / 3;
	}
}

void reset_palette(void)
{
	int n;
	int32 *p;

	p = (int32 *)palette;
	for (n = 0; n < MAX_COLOURS; n++) {
		*p++ = 0L;
		cols[n] = EXPAND_COL(n);
	}
}

static void copy_palette(sColour *source, sColour *dest, int n)
{
	while (n--) {
		*dest++ = *source++;
	}
}

void read_palette(sColour *pal, int first, int last)
{
	copy_palette(&palette[first], pal, last - first + 1);
}

void write_palette(sColour *pal, int first, int last)
{
	copy_palette(pal, &palette[first], last - first + 1);
}

void save_as_base_palette(int first, int last)
{
	copy_palette(&palette[first], &base_palette[first], last - first + 1);
}

void use_base_palette(int first, int last)
{
	copy_palette(&base_palette[first], &palette[first], last - first + 1);
	use_palette(first, last);
}

void use_palette(int first, int last)
{
	int n;
	int32 *p1, *p2;

	p1 = (int32 *)&palette[first];
	p2 = (int32 *)&used_palette[first];
	for (n = first; n <= last; n++) {
		*p2++ = *p1++;
		cols[n] = EXPAND_COL(n);
	}
	vsync();
	load_palette(&palette[first], first, last);
}

void zero_palette(int first, int last)
{
	int n;
	int32 *p;

	p = (int32 *)&used_palette[first];
	for (n = first; n <= last; n++) {
		*p++ = 0L;
		cols[n] = EXPAND_COL(n);
	}
	vsync();
	load_palette(&used_palette[first], first, last);
}

static int fade_colour(int val, int reduction, int max)
{
	return((val * reduction) / max);
}

void fade_out(int steps, int first, int last)
{
	int i, n;

	for (i = steps; i >= 0; i--) {
		for (n = first; n < last; n++) {
			grey_palette[n].red = fade_colour(used_palette[n].red, i, steps);
			grey_palette[n].green = fade_colour(used_palette[n].green, i, steps);
			grey_palette[n].blue = fade_colour(used_palette[n].blue, i, steps);
		}
		vsync();
		load_palette(&grey_palette[first], first, last);
	}
	zero_palette(first, last);
}

void fade_in(int steps, int first, int last)
{
	int i, n;

	for (i = 0; i <= steps; i++) {
		for (n = first; n <= last; n++) {
			used_palette[n].red = fade_colour(palette[n].red, i, steps);
			used_palette[n].green = fade_colour(palette[n].green, i, steps);
			used_palette[n].blue = fade_colour(palette[n].blue, i, steps);
			cols[n] = EXPAND_COL(n);
		}
		vsync();
		load_palette(&used_palette[first], first, last);
	}
}

int match_colour(int r, int g, int b, int first, int last)
{
	int i, n, val, min;

	min = 10000;
	for (i = first; i <= last; i++) {
		val = ABS(r - palette[i].red) + ABS(g - palette[i].green) + ABS(b - palette[i].blue);
		if (val < min) {
			min = val;
			n = i;
		}
	}
	return n;
}

void map_greens(int first, int last, int first_green, int num_greens)
{
	int i, n;

	for (i = first; i <= last; i++) {
		n = first_green + ((palette[i].average * num_greens) >> 6);
		cols[i] = EXPAND_COL(n);
	}
}

void unmap_greens(int first, int last)
{
	int i;

	for (i = first; i <= last; i++) {
		cols[i] = EXPAND_COL(i);
	}
}

void map_flir(int first, int last, int first_green, int num_greens, uchar *map)
{
	int i, n, a;

	for (i = first; i <= last; i++) {
		a = base_palette[i].average;
		n = first_green + ((a * num_greens) >> 6);
		map[i] = n;
	}
}

void convert_colours(char *file, uchar *buff, int width, int height)
{
}

static void set_arc_colour(int index, int col)
{
	int r, g, b, w;

	w = (col & 0x03);
	r = (((col & 0x10) >> 1) | (col & 0x04) | w) * 255 / 15;
	g = (((col & 0x40) >> 3) | ((col & 0x20) >> 3) | w) * 255 / 15;
	b = (((col & 0x80) >> 4) | ((col & 0x08) >> 1) | w) * 255 / 15;
	set_palette(index, r, g, b);
}

void default_colours(void)
{
    int i;

	reset_colours(256);
	for (i = 0; i < GAME_COLS; i++) {
		cols[i] = EXPAND_COL(i);
		set_arc_colour(i, i);
	}
}

void load_colourmap(char *map, int first, int last)
{
    FILE *fp;
    int i, j, r, g, b, num_cols;

	fp = fopen(map,"r");
	if (fp != NULL) {
		fscanf(fp, "%d\n", &num_cols);
		if (num_cols > GAME_COLS)
			num_cols = GAME_COLS;
		if (num_cols > last)
			num_cols = last + 1;
    	for(i = 0; i < num_cols; i++) {
			fscanf(fp, "%d %x,%x,%x\n", &j, &r, &g, &b);
			if ((i >= first) && (i <= last)) {
	 			cols[j] = EXPAND_COL(j);
				set_palette(j, r, g, b);
			}
		}
    	fclose(fp);
	} else {
		default_colours();
	}
}

void save_colourmap(char *map, int first, int last)
{
    FILE *fp;
    int i;

	fp = fopen(map,"w");
	if (fp != NULL) {
    	for(i = first; i <= last; i++) {
			fprintf(fp, "%d %x,%x,%x\n", i, palette[i].red, palette[i].green, palette[i].blue);
		}
    	fclose(fp);
	}
}

void normal_colourmap(void)
{
	load_palette(&palette[0], 0, GAME_COLS - 1);
}

void g_colourmap(int n)
{
	static int last_n = 0, lastin1 = 0, lastin2 = 0;
    int i, c, r, g, b, av;

	lastin2 = lastin1;
	lastin1 = n;
	n = (lastin2 + lastin1) >> 2;
	if (n != last_n) {
		last_n = n;
		if (n == 0) {
			load_palette(&palette[0], 0, GAME_COLS - 1);
		} else if (n >= 16) {
			for (i = 0; i < GAME_COLS; i++) {
				grey_palette[i].red = 0;
				grey_palette[i].green = 0;
				grey_palette[i].blue = 0;
			}
			load_palette(&grey_palette[0], 0, GAME_COLS - 1);
		} else if (n > 0) {
			n++;
			for (i = 0; i < GAME_COLS; i++) {
				r = palette[i].red;
				g = palette[i].green;
				b = palette[i].blue;
				av = palette[i].average;
				c = r - (((r - av) * n) >> 4);
				grey_palette[i].red = c;
				c = g - (((g - av) * n) >> 4);
				grey_palette[i].green = c;
				c = b - (((b - av) * n) >> 4);
				grey_palette[i].blue = c;
			}
			load_palette(&grey_palette[0], 0, GAME_COLS - 1);
		} else {
			n = -n + 1;
			for (i = 0; i < GAME_COLS; i++) {
				r = palette[i].red;
				g = palette[i].green;
				b = palette[i].blue;
				av = palette[i].average >> 1;
				c = r - (((r - av) * n) >> 4);
				grey_palette[i].red = c;
				c = g - ((g * n) >> 4);
				if (c < 0)
					c = 0;
				grey_palette[i].green = c;
				c = b - ((b * n) >> 4);
				if (c < 0)
					c = 0;
				grey_palette[i].blue = c;
			}
			load_palette(&grey_palette[0], 0, GAME_COLS - 1);
		}
	}
}

void night_colourmap(int first, int last, int n, int max)
{
    int i, r, g, b, av;

	if (n == 0) {
		copy_palette(&base_palette[first], &palette[first], last - first + 1);
		load_palette(&palette[first], first, last);
	} else {
		for (i = first; i <= last; i++) {
			r = base_palette[i].red;
			g = base_palette[i].green;
			b = base_palette[i].blue;
			av = base_palette[i].average / n;
			r -= MULDIV(r - av, n, max);
			palette[i].red = r;
			g -= MULDIV(g - av, n, max);
			palette[i].green = g;
			b -= MULDIV(b - av, n, max);
			palette[i].blue = b;
			palette[i].average = (r + g + b) / 3;
		}
		load_palette(&palette[first], first, last);
	}
}

void setup_colours(void)
{
	if (palette == NULL) {
		palette = alloc_bytes(sizeof(sColour) * MAX_COLOURS);
		used_palette = alloc_bytes(sizeof(sColour) * MAX_COLOURS);
		grey_palette = alloc_bytes(sizeof(sColour) * MAX_COLOURS);
		base_palette = alloc_bytes(sizeof(sColour) * MAX_COLOURS);
		if ((palette == NULL) || (used_palette == NULL) ||
			(grey_palette == NULL) || (base_palette == NULL)) {
			stop("No memory for palettes");
		}
	}
}

void tidyup_colours(void)
{
	free(base_palette);
	free(grey_palette);
	free(used_palette);
	free(palette);
}

