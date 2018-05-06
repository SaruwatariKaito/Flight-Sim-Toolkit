/* PCX image file functions */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "pic.h"

PCX_def PCX_info = {0, 0, 0};

extern void *alloc_bytes(unsigned int nbytes);

#define DEFAULT_LINE_SIZE MAX_SCREEN_X
static unsigned int line_size = DEFAULT_LINE_SIZE;
static uchar *line_buffer = NULL;
static uchar *expand_buffer = NULL;

static FILE *pic_fp = NULL;

static void reset_pic(void);

/* PCX file format */
/* Simple PCX:
 * 128 byte header
 * Run length encoded 8 bit pixels: BITS 6,7: 11<count>,<colour> or 00<colour>
 * Flag byte
 * Palette
 */

typedef struct {
	uchar manuf;	/* always 0x0a */
	uchar version;	/* use 0x05 */
	uchar encod;	/* 1 = run length */
	uchar bitpx;	/* bits per pixel (= 8 or 1) */
	short x1,y1,x2,y2;	/* picture dimensions */
	short hres,vres;	/* display resolution */
	uchar colormap[48];	/* palette if 16 or less colours */
	uchar vmode;	/* not used */
	uchar nplanes;	/* number of bitplanes (= 1 or 4) */
	short bplin;	/* bytes per (plane) line */
	short palinfo;	/* palette info. 1=colour, 2=monochrome */
	short shres,svres;	/* scanner resolution in DPI */
	uchar xtra[54];	/* space filler */
} PCX_Header;

static PCX_Header header;

static int open_pic_file(char *file)
{
	pic_fp = fopen(file, "rb");
	if (pic_fp == NULL) {
		return FALSE;
	}
	return TRUE;
}

static void close_pic_file(void)
{
	fclose(pic_fp);
}

static uchar next_pic_byte(void)
{
	int c;

	c = fgetc(pic_fp);
	if (c == EOF) {
		return 0;
/*		close_pic_file();
		stop("PCX picture file corrupted");*/
	}
	return((uchar)c);
}

static int next_pic_char(void)
{
	return(fgetc(pic_fp));
}

static int find_symbol(char *name)
{
	int c, found;
	char *s;

	found = FALSE;
	s = name;
	c = next_pic_char();
	if (c >= 0) {
		while (TRUE) {
			if (*s == c) {
				s++;
			} else {
				s = name;
			}
			if (*s == '\0') {
				found = TRUE;
				break;
			}
			c = next_pic_char();
			if (c < 0)
				break;
		}
	}
	return found;
}

static int32 next_pic_long(void)
{
	int32 val;
	uchar p0, p1, p2, p3;

	p0 = next_pic_byte();
	p1 = next_pic_byte();
	p2 = next_pic_byte();
	p3 = next_pic_byte();
	val = (p0 << 24) | (p1 << 16) | (p2 << 8) | p3;
	return val;
}

static short next_pic_word(void)
{
	short val;
	uchar p0, p1;

	p0 = next_pic_byte();
	p1 = next_pic_byte();
	val = (p0 << 8) | p1;
	return val;
}

static void get_header(void)
{
	int i;
	uchar *p;

	p = (uchar *)&header;
	for (i = 0; i < 128; i++) {
		*p++ = next_pic_byte();
	}
/*	printf("manuf: %2x\n", header.manuf);
	printf("version: %2x\n", header.version);
	printf("hres %d, vres %d\n", header.hres, header.vres);
	printf("x1 %d, y1 %d; x2 %d, y2 %d\n", header.x1, header.y1, header.x2, header.y2);
	wait_ascii();*/
}

