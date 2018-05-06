/* Must be compiled using -ml with 16 bit Symantec */

#include <stdlib.h>
#include <stdio.h>
#include <dos.h>

static char buffer[256];

static union REGS r;

static void set_mode(int n)
{
	if (n > 0xff) {
		r.h.ah = 0x4f;
		r.h.al = 0x02;
		r.x.ebx = n;
		int86(0x10, &r, &r);
	} else {
		r.h.ah = 0x00;
		r.h.al = n;
		int86(0x10, &r, &r);
	}
}

static int get_info(char *buff)
{
	struct SREGS sr;

	r.h.ah = 0x4f;
	r.h.al = 0x00;
	r.x.edi = FP_OFF(buff);
	sr.es = FP_SEG(buff);
	int86x(0x10, &r, &r, &sr);
	if (r.x.eax != 0x004f) {
		printf("get VESA info failed: %4x\n", r.x.eax);
		return 0;
	}
	return 1;
}

static void get_mode(int n, char *buff)
{
	struct SREGS sr;

	r.h.ah = 0x4f;
	r.h.al = 0x01;
	r.x.ecx = n;
	r.x.edi = FP_OFF(buff);
	sr.es = FP_SEG(buff);
	int86x(0x10, &r, &r, &sr);
	if (r.x.eax != 0x004f) {
		printf("get mode failed: %4x\n", r.x.eax);
	}
}

static void set_page(int n)
{
	r.h.ah = 0x4f;
	r.h.al = 0x05;
	r.h.bh = 0x00;
	r.h.bl = 0x00;
	r.x.edx = n * 0x1000;
	int86(0x10, &r, &r);
}

static int get_page(void)
{
	r.h.ah = 0x4f;
	r.h.al = 0x05;
	r.h.bh = 0x01;
	r.h.bl = 0x00;
	int86(0x10, &r, &r);
	return(r.x.edx);
}

static void set_line_length(int n)
{
	union REGS r;

	r.h.ah = 0x4f;
	r.h.al = 0x06;
	r.h.bh = 0x00;
	r.h.bl = 0x00;
	r.x.ecx = n;
	int86(0x10, &r, &r);
	if (r.x.eax != 0x004f)
		printf("set_line_length failed\n");
	if (r.x.ecx != n)
		printf("wrong line length in set_line_length\n");
}

static void save_mode(int mode, FILE *fp)
{
	int i;
	char *b_info;
	short *w_info;

	fprintf(fp, "DATA FOR MODE %X\n", mode);
	for (i = 0; i < 256; i++)
		buffer[i] = 0;
	get_mode(mode, buffer);
	b_info = (char *)buffer;
	w_info = (short *)buffer;
	fprintf(fp, "Attributes: mode=%x, WinA=%x, WinB=%x\n", w_info[0], b_info[2], b_info[3]);
	fprintf(fp, "Win granularity: %4x\n", w_info[2]);
	fprintf(fp, "Win size: %4x\n", w_info[3]);
	fprintf(fp, "Win segments: A=%x, B=%x\n", w_info[4], w_info[5]);
	fprintf(fp, "Win func ptr: 1=%x, 2=%x\n", w_info[6], w_info[7]);
	fprintf(fp, "Bytes per scan line: %d\n", w_info[8]);
	if (w_info[0] & 0x0002) {
		fprintf(fp, "resolution: X=%d, Y=%d\n", w_info[9], w_info[10]);
		fprintf(fp, "char size: X=%d, Y=%d\n", b_info[22], b_info[23]);
		fprintf(fp, "Number of planes: %d\n", b_info[24]);
		fprintf(fp, "Bits per pixel: %d\n", b_info[25]);
		fprintf(fp, "Number of banks: %d\n", b_info[26]);
		fprintf(fp, "Memory model: %2x\n", b_info[27]);
		fprintf(fp, "Bank size: %2x\n", b_info[28]);
		fprintf(fp, "Number of image pages: %2x\n", b_info[29]);
	}
}

static int save_info(int mode, FILE *fp)
{
	int i, found;
	char *b_info;
	short *w_info;
	short far *modes;
	char s[64];
	char far *p;

	found = 0;
	for (i = 0; i < 256; i++)
		buffer[i] = 0;
	if (get_info(buffer)) {
		b_info = (char *)buffer;
		w_info = (short *)buffer;
		for (i = 0; i < 4; i++) {
			s[i] = b_info[i];
		}
		s[4] = '\0';
		fprintf(fp, "Signature: %s, Version: %d\n", s, w_info[2]);
		p = MK_FP(w_info[4], w_info[3]);
		fprintf(fp, "CARD ID: ");
		for (i = 0; (*p != '\0') && (i < 60); i++) {
			fprintf(fp, "%c", *p++);
		}
		fprintf(fp, "\n");
		if (b_info[10] & 0x01)
			fprintf(fp, "Programmable 8 bit palette\n");
		else
			fprintf(fp, "Fixed 6 bit palette\n");
		fprintf(fp, "Total memory: %d Kbytes\n", w_info[9] * 64);
		fprintf(fp, "Supported modes: ");
		modes = MK_FP(w_info[8], w_info[7]);
		for (i = 0; (modes[i] >= 0) && (i < 20); i++) {
			fprintf(fp, "%x, ", modes[i]);
			if (modes[i] == mode)
				found = 1;
		}
		fprintf(fp, "\n");
	}
	return found;
}

int main(int argc, char** argv)
{
	int mode;
	FILE *fp;

	mode = 0x101;
	if (argc == 2)
		sscanf(argv[1], "%X", &mode);
	fp = fopen("VESA.DAT", "w");
	if (fp) {
		if (save_info(mode, fp)) {
			save_mode(mode, fp);
		} else {
			printf("\n");
			printf("Mode %X not supported", mode);
			printf("\n");
		}
		fclose(fp);
	}
	return 0;
}
