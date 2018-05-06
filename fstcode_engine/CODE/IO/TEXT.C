/* Text library */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "text.h"

extern void *alloc_bytes(unsigned int nbytes);

Font *current_font = NULL;

typedef struct Font_def {
	struct Font_def *next;
	char file[16];
	Font *font;
} Font_def;

static Font_def *used_fonts = NULL;


Font *use_font(char *file)
{
	Font_def *fd;
	Font *f;
	char full_name[20];

	for (fd = used_fonts; fd != NULL; fd = fd->next) {
		if (strcmp(file, fd->file) == 0) {
			current_font = fd->font;
			break;
		}
	}
	if (fd == NULL) {
		f = load_font(file);
		if (f == NULL) {
			sprintf(full_name, "D\\%s", file);
			f = load_font(full_name);
		}
		if (f != NULL) {
			fd = alloc_bytes(sizeof(Font_def));
			if (fd == NULL)
				stop("No memory for font definition");
			fd->font = f;
			strncpy(fd->file, file, 15);
			fd->next = used_fonts;
			used_fonts = fd;
			current_font = fd->font;
		}
	}
	return current_font;
}

Font *new_font(int max_width, int height)
{
	int n, i;
	uchar *p;
	short *ps;
	Font *f;

	f = (Font *)alloc_bytes(sizeof(Font));
	if (f == NULL)
		stop("No memory for new font");
	f->max_width = max_width;
	f->height = height;
	f->widths = (uchar *)alloc_bytes(FONT_SIZE);
	if (f->widths == NULL)
		stop("No memory for new font width data");
 	p = f->widths;
	for (n = 0; n < FONT_SIZE; n++) {
		*p++ = max_width;
	}
	f->data = (short *)alloc_bytes(2 * height * FONT_SIZE);
	if (f->data == NULL)
		stop("No memory for new font data");
	ps = f->data;
	for (n = 0; n < FONT_SIZE; n++) {
		for (i = 0; i < height; i++) {
			*ps++ = 0;
		}
	}
	return f;
}

void save_font(Font *f, char *file)
{
	int n, i, c;
	short *p;
	FILE *fp;

	fp = fopen(file, "wb");
	if (fp != NULL) {
		fprintf(fp, "%d %d %d\n", 2, f->max_width, f->height);
		p = f->data;
		for (n = 0; n < FONT_SIZE; n++) {
			for (i = 0; i < f->height; i++) {
				c = *p++;
				putc(c & 0xff, fp);
				putc((c >> 8) & 0xff, fp);
			}
		}
		fclose(fp);
	}
}

static int find_width(int pat, int max)
{
	int i, b, n;

	for (i = 1, b = 0x8000, n = 1; i <= max; i++, b >>= 1) {
		if (pat & b)
			n = i;
	}
	return n;
}

Font *load_font(char *file)
{
	int max_width, height, w, max_w, n, i, c, version;
	uchar *wp;
	short *dp;
	FILE *fp;
	Font *f;

	f = NULL;
	fp = fopen(file, "rb");
	if (fp != NULL) {
		fscanf(fp, "%d %d %d\n", &version, &max_width, &height);
		f = new_font(max_width, height);
		wp = f->widths;
		dp = f->data;
		for (c = 0, n = 0; (n < FONT_SIZE) && (c != EOF); n++) {
			for (i = 0, max_w = 0, w = 0; (i < height) && (c != EOF); i++) {
				if (version == 1) {
					c = getc(fp) << 8;
				} else {
					c = getc(fp);
					c |= getc(fp) << 8;
				}
				if (c >= 0) {
					*dp++ = c;
					w = find_width(c, max_width);
					if (w > max_w)
						max_w = w;
				}
			}
			if (c >= 0) {
				*wp++ = max_w;
			}
		}
		f->widths[0] = max_width - 1;
		fclose(fp);
	}
	return f;
}

int draw_char(int x, int y, int c, int col)
{
	int i, pat, width;
	short *p;
	Font *f;

	f = current_font;
	width = 0;
	if ((x > clip_region.right) || (y > clip_region.bottom))
		return width;
	if ((x + f->max_width) < clip_region.left)
		return width;
	if ((y + f->height) < clip_region.top)
		return width;
	if (f != NULL) {
		c -= ' ';
		p = f->data + (c * f->height);
		col = COL(col);
		width = f->widths[c];
		for (i = 0; i < f->height; i++) {
			pat = *p++;
			char_prim(x, y++, width, pat, col);
		}
	}
	return width;
}

int draw_string(int x, int y, char *s, int col)
{
	int c, n;

	if (current_font == NULL)
		stop("No font selected");
	for (n = 0; (n < 100) && ((c = *s++) != '\0'); n++) {
		x += draw_char(x, y, c, col) + 1;
	}
	return x;
}

void draw_boxed_string(sBox *box, char *s, int col)
{
	int c, x, y, n;
    Screen_clip sc;

	if (current_font == NULL)
		stop("No font selected");
	sc = clip_region;
    clipping(box->x1, box->x2, box->y1, box->y2);
	x = box->x1 + 1;
	y = box->y1 + 1;
	for (n = 0; (n < 100) && ((c = *s++) != '\0'); n++) {
		if (c != '\n')
			x += draw_char(x, y, c, col) + 1;
		if ((c == '\n') || (x > (box->x2 - current_font->max_width))) {
			x = box->x1 + 1;
			y += current_font->height + 1;
		}
	}
    clipping(sc.left, sc.right, sc.top, sc.bottom);
}

int draw_string_width(char *s)
{
	int c, width, n;
	Font *f;

	if (current_font == NULL)
		stop("No font selected");
	f = current_font;
	width = 0;
	if (f != NULL) {
		for (n = 0; (n < 100) && ((c = *s++) != '\0'); n++) {
			c -= ' ';
			width += f->widths[c] + 1;
		}
	}
	return width;
}

void draw_string_size(char *s, int *width, int *height)
{
	*width = draw_string_width(s);
	*height = current_font->height;
}