static void get_colormap(int set, int num_cols)
{
	int flag, i, r, g, b;
	uchar *p;

	if (header.bitpx == 8) {
		if (set) {
			fseek(pic_fp, -((256 * 3) + 1), SEEK_END);
			flag = next_pic_byte();
			if (flag == 12) {
				if (num_cols > 256)
					num_cols = 256;
				for (i = 0; i < num_cols; i++) {
					r = next_pic_byte();
					g = next_pic_byte();
					b = next_pic_byte();
					set_palette(i, r, g, b);
				}
			}
			fseek(pic_fp, 128, SEEK_SET);
		}
	} else {
		if (set) {
			if (num_cols > 16)
				num_cols = 16;
			for (i = 0, p = header.colormap; i < num_cols; i++) {
				r = *p++;
				g = *p++;
				b = *p++;
				set_palette(i, r, g, b);
			}
		}
	}
}

static void merge_line(uchar *in, int bit, uchar *merge_buff, unsigned int size)
{
	unsigned int i;
	uchar *p, c;

	p = in;
	for (i = 0; i < size; i++, merge_buff += 8) {
		c = *p++;
		merge_buff[0] |= ((c & 0x80) >> 7) << bit;
		merge_buff[1] |= ((c & 0x40) >> 6) << bit;
		merge_buff[2] |= ((c & 0x20) >> 5) << bit;
		merge_buff[3] |= ((c & 0x10) >> 4) << bit;
		merge_buff[4] |= ((c & 0x08) >> 3) << bit;
		merge_buff[5] |= ((c & 0x04) >> 2) << bit;
		merge_buff[6] |= ((c & 0x02) >> 1) << bit;
		merge_buff[7] |= (c & 0x01) << bit;
	}
}

/* Run length encoded */
static int expand_line(uchar *p, int length)
{
	unsigned int c, count, i;
	uchar col;

	for (i = 0; i < length; ) {
		c = next_pic_byte();
		if ((c & 0xc0) == 0xc0) {
			count = c & 0x3f;
			i += count;
			col = next_pic_byte();
			while (count--)
				*p++ = col;
		} else {
			i++;
			*p++ = c;
		}
	}
	return FALSE;
}

static int read_line(uchar *out, unsigned int size)
{
	int i;

	for (i = 0; i < size; i++) {
		*out++ = next_pic_byte();
	}
	return FALSE;
}

static void get_bitmap(uchar *buff, int max_height)
{
	int error;
	unsigned int y, width, height, size, n;
	uchar *out_buff;

	width = PCX_info.width;
	height = PCX_info.height;
	size = (width * max_height);
	for (n = 0, out_buff = buff; n < size; n++)
		*out_buff++ = 0;
	if (header.encod == 1) {
		if (header.nplanes == 1) {
			for (y = 0, error = FALSE; (y < height) && !error; y++) {
				if (y < max_height) {
					error = expand_line(buff, width);
					buff += width;
				} else {
					error = expand_line(expand_buffer, width);
				}
			}
		} else if (header.nplanes == 4) {
			for (y = 0, error = FALSE; (y < height) && !error; y++) {
				error = expand_line(expand_buffer, header.bplin * header.nplanes);
				if (y < max_height) {
					for (n = 0, out_buff = expand_buffer; (n < header.nplanes) & !error; n++) {
						merge_line(out_buff, n, buff, header.bplin);
						out_buff += header.bplin;
					}
					buff += width;
				}
			}
		}
	} else {
		if (header.nplanes == 1) {
			for (y = 0, error = FALSE; (y < height) && !error; y++) {
				if (y < max_height) {
					error = read_line(buff, width);
					buff += width;
				} else {
					error = read_line(expand_buffer, width);
				}
			}
		} else if (header.nplanes == 4) {
			for (y = 0, error = FALSE; (y < height) && !error; y++) {
				error = read_line(expand_buffer, header.bplin * header.nplanes);
				if (y < max_height) {
					for (n = 0, out_buff = expand_buffer; (n < header.nplanes) & !error; n++) {
						merge_line(out_buff, n, buff, header.bplin);
						out_buff += header.bplin;
					}
					buff += width;
				}
			}
		}
	}
}

static void show_bitmap(void)
{
	int error;
	unsigned int y, width, height, n;
	uchar *out_buff;

	width = PCX_info.width;
	height = PCX_info.height;
	if (header.encod == 1) {
		if (header.nplanes == 1) {
			for (y = 0, error = FALSE; (y < height) && !error; y++) {
				error = expand_line(line_buffer, width);
    			bitmap_prim(0, y, screen_width, line_buffer);
			}
		} else if (header.nplanes == 4) {
			for (y = 0, error = FALSE; (y < height) && !error; y++) {
				for (n = 0, out_buff = line_buffer; n < width; n++)
					*out_buff++ = 0;
				error = expand_line(expand_buffer, header.bplin * header.nplanes);
				for (n = 0, out_buff = expand_buffer; (n < header.nplanes) & !error; n++) {
					merge_line(out_buff, n, line_buffer, header.bplin);
					out_buff += header.bplin;
				}
    			bitmap_prim(0, y, screen_width, line_buffer);
			}
		}
	} else {
		if (header.nplanes == 1) {
			for (y = 0, error = FALSE; (y < height) && !error; y++) {
				error = read_line(line_buffer, width);
    			bitmap_prim(0, y, screen_width, line_buffer);
			}
		} else if (header.nplanes == 4) {
			for (y = 0, error = FALSE; (y < height) && !error; y++) {
				for (n = 0, out_buff = line_buffer; n < width; n++)
					*out_buff++ = 0;
				error = read_line(expand_buffer, header.bplin * header.nplanes);
				for (n = 0, out_buff = expand_buffer; (n < header.nplanes) & !error; n++) {
					merge_line(out_buff, n, line_buffer, header.bplin);
					out_buff += header.bplin;
				}
    			bitmap_prim(0, y, screen_width, line_buffer);
			}
		}
	}
}

int load_pic(char *file, sBitmap *pic_ptr, int max_width, int max_height, int set_palette, int num_cols)
{
	char s[64];

	reset_pic();
	if (open_pic_file(file)) {
		get_header();
		if (header.manuf == 0x0a) {
			PCX_info.width = (header.x2 - header.x1) + 1;
			PCX_info.height = (header.y2 - header.y1) + 1;
			PCX_info.cols = header.bitpx * header.nplanes;
			if (PCX_info.width > max_width) {
				close_pic_file();
				return FALSE;
			}
			if ((header.version == 0x05) || (header.version == 0x02)) {
				get_colormap(set_palette, num_cols);
				get_bitmap((uchar *)pic_ptr, max_height);
				close_pic_file();
			} else {
				close_pic_file();
				sprintf(s, "Bad PCX version: %d", header.version);
				stop(s);
			}
		} else {
			close_pic_file();
			stop("Not recognised as a PCX image file");
		}
		return TRUE;
	}
	return FALSE;
}

int show_pic(char *file, int max_height, int set_palette, int num_cols)
{
	char s[64];

	reset_pic();
	if (max_height > screen_height)
		max_height = screen_height;
	if (open_pic_file(file)) {
		get_header();
		if (header.manuf == 0x0a) {
			PCX_info.width = (header.x2 - header.x1) + 1;
			PCX_info.height = (header.y2 - header.y1) + 1;
			PCX_info.cols = header.bitpx * header.nplanes;
			if ((header.version == 0x05) || (header.version == 0x02)) {
				get_colormap(set_palette, num_cols);
				show_bitmap();
				close_pic_file();
			} else {
				close_pic_file();
				sprintf(s, "Bad PCX version: %d", header.version);
				stop(s);
			}
		} else {
			close_pic_file();
			stop("Not recognised as a PCX image file");
		}
		return TRUE;
	}
	return FALSE;
}

int load_pcx_info(char *file)
{
	if (open_pic_file(file)) {
		get_header();
		if (header.manuf == 0x0a) {
			PCX_info.width = (header.x2 - header.x1) + 1;
			PCX_info.height = (header.y2 - header.y1) + 1;
			PCX_info.cols = header.bitpx * header.nplanes;
		}
		close_pic_file();
		return TRUE;
	}
	return FALSE;
}

void setup_pic(void)
{
	setup_colours();
	if (line_size < screen_width)
		line_size = screen_width;
	if (line_buffer == NULL) {
		line_buffer = alloc_bytes(line_size);
		if (line_buffer == NULL)
			stop("No memory for line buffer");
		expand_buffer = alloc_bytes(line_size);
		if (expand_buffer == NULL)
			stop("No memory for expand buffer");
	}
}

void tidyup_pic(void)
{
	free(expand_buffer);
	free(line_buffer);
	line_size = 0;
}

static void reset_pic(void)
{
	if (line_buffer == NULL) {
		setup_pic();
	} else if (line_size < screen_width) {
		free(expand_buffer);
		free(line_buffer);
		line_size = screen_width;
		line_buffer = alloc_bytes(line_size);
		if (line_buffer == NULL)
			stop("No memory for line buffer");
		expand_buffer = alloc_bytes(line_size);
		if (expand_buffer == NULL)
			stop("No memory for expand buffer");
	}
}

/* PCX image save */

static void write_word(int val, FILE *fp)
{
	fputc(val & 0xff, fp);
	fputc((val >> 8) & 0xff, fp);
}

static void write_pcx_header(FILE *fp)
{
	int i;

	fputc(0x0a, fp);
	fputc(0x05, fp);
	fputc(1, fp);
	fputc(8, fp);
	write_word(0, fp);
	write_word(0, fp);
	write_word(screen_width - 1, fp);
	write_word(screen_height - 1, fp);
	write_word(screen_width, fp);
	write_word(screen_height, fp);
	for (i = 0; i < 48; i++)
		fputc(0, fp);
	fputc(0, fp);
	fputc(1, fp);
	write_word(screen_width, fp);
	write_word(1, fp);
	write_word(0, fp);
	write_word(0, fp);
	for (i = 0; i < 54; i++)
		fputc(0, fp);
}

static void write_pcx_run(int col, int length, FILE *fp)
{
	if (length == 1) {
		if (col < 192) {
			fputc(col, fp);
		} else {
			fputc(0xc1, fp);
			fputc(col, fp);
		}
	} else {
		while (length > 63) {
			fputc(0xff, fp);
			fputc(col, fp);
			length -= 63;
		}
		if (length > 0) {
			fputc(0xc0 | length, fp);
			fputc(col, fp);
		}
	}
}

static void write_pcx_data(FILE *fp)
{
	int x, y, i, col, run_count, run_col;
    Screen_clip s;

    s = clip_region;
    clipping(0, screen_width - 1, 0, screen_height - 1);
	for (y = 0; y < screen_height; y++) {
		run_count = 0;
		run_col = getpixel_prim(0, y);
		for (x = 0; x < screen_width; x++) {
			col = getpixel_prim(x, y);
			if (col == run_col) {
				run_count++;
			} else {
				write_pcx_run(run_col, run_count, fp);
				run_count = 1;
				run_col = col;
			}
		}
		write_pcx_run(run_col, run_count, fp);
	}
    clipping(s.left, s.right, s.top, s.bottom);
}

static void write_pcx_palette(FILE *fp)
{
	int i;

	fputc(0x0c, fp);
	for (i = 0; i < 256; i++) {
		fputc(used_palette[i].red, fp);
		fputc(used_palette[i].green, fp);
		fputc(used_palette[i].blue, fp);
	}
}

void save_screen(char *file_name)
{
	FILE *fp;

	reset_pic();
	fp = fopen(file_name, "wb");
	if (fp != NULL) {
		write_pcx_header(fp);
		write_pcx_data(fp);
		write_pcx_palette(fp);
		fclose(fp);
	}
}

